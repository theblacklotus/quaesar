// Mini memory editor for Dear ImGui (to embed in your game/tools)
// Get latest version at http://www.github.com/ocornut/imgui_club
//
// Right-click anywhere to access the Options menu!
// You can adjust the keyboard repeat delay/rate in ImGuiIO.
// The code assume a mono-space font for simplicity!
// If you don't use the default font, use ImGui::PushFont()/PopFont() to switch to a mono-space font before calling
// this.
//
// Usage:
//   // Create a window and draw memory editor inside it:
//   static MemoryView mem_edit_1;
//   static char data[0x10000];
//   size_t data_size = 0x10000;
//   mem_edit_1.draw_window("Memory Editor", data, data_size);
//
// Usage:
//   // If you already have a window, use draw_contents() instead:
//   static MemoryView mem_edit_2;
//   ImGui::Begin("MyWindow")
//   mem_edit_2.draw_contents(this, sizeof(*this), (size_t)this);
//   ImGui::End();
//
// Changelog:
// - v0.10: initial version
// - v0.23 (2017/08/17): added to github. fixed right-arrow triggering a byte write.
// - v0.24 (2018/06/02): changed DragInt("Rows" to use a %d data format (which is desirable since imgui 1.61).
// - v0.25 (2018/07/11): fixed wording: all occurrences of "Rows" renamed to "Columns".
// - v0.26 (2018/08/02): fixed clicking on hex region
// - v0.30 (2018/08/02): added data preview for common data types
// - v0.31 (2018/10/10): added opt_upper_case_hex option to select lower/upper casing display [@samhocevar]
// - v0.32 (2018/10/10): changed signatures to use void* instead of unsigned char*
// - v0.33 (2018/10/10): added opt_show_options option to hide all the interactive option setting.
// - v0.34 (2019/05/07): binary preview now applies endianness setting [@nicolasnoble]
// - v0.35 (2020/01/29): using ImGuiDataType available since Dear ImGui 1.69.
// - v0.36 (2020/05/05): minor tweaks, minor refactor.
// - v0.40 (2020/10/04): fix misuse of ImGuiListClipper API, broke with Dear ImGui 1.79. made cursor position appears on
// left-side of edit box. option popup appears on mouse release. fix MSVC warnings where _CRT_SECURE_NO_WARNINGS wasn't
// working in recent versions.
// - v0.41 (2020/10/05): fix when using with keyboard/gamepad navigation enabled.
// - v0.42 (2020/10/14): fix for . character in ASCII view always being greyed out.
// - v0.43 (2021/03/12): added opt_footer_extra_height to allow for custom drawing at the bottom of the editor
// [@leiradel]
// - v0.44 (2021/03/12): use ImGuiInputTextFlags_AlwaysOverwrite in 1.82 + fix hardcoded width.
// - v0.50 (2021/11/12): various fixes for recent dear imgui versions (fixed misuse of clipper, relying on
// SetKeyboardFocusHere() handling scrolling from 1.85). added default size.
//
// Todo/Bugs:
// - This is generally old/crappy code, it should work but isn't very good.. to be rewritten some day.
// - PageUp/PageDown are supported because we use _NoNav. This is a good test scenario for working out idioms of how to
// mix natural nav and our own...
// - Arrows are being sent to the InputText() about to disappear which for LeftArrow makes the text cursor appear at
// position 1 for one frame.
// - Using InputText() is awkward and maybe overkill here, consider implementing something custom.

#include "memory_wnd.h"
#include <debugger/debugger.h>

