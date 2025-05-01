#include "registers_wnd.h"
#include <debugger/debugger.h>
#include <debugger/vm/vm.h>
#include <imgui_eastl.h>
#include <src/ui/ui_style.h>

namespace qd {
namespace window {

#define REG_A 0x00
#define REG_D 0x08
#define REG_PC 0x10

// clang-format off
static const char* s_regLookup[] = {
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "PC",
};
// clang-format on

struct FlagDef {
    const char* name;
    const char* displayName;
    CpuFlg_ flagType;
};

// clang-format off
static const FlagDef s_flagDefs[] = {
    {"Z", "Z:", CpuFlg_Z},
    {"C", "C:", CpuFlg_C},
    {"N", "N:", CpuFlg_N},
    {"V", "V:", CpuFlg_V},
    {"X", "X:", CpuFlg_X}
};
// clang-format on

void RegistersView::drawContent() {
    Debugger* dbg = getDbg();
    VM* vm = dbg->vm;
    VM::Cpu* cpu = vm->cpu;

    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 0);

    eastl::fixed_string<char, 128, false> stVal, stCmd, stId;

    auto displayRegName = [](const char* name) {
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("%s", name);
        ImGui::PopStyleColor();
    };

    auto editRegisterValue = [&](uint32_t reg_val, const char* reg_name) {
        stVal.sprintf("%08X", reg_val);
        stId.assign("##") += reg_name;
        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegValue));
        if (ImGui::InputText(stId.c_str(), &stVal, ImGuiInputTextFlags_EnterReturnsTrue)) {
            stCmd.sprintf("r %s %s", reg_name, stVal.c_str());
            dbg->execConsoleCmd(stCmd.c_str());
        }
        ImGui::PopStyleColor();
    };

    auto editFlagValue = [&](const char* flag_name, uint32_t flag_val) {
        stVal.sprintf("%01X", flag_val);
        stId.assign("##") += flag_name;
        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
        if (ImGui::InputText(stId.c_str(), &stVal, ImGuiInputTextFlags_EnterReturnsTrue)) {
            stCmd.sprintf("r %s %s", flag_name, stVal.c_str());
            dbg->execConsoleCmd(stCmd.c_str());
        }
    };

    int flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##registers", 4, flags, ImVec2(0, 0))) {
        // Draw A and D registers side by side
        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();

            // A registers
            ImGui::TableNextColumn();
            displayRegName(s_regLookup[REG_A + i]);
            ImGui::TableNextColumn();
            editRegisterValue(cpu->getRegA(i), s_regLookup[REG_A + i]);

            // D registers
            ImGui::TableNextColumn();
            displayRegName(s_regLookup[REG_D + i]);
            ImGui::TableNextColumn();
            editRegisterValue(cpu->getRegD(i), s_regLookup[REG_D + i]);
        }

        // PC and IMASK row
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        displayRegName("PC");
        ImGui::TableNextColumn();
        editRegisterValue(cpu->getPC(), s_regLookup[REG_PC]);
        ImGui::TableNextColumn();
        displayRegName("IMASK");
        ImGui::TableNextColumn();
        ImGui::Text("%i", cpu->getIntMask());

        // CPU flags
        const int flagCount = sizeof(s_flagDefs) / sizeof(s_flagDefs[0]);
        for (int i = 0; i < flagCount; i++) {
            // Create a new row for every even index (0, 2, 4...)
            if (i % 2 == 0) {
                ImGui::TableNextRow();
            }

            ImGui::TableNextColumn();
            displayRegName(s_flagDefs[i].displayName);
            ImGui::TableNextColumn();
            editFlagValue(s_flagDefs[i].name, cpu->getFlg(s_flagDefs[i].flagType));
        }

        ImGui::EndTable();
    }
}

};  // namespace window
};  // namespace qd