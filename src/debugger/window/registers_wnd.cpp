#include "registers_wnd.h"
#include <debugger/debugger.h>
#include <debugger/vm/vm.h>
#include <imgui_eastl.h>
#include <src/ui/ui_style.h>

namespace qd {
namespace window {


void RegistersView::drawContent() {
    Debugger* dbg = getDbg();
    VM* vm = dbg->vm;
    VM::Cpu* cpu = vm->cpu;

    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 0);

    eastl::fixed_string<char, 128, false> stReg, stVal, stCmd, stId;

    auto editCommonRegVal = [&](uint32_t reg_val) {
        stVal.sprintf("%08X", reg_val);
        stId.assign("##") += stReg;
        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegValue));
        if (ImGui::InputText(stId.c_str(), &stVal, ImGuiInputTextFlags_EnterReturnsTrue)) {
            stCmd.sprintf("r %s %s", stReg.c_str(), stVal.c_str());
            dbg->execConsoleCmd(stCmd.c_str());
        }
        ImGui::PopStyleColor();
    };

    int flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##registers", 4, flags, ImVec2(0, 0))) {
        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Ax col
            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
            ImGui::Text("A%d", i);
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            editCommonRegVal(cpu->getRegA(i));
            ImGui::TableNextColumn();

            // Dx col
            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
            ImGui::Text("D%d", i);
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            editCommonRegVal(cpu->getRegD(i));
            // ImGui::TableNextColumn();
        }

        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // PC
            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
            ImGui::Text("PC");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            editCommonRegVal(cpu->getPC());
            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
            ImGui::Text("IMASK");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            ImGui::Text("%i", cpu->getIntMask());
            ImGui::TableNextColumn();
        }

        //
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        auto editFlagsRegVal = [&](const char* flg_name, uint32_t reg_val) {
            stVal.sprintf("%01X", reg_val);
            stId.assign("##") += flg_name;
            ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
            if (ImGui::InputText(stId.c_str(), &stVal, ImGuiInputTextFlags_EnterReturnsTrue)) {
                stCmd.sprintf("r %s %s", flg_name, stVal.c_str());
                dbg->execConsoleCmd(stCmd.c_str());
            }
        };

        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("Z:");
        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        editFlagsRegVal("Z", cpu->getFlg(CpuFlg_Z));
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("C:");
        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        editFlagsRegVal("C", cpu->getFlg(CpuFlg_C));
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("N:");
        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        editFlagsRegVal("N", cpu->getFlg(CpuFlg_N));
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("V:");
        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        editFlagsRegVal("V", cpu->getFlg(CpuFlg_V));
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::Text("X:");
        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        editFlagsRegVal("X", cpu->getFlg(CpuFlg_X));
        ImGui::TableNextColumn();

        ImGui::EndTable();
    }
}

};  // namespace window
};  // namespace qd