namespace qd::window {

QDB_WINDOW_REGISTER(MemoryView);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#define _PRISizeT "I"
#define ImSnprintf _snprintf
#else
#define _PRISizeT "z"
#define ImSnprintf snprintf
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MemoryView::MemoryView(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Memory";
    mVisible = true;

    // Settings
    read_only = false;
    cols = 16;
    opt_show_options = true;
    opt_show_data_preview = false;
    opt_show_hexii = false;
    opt_show_ascii = true;
    opt_greyout_zeroes = true;
    opt_upper_case_hex = true;
    opt_mid_cols_count = 8;
    opt_addr_digits_count = 0;
    opt_footer_extra_height = 0.0f;
    highlight_color = IM_COL32(255, 255, 255, 50);
    read_fn = nullptr;
    write_fn = nullptr;
    highlight_fn = nullptr;

    // State/Internals
    contents_width_changed = false;
    data_preview_addr = data_editing_addr = (size_t)-1;
    data_editing_take_fucus = false;
    memset(data_input_buffer, 0, sizeof(data_input_buffer));
    memset(addr_input_buffer, 0, sizeof(addr_input_buffer));
    goto_address = (size_t)-1;
    highlight_min = hightlight_max = (size_t)-1;
    preview_endianess = 0;
    preview_data_type = ImGuiDataType_S32;

    setMemAddr(getDbg()->addrToPtr(0x0000), 512 * 1024, 0x0000);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryView::goto_address_and_highlight(size_t addr_min, size_t addr_max) {
    goto_address = addr_min;
    highlight_min = addr_min;
    hightlight_max = addr_max;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryView::calc_sizes(Sizes& s, size_t mem_size, size_t base_display_addr) {
    ImGuiStyle& style = ImGui::GetStyle();
    s.addr_digits_count = opt_addr_digits_count;

    if (s.addr_digits_count == 0) {
        for (size_t n = base_display_addr + mem_size - 1; n > 0; n >>= 4)
            s.addr_digits_count++;
    }

    s.line_height = ImGui::GetTextLineHeight();
    s.glyph_width = ImGui::CalcTextSize("F").x + 1;  // We assume the font is mono-space

    // "FF " we include trailing space in the width to easily catch clicks everywhere
    s.hex_cell_width = (float)(int)(s.glyph_width * 2.5f);

    // Every opt_mid_cols_count columns we add a bit of extra spacing
    s.spacing_between_mid_cols = (float)(int)(s.hex_cell_width * 0.25f);
    s.pos_hex_start = (s.addr_digits_count + 2) * s.glyph_width;
    s.pos_hex_end = s.pos_hex_start + (s.hex_cell_width * cols);
    s.pos_ascii_start = s.pos_ascii_end = s.pos_hex_end;

    if (opt_show_ascii) {
        s.pos_ascii_start = s.pos_hex_end + s.glyph_width * 1;
        if (opt_mid_cols_count > 0)
            s.pos_ascii_start +=
                (float)((cols + opt_mid_cols_count - 1) / opt_mid_cols_count) * s.spacing_between_mid_cols;
        s.pos_ascii_end = s.pos_ascii_start + cols * s.glyph_width;
    }

    s.window_width = s.pos_ascii_end + style.ScrollbarSize + style.WindowPadding.x * 2 + s.glyph_width;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryView::setMemAddr(void* mem_data, size_t mem_size, size_t base_display_addr) {
    m_memAddr = mem_data;
    m_memSize = mem_size;
    m_baseDisplayAddr = base_display_addr;
}

void MemoryView::drawContent() {
    MemoryView::Sizes s;
    calc_sizes(s, m_memSize, m_baseDisplayAddr);
    ImGui::SetNextWindowSize(ImVec2(s.window_width, s.window_width * 0.60f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.window_width, FLT_MAX));

    if (ImGui::Begin(mTitle.c_str(), &mVisible, ImGuiWindowFlags_NoScrollbar)) {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");
        draw_contents(m_memAddr, m_memSize, m_baseDisplayAddr);
        if (contents_width_changed) {
            calc_sizes(s, m_memSize, m_baseDisplayAddr);
            ImGui::SetWindowSize(ImVec2(s.window_width, ImGui::GetWindowSize().y));
        }
    }
    ImGui::End();
}

void MemoryView::draw_contents(void* mem_data_void, size_t mem_size, size_t base_display_addr) {
    if (cols < 1)
        cols = 1;

    uint8_t* mem_data = (uint8_t*)mem_data_void;
    Sizes s;
    calc_sizes(s, mem_size, base_display_addr);
    ImGuiStyle& style = ImGui::GetStyle();

    // We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove' in order to prevent click from moving
    // the window. This is used as a facility since our main click detection code doesn't assign an ActiveId so the
    // click would normally be caught as a window-move.
    const float height_separator = style.ItemSpacing.y;
    float footer_height = opt_footer_extra_height;

    if (opt_show_options) {
        footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1;
    }

    if (opt_show_data_preview) {
        footer_height +=
            height_separator + ImGui::GetFrameHeightWithSpacing() * 1 + ImGui::GetTextLineHeightWithSpacing() * 3;
    }

    ImGui::BeginChild("##scrolling", ImVec2(0, -footer_height), false,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    // We are not really using the clipper API correctly here, because we rely on
    // visible_start_addr/visible_end_addr for our scrolling function.
    const int line_total_count = (int)((mem_size + cols - 1) / cols);
    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, s.line_height);

    bool data_next = false;

    if (read_only || data_editing_addr >= mem_size)
        data_editing_addr = (size_t)-1;
    if (data_preview_addr >= mem_size)
        data_preview_addr = (size_t)-1;

    size_t preview_data_type_size = opt_show_data_preview ? data_type_get_size(preview_data_type) : 0;

    size_t data_editing_addr_next = (size_t)-1;
    if (data_editing_addr != (size_t)-1) {
        // Move cursor but only apply on next frame so scrolling with be synchronized (because currently we can't
        // change the scrolling while the window is being rendered)
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) &&
            (ptrdiff_t)data_editing_addr >= (ptrdiff_t)cols) {
            data_editing_addr_next = data_editing_addr - cols;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) &&
                   (ptrdiff_t)data_editing_addr < (ptrdiff_t)mem_size - cols) {
            data_editing_addr_next = data_editing_addr + cols;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) &&
                   (ptrdiff_t)data_editing_addr > (ptrdiff_t)0) {
            data_editing_addr_next = data_editing_addr - 1;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) &&
                   (ptrdiff_t)data_editing_addr < (ptrdiff_t)mem_size - 1) {
            data_editing_addr_next = data_editing_addr + 1;
        }
    }

    // Draw vertical separator
    ImVec2 window_pos = ImGui::GetWindowPos();
    if (opt_show_ascii)
        draw_list->AddLine(ImVec2(window_pos.x + s.pos_ascii_start - s.glyph_width, window_pos.y),
                           ImVec2(window_pos.x + s.pos_ascii_start - s.glyph_width, window_pos.y + 9999),
                           ImGui::GetColorU32(ImGuiCol_Border));

    const ImU32 color_text = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 color_disabled = opt_greyout_zeroes ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : color_text;

    const char* format_address = opt_upper_case_hex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
    const char* format_data = opt_upper_case_hex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
    const char* format_byte = opt_upper_case_hex ? "%02X" : "%02x";
    const char* format_byte_space = opt_upper_case_hex ? "%02X " : "%02x ";

    while (clipper.Step())
        for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++)  // display only visible lines
        {
            size_t addr = (size_t)(line_i * cols);
            ImGui::Text(format_address, s.addr_digits_count, base_display_addr + addr);

            // Draw Hexadecimal
            for (int n = 0; n < cols && addr < mem_size; n++, addr++) {
                float byte_pos_x = s.pos_hex_start + s.hex_cell_width * n;
                if (opt_mid_cols_count > 0)
                    byte_pos_x += (float)(n / opt_mid_cols_count) * s.spacing_between_mid_cols;
                ImGui::SameLine(byte_pos_x);

                // Draw highlight
                bool is_highlight_from_user_range = (addr >= highlight_min && addr < hightlight_max);
                bool is_highlight_from_user_func = (highlight_fn && highlight_fn(mem_data, addr));
                bool is_highlight_from_preview =
                    (addr >= data_preview_addr && addr < data_preview_addr + preview_data_type_size);
                if (is_highlight_from_user_range || is_highlight_from_user_func || is_highlight_from_preview) {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    float highlight_width = s.glyph_width * 2;
                    bool is_next_byte_highlighted =
                        (addr + 1 < mem_size) && ((hightlight_max != (size_t)-1 && addr + 1 < hightlight_max) ||
                                                  (highlight_fn && highlight_fn(mem_data, addr + 1)));
                    if (is_next_byte_highlighted || (n + 1 == cols)) {
                        highlight_width = s.hex_cell_width;
                        if (opt_mid_cols_count > 0 && n > 0 && (n + 1) < cols && ((n + 1) % opt_mid_cols_count) == 0)
                            highlight_width += s.spacing_between_mid_cols;
                    }
                    draw_list->AddRectFilled(pos, ImVec2(pos.x + highlight_width, pos.y + s.line_height),
                                             highlight_color);
                }

                if (data_editing_addr == addr) {
                    // Display text input on current byte
                    bool data_write = false;
                    ImGui::PushID((void*)addr);
                    if (data_editing_take_fucus) {
                        ImGui::SetKeyboardFocusHere(0);
                        sprintf(addr_input_buffer, format_data, s.addr_digits_count, base_display_addr + addr);
                        sprintf(data_input_buffer, format_byte, read_fn ? read_fn(mem_data, addr) : mem_data[addr]);
                    }
                    struct UserData {
                        // FIXME: We should have a way to retrieve the text edit cursor position more easily in the
                        // API, this is rather tedious. This is such a ugly mess we may be better off not using
                        // InputText() at all here.
                        static int Callback(ImGuiInputTextCallbackData* data) {
                            UserData* user_data = (UserData*)data->UserData;
                            if (!data->HasSelection())
                                user_data->CursorPos = data->CursorPos;
                            if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen) {
                                // When not editing a byte, always refresh its InputText content pulled from
                                // underlying memory data (this is a bit tricky, since InputText technically "owns"
                                // the master copy of the buffer we edit it in there)
                                data->DeleteChars(0, data->BufTextLen);
                                data->InsertChars(0, user_data->CurrentBufOverwrite);
                                data->SelectionStart = 0;
                                data->SelectionEnd = 2;
                                data->CursorPos = 0;
                            }
                            return 0;
                        }
                        char CurrentBufOverwrite[3];  // Input
                        int CursorPos;                // Output
                    };
                    UserData user_data;
                    user_data.CursorPos = -1;
                    sprintf(user_data.CurrentBufOverwrite, format_byte,
                            read_fn ? read_fn(mem_data, addr) : mem_data[addr]);
                    ImGuiInputTextFlags flags =
                        ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue |
                        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll |
                        ImGuiInputTextFlags_CallbackAlways;
                    flags |= ImGuiInputTextFlags_AlwaysOverwrite;

                    ImGui::SetNextItemWidth(s.glyph_width * 2);
                    if (ImGui::InputText("##data", data_input_buffer, IM_ARRAYSIZE(data_input_buffer), flags,
                                         UserData::Callback, &user_data))
                        data_write = data_next = true;
                    else if (!data_editing_take_fucus && !ImGui::IsItemActive())
                        data_editing_addr = data_editing_addr_next = (size_t)-1;
                    data_editing_take_fucus = false;
                    if (user_data.CursorPos >= 2)
                        data_write = data_next = true;
                    if (data_editing_addr_next != (size_t)-1)
                        data_write = data_next = false;
                    unsigned int data_input_value = 0;
                    if (data_write && sscanf(data_input_buffer, "%X", &data_input_value) == 1) {
                        if (write_fn)
                            write_fn(mem_data, addr, (uint8_t)data_input_value);
                        else
                            mem_data[addr] = (uint8_t)data_input_value;
                    }
                    ImGui::PopID();
                } else {
                    // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click
                    // on.
                    uint8_t b = read_fn ? read_fn(mem_data, addr) : mem_data[addr];

                    if (opt_show_hexii) {
                        if ((b >= 32 && b < 128))
                            ImGui::Text(".%c ", b);
                        else if (b == 0xFF && opt_greyout_zeroes)
                            ImGui::TextDisabled("## ");
                        else if (b == 0x00)
                            ImGui::Text("   ");
                        else
                            ImGui::Text(format_byte_space, b);
                    } else {
                        if (b == 0 && opt_greyout_zeroes)
                            ImGui::TextDisabled("00 ");
                        else
                            ImGui::Text(format_byte_space, b);
                    }
                    if (!read_only && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                        data_editing_take_fucus = true;
                        data_editing_addr_next = addr;
                    }
                }
            }

            if (opt_show_ascii) {
                // Draw ASCII values
                ImGui::SameLine(s.pos_ascii_start);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                addr = line_i * cols;
                ImGui::PushID(line_i);
                if (ImGui::InvisibleButton("ascii", ImVec2(s.pos_ascii_end - s.pos_ascii_start, s.line_height))) {
                    data_editing_addr = data_preview_addr =
                        addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / s.glyph_width);
                    data_editing_take_fucus = true;
                }
                ImGui::PopID();
                for (int n = 0; n < cols && addr < mem_size; n++, addr++) {
                    if (addr == data_editing_addr) {
                        draw_list->AddRectFilled(pos, ImVec2(pos.x + s.glyph_width, pos.y + s.line_height),
                                                 ImGui::GetColorU32(ImGuiCol_FrameBg));
                        draw_list->AddRectFilled(pos, ImVec2(pos.x + s.glyph_width, pos.y + s.line_height),
                                                 ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
                    }
                    unsigned char c = read_fn ? read_fn(mem_data, addr) : mem_data[addr];
                    char display_c = (c < 32 || c >= 128) ? '.' : c;
                    draw_list->AddText(pos, (display_c == c) ? color_text : color_disabled, &display_c, &display_c + 1);
                    pos.x += s.glyph_width;
                }
            }
        }
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    // Notify the main window of our ideal child content size (FIXME: we are missing an API to get the contents size
    // from the child)
    ImGui::SetCursorPosX(s.window_width);

    if (data_next && data_editing_addr + 1 < mem_size) {
        data_editing_addr = data_preview_addr = data_editing_addr + 1;
        data_editing_take_fucus = true;
    } else if (data_editing_addr_next != (size_t)-1) {
        data_editing_addr = data_preview_addr = data_editing_addr_next;
        data_editing_take_fucus = true;
    }

    const bool lock_show_data_preview = opt_show_data_preview;
    if (opt_show_options) {
        ImGui::Separator();
        draw_options_line(s, mem_data, mem_size, base_display_addr);
    }

    if (lock_show_data_preview) {
        ImGui::Separator();
        draw_preview_line(s, mem_data, mem_size, base_display_addr);
    }
}

