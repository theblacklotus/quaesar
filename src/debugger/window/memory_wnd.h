// Based on:
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

#pragma once

#include <imgui_eastl.h>
#include <stdint.h>  // uint8_t, etc.
#include <stdio.h>   // sprintf, scanf
#include "debugger/ui/ui_view.h"

namespace qd::window {

class MemoryView : public UiWindow {
    QDB_CLASS_ID(WndId::MemoryView);
    void* m_memAddr = 0;
    size_t m_memSize = 0;
    size_t m_baseDisplayAddr = 0x0000;

public:
    enum DataFormat { DataFormat_Bin = 0, DataFormat_Dec = 1, DataFormat_Hex = 2, DataFormat_COUNT };

    // Settings
    // = false  // disable any editing.
    bool read_only;
    // = 16     // number of columns to display.
    int cols;
    // = true   // display options button/context menu. when disabled, options will be locked unless you provide your
    // own ui for them.
    bool opt_show_options;
    // = false  // display a footer previewing the decimal/binary/hex/float representation of the currently selected
    // bytes.
    bool opt_show_data_preview;
    // = false  // display values in hexii representation instead of regular hexadecimal: hide null/zero bytes, ascii
    // values as ".x".
    bool opt_show_hexii;
    // = true   // display ascii representation on the right side.
    bool opt_show_ascii;
    // = true   // display null/zero bytes using the textdisabled color.
    bool opt_greyout_zeroes;
    // = true   // display hexadecimal values as "ff" instead of "ff".
    bool opt_upper_case_hex;
    // = 8      // set to 0 to disable extra spacing between every mid-cols.
    int opt_mid_cols_count;
    // = 0      // number of addr digits to display (default calculated based on maximum displayed addr).
    int opt_addr_digits_count;
    // = 0      // space to reserve at the bottom of the widget to add custom widgets
    float opt_footer_extra_height;
    //          // background color of highlighted bytes.
    uint32_t highlight_color;
    // = 0      // optional handler to read bytes.
    uint8_t (*read_fn)(const uint8_t* data, size_t off);
    // = 0      // optional handler to write bytes.
    void (*write_fn)(uint8_t* data, size_t off, uint8_t d);
    //= 0      // optional handler to return Highlight property (to support non-contiguous highlighting).
    bool (*highlight_fn)(const uint8_t* data, size_t off);

    // [Internal State]
    bool contents_width_changed;
    size_t data_preview_addr;
    size_t data_editing_addr;
    bool data_editing_take_fucus;
    char data_input_buffer[32];
    char addr_input_buffer[32];
    size_t goto_address;
    size_t highlight_min, hightlight_max;
    int preview_endianess;
    ImGuiDataType preview_data_type;

    struct Sizes {
        int addr_digits_count;
        float line_height;
        float glyph_width;
        float hex_cell_width;
        float spacing_between_mid_cols;
        float pos_hex_start;
        float pos_hex_end;
        float pos_ascii_start;
        float pos_ascii_end;
        float window_width;

        Sizes() {
            memset(this, 0, sizeof(*this));
        }
    };

    MemoryView(UiViewCreate* cp);
    virtual void drawContent() override;

    void goto_address_and_highlight(size_t addr_min, size_t addr_max);
    void setMemAddr(void* mem_data, size_t mem_size, size_t base_display_addr = 0x0000);

private:
    void draw_contents(void* mem_data_void, size_t mem_size, size_t base_display_addr = 0x0000);
    void draw_options_line(const Sizes& s, void* mem_data, size_t mem_size, size_t base_display_addr);
    void draw_preview_line(const Sizes& s, void* mem_data_void, size_t mem_size, size_t base_display_addr);
    void calc_sizes(Sizes& s, size_t mem_size, size_t base_display_addr);
    const char* data_dype_get_desc(ImGuiDataType data_type) const;
    size_t data_type_get_size(ImGuiDataType data_type) const;
    void draw_preview_data(size_t addr, const uint8_t* mem_data, size_t mem_size, ImGuiDataType data_type,
                           DataFormat data_format, char* out_buf, size_t out_buf_size) const;

    void* endianness_copy(void* dst, void* src, size_t size) const;
};

};  // namespace qd::window
