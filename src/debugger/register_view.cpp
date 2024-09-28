#include "register_view.h"
#include <capstone/m68k.h>
#include <dear_imgui/imgui.h>
#include <stdint.h>
#include "disassembly_view.h"
// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
// clang-format on

struct RegisterView {
    uint32_t dummy;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

RegisterView* RegisterView_create() {
    return new RegisterView();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void update_selected_registers(int reg_id, const SelectedRegisters* selected_registers) {
    if (selected_registers->read_registers[reg_id]) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(0, 127, 0, 127));
    } else if (selected_registers->write_registers[reg_id]) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(127, 0, 0, 127));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterView_update(RegisterView* self, const SelectedRegisters* selected_registers) {
    static bool open = true;

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

    float text_char_width = ImGui::CalcTextSize("F").x;

    if (ImGui::Begin("Registers", &open, ImGuiWindowFlags_NoScrollbar)) {
        if (ImGui::BeginTable("disassembly", 2, flags)) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (int i = 0; i < 8; i++) {
                ImGui::TableNextColumn();
                update_selected_registers(M68K_REG_A0 + i, selected_registers);
                ImGui::Text("a%d", i);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", m68k_areg(regs, i));
            }

            for (int i = 0; i < 8; i++) {
                ImGui::TableNextColumn();
                update_selected_registers(M68K_REG_D0 + i, selected_registers);
                ImGui::Text("d%d", i);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", m68k_dreg(regs, i));
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
