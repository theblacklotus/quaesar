#include "disassembly_view.h"
#include <capstone/capstone.h>
#include <dear_imgui/imgui.h>
// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
// clang-format on

static SelectedRegisters s_selected_registers;

static uint32_t s_colors[] = {
    0xb274747f, 0xb280507f, 0xa9b2507f, 0x60b2507f, 0x4fb2927f, 0x4f71b27f, 0x8850b27f, 0xb250917f,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DisassemblyView {
    csh capstone;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DisassemblyView* DisassemblyView_create(csh capstone) {
    DisassemblyView* view = new DisassemblyView();
    view->capstone = capstone;
    return view;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_destroy(DisassemblyView* self) {
    delete self;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_disassembly(DisassemblyView* self) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    uae_u32 pc = M68K_GETPC;
    uae_u32 offset = 40;
    uae_u32 count_bytes = 80;

    uae_u8* pc_addr = memory_get_real_address(pc);
    uae_u32 start_disasm = pc - offset;

#define M68K_CODE "\x38\x38"

    cs_insn* insn = nullptr;
    size_t count = cs_disasm(self->capstone, pc_addr - offset, count_bytes, start_disasm, 0, &insn);
    // size_t count = cs_disasm(self->capstone, (unsigned char*)M68K_CODE, 2, 0, 0, &insn);

    int color_index = 0;
    memset(&s_selected_registers, 0, sizeof(s_selected_registers));

    for (size_t j = 0; j < count; j++) {
        cs_detail* detail = insn[j].detail;

        if (insn[j].address != pc) {
            continue;
        }

        for (int i = 0; i < detail->regs_read_count; i++) {
            int reg_id = detail->regs_read[i];
            s_selected_registers.read_registers[reg_id] = 1;
            s_selected_registers.read_registers_colors[reg_id] = s_colors[color_index & 7];
            color_index++;
        }

        for (int i = 0; i < detail->regs_write_count; i++) {
            int reg_id = detail->regs_write[i];
            s_selected_registers.write_registers[reg_id] = 1;
            s_selected_registers.write_register_colors[reg_id] = s_colors[color_index & 7];
            color_index++;
        }
    }

    int max_instruction_width = 20;

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

    char buffer[512];

    float text_height = ImGui::GetTextLineHeight();
    float text_char_width = ImGui::CalcTextSize("F").x;

    if (ImGui::BeginTable("disassembly", 4, flags)) {
        ImGui::TableSetupColumn("Loc");
        ImGui::TableSetupColumn("Adress");
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableSetupColumn("Cycles");
        ImGui::TableHeadersRow();

        for (size_t j = 0; j < count; j++) {
            cs_detail* detail = insn[j].detail;

            // Assuming we're rendering the triangle in the first column
            ImGui::TableNextColumn();
            if (insn[j].address == pc) {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImVec2 tri[3] = {ImVec2(p.x, p.y), ImVec2(p.x + text_height / 2, p.y + text_height / 2),
                                 ImVec2(p.x, p.y + text_height)};

                ImU32 row_bg_color = ImGui::GetColorU32(ImVec4(0.25f, 0.50f, 0.25f, 0.25f));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);

                draw_list->AddTriangleFilled(tri[0], tri[1], tri[2], IM_COL32(255, 255, 0, 255));
            }

            ImGui::TableNextColumn();
            ImGui::Text("0x%" PRIx64, insn[j].address);
            ImGui::TableNextColumn();

            int width = strlen(insn[j].mnemonic);
            memcpy(buffer, insn[j].mnemonic, width);

            for (int i = 0; i < max_instruction_width - width; i++) {
                buffer[width + i] = ' ';
            }

            ImVec2 p = ImGui::GetCursorScreenPos();

            strcpy(buffer + max_instruction_width, insn[j].op_str);
            ImGui::Text("%s", buffer);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            float op_str_start = p.x + ((max_instruction_width - 1) * text_char_width);

            // Check if the current instruction includes any of the selected registers
            for (int i = 0; i < detail->regs_read_count; i++) {
                int reg_id = detail->regs_read[i];
                if (s_selected_registers.read_registers[reg_id]) {
                    const uint32_t color = s_selected_registers.read_registers_colors[reg_id];
                    const RegisterStringInfo* reg = &detail->regs_read_string_info[reg_id];
                    ImVec2 start_pos = ImVec2(op_str_start + (reg->offset * text_char_width), p.y);
                    ImVec2 end_pos = ImVec2(start_pos.x + (reg->length * text_char_width), p.y + text_height);
                    draw_list->AddRectFilled(start_pos, end_pos, IM_COL32(0, 127, 0, 127));
                }
            }

            for (int i = 0; i < detail->regs_write_count; i++) {
                int reg_id = detail->regs_write[i];
                if (s_selected_registers.write_registers[reg_id]) {
                    const uint32_t color = s_selected_registers.write_registers[reg_id];
                    const RegisterStringInfo* reg = &detail->regs_write_string_info[reg_id];
                    ImVec2 start_pos = ImVec2(op_str_start + (reg->offset * text_char_width), p.y);
                    ImVec2 end_pos = ImVec2(start_pos.x + (reg->length * text_char_width), p.y + text_height);
                    draw_list->AddRectFilled(start_pos, end_pos, IM_COL32(127, 0, 0, 127));
                }
            }

            ImGui::TableNextColumn();
        }

        ImGui::EndTable();
    }

    // printf("%08x\n", M68K_GETPC);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisassemblyView_update(DisassemblyView* self, const char* name) {
    static bool open = true;

    float text_height = ImGui::GetTextLineHeight();

    if (ImGui::Begin("Disassembly", &open, ImGuiWindowFlags_NoScrollbar)) {
        draw_disassembly(self);
    }

    ImGui::End();
}

SelectedRegisters* DisassemblyView_get_selected_registers(DisassemblyView* self) {
    return &s_selected_registers;
}