void MemoryView::draw_options_line(const Sizes& s, void* mem_data, size_t mem_size, size_t base_display_addr) {
    IM_UNUSED(mem_data);
    ImGuiStyle& style = ImGui::GetStyle();
    const char* format_range = opt_upper_case_hex ? "Range %0*" _PRISizeT "X..%0*" _PRISizeT "X"
                                                  : "Range %0*" _PRISizeT "x..%0*" _PRISizeT "x";

    // Options menu
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("context");
    if (ImGui::BeginPopup("context")) {
        ImGui::SetNextItemWidth(s.glyph_width * 7 + style.FramePadding.x * 2.0f);
        if (ImGui::DragInt("##cols", &cols, 0.2f, 4, 32, "%d cols")) {
            contents_width_changed = true;
            if (cols < 1)
                cols = 1;
        }
        ImGui::Checkbox("Show Data Preview", &opt_show_data_preview);
        ImGui::Checkbox("Show HexII", &opt_show_hexii);
        if (ImGui::Checkbox("Show Ascii", &opt_show_ascii)) {
            contents_width_changed = true;
        }
        ImGui::Checkbox("Grey out zeroes", &opt_greyout_zeroes);
        ImGui::Checkbox("Uppercase Hex", &opt_upper_case_hex);

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text(format_range, s.addr_digits_count, base_display_addr, s.addr_digits_count,
                base_display_addr + mem_size - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth((s.addr_digits_count + 1) * s.glyph_width + style.FramePadding.x * 2.0f);
    if (ImGui::InputText("##addr", addr_input_buffer, IM_ARRAYSIZE(addr_input_buffer),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        size_t goto_addr;
        if (sscanf(addr_input_buffer, "%" _PRISizeT "X", &goto_addr) == 1) {
            goto_address = goto_addr - base_display_addr;
            highlight_min = hightlight_max = (size_t)-1;
        }
    }

    if (goto_address != (size_t)-1) {
        if (goto_address < mem_size) {
            ImGui::BeginChild("##scrolling");
            ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + (goto_address / cols) * ImGui::GetTextLineHeight());
            ImGui::EndChild();
            data_editing_addr = data_preview_addr = goto_address;
            data_editing_take_fucus = true;
        }
        goto_address = (size_t)-1;
    }
}

void MemoryView::draw_preview_line(const Sizes& s, void* mem_data_void, size_t mem_size, size_t base_display_addr) {
    IM_UNUSED(base_display_addr);
    uint8_t* mem_data = (uint8_t*)mem_data_void;
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Preview as:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth((s.glyph_width * 10.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
    if (ImGui::BeginCombo("##combo_type", data_dype_get_desc(preview_data_type), ImGuiComboFlags_HeightLargest)) {
        for (int n = 0; n < ImGuiDataType_COUNT; n++)
            if (ImGui::Selectable(data_dype_get_desc((ImGuiDataType)n), preview_data_type == n))
                preview_data_type = (ImGuiDataType)n;
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth((s.glyph_width * 6.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
    ImGui::Combo("##combo_endianess", &preview_endianess, "LE\0BE\0\0");

    char buf[128] = "";
    float x = s.glyph_width * 6.0f;
    bool has_value = data_preview_addr != (size_t)-1;
    if (has_value)
        draw_preview_data(data_preview_addr, mem_data, mem_size, preview_data_type, DataFormat_Dec, buf,
                          (size_t)IM_ARRAYSIZE(buf));
    ImGui::Text("Dec");
    ImGui::SameLine(x);
    ImGui::TextUnformatted(has_value ? buf : "N/A");
    if (has_value)
        draw_preview_data(data_preview_addr, mem_data, mem_size, preview_data_type, DataFormat_Hex, buf,
                          (size_t)IM_ARRAYSIZE(buf));
    ImGui::Text("Hex");
    ImGui::SameLine(x);
    ImGui::TextUnformatted(has_value ? buf : "N/A");
    if (has_value)
        draw_preview_data(data_preview_addr, mem_data, mem_size, preview_data_type, DataFormat_Bin, buf,
                          (size_t)IM_ARRAYSIZE(buf));
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    ImGui::Text("Bin");
    ImGui::SameLine(x);
    ImGui::TextUnformatted(has_value ? buf : "N/A");
}

// Utilities for Data Preview
const char* MemoryView::data_dype_get_desc(ImGuiDataType data_type) const {
    const char* descs[] = {"Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double"};
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return descs[data_type];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t MemoryView::data_type_get_size(ImGuiDataType data_type) const {
    const size_t sizes[] = {1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double)};
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return sizes[data_type];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
const char* get_format_get_desc(MemoryView::DataFormat data_format) {
    const char* descs[] = {"Bin", "Dec", "Hex"};
    IM_ASSERT(data_format >= 0 && data_format < DataFormat_COUNT);
    return descs[data_format];
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool is_big_endian() {
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void* endianness_copy_big_endian(void* _dst, void* _src, size_t s, int is_little_endian) {
    if (is_little_endian) {
        uint8_t* dst = (uint8_t*)_dst;
        uint8_t* src = (uint8_t*)_src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i)
            memcpy(dst++, src--, 1);
        return _dst;
    } else {
        return memcpy(_dst, _src, s);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void* endianness_copy_little_endian(void* _dst, void* _src, size_t s, int is_little_endian) {
    if (is_little_endian) {
        return memcpy(_dst, _src, s);
    } else {
        uint8_t* dst = (uint8_t*)_dst;
        uint8_t* src = (uint8_t*)_src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i)
            memcpy(dst++, src--, 1);
        return _dst;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* MemoryView::endianness_copy(void* dst, void* src, size_t size) const {
    static void* (*fp)(void*, void*, size_t, int) = NULL;
    if (fp == NULL)
        fp = is_big_endian() ? endianness_copy_big_endian : endianness_copy_little_endian;
    return fp(dst, src, size, preview_endianess);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* format_binary(const uint8_t* buf, int width) {
    IM_ASSERT(width <= 64);
    size_t out_n = 0;
    static char out_buf[64 + 8 + 1];
    int n = width / 8;
    for (int j = n - 1; j >= 0; --j) {
        for (int i = 0; i < 8; ++i)
            out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
        out_buf[out_n++] = ' ';
    }
    IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
    out_buf[out_n] = 0;
    return out_buf;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryView::draw_preview_data(size_t addr, const uint8_t* mem_data, size_t mem_size, ImGuiDataType data_type,
                                   DataFormat data_format, char* out_buf, size_t out_buf_size) const {
    uint8_t buf[8];
    size_t elem_size = data_type_get_size(data_type);
    size_t size = addr + elem_size > mem_size ? mem_size - addr : elem_size;
    if (read_fn)
        for (int i = 0, n = (int)size; i < n; ++i)
            buf[i] = read_fn(mem_data, addr + i);
    else
        memcpy(buf, mem_data + addr, size);

    if (data_format == DataFormat_Bin) {
        uint8_t binbuf[8];
        endianness_copy(binbuf, buf, size);
        ImSnprintf(out_buf, out_buf_size, "%s", format_binary(binbuf, (int)size * 8));
        return;
    }

    out_buf[0] = 0;
    switch (data_type) {
        case ImGuiDataType_S8: {
            int8_t int8 = 0;
            endianness_copy(&int8, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hhd", int8);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF);
                return;
            }
            break;
        }
        case ImGuiDataType_U8: {
            uint8_t uint8 = 0;
            endianness_copy(&uint8, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hhu", uint8);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF);
                return;
            }
            break;
        }
        case ImGuiDataType_S16: {
            int16_t int16 = 0;
            endianness_copy(&int16, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hd", int16);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF);
                return;
            }
            break;
        }
        case ImGuiDataType_U16: {
            uint16_t uint16 = 0;
            endianness_copy(&uint16, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hu", uint16);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF);
                return;
            }
            break;
        }
        case ImGuiDataType_S32: {
            int32_t int32 = 0;
            endianness_copy(&int32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%d", int32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%08x", int32);
                return;
            }
            break;
        }
        case ImGuiDataType_U32: {
            uint32_t uint32 = 0;
            endianness_copy(&uint32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%u", uint32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32);
                return;
            }
            break;
        }
        case ImGuiDataType_S64: {
            int64_t int64 = 0;
            endianness_copy(&int64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64);
                return;
            }
            break;
        }
        case ImGuiDataType_U64: {
            uint64_t uint64 = 0;
            endianness_copy(&uint64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64);
                return;
            }
            break;
        }
        case ImGuiDataType_Float: {
            float float32 = 0.0f;
            endianness_copy(&float32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%f", float32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "%a", float32);
                return;
            }
            break;
        }
        case ImGuiDataType_Double: {
            double float64 = 0.0;
            endianness_copy(&float64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%f", float64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "%a", float64);
                return;
            }
            break;
        }
        case ImGuiDataType_COUNT:
            break;
    }  // Switch
    IM_ASSERT(0);  // Shouldn't reach
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef _PRISizeT
#undef ImSnprintf

#ifdef _MSC_VER
#pragma warning(pop)
#endif

};  // namespace qd::window
