#include "registers_wnd.h"
#include <debugger/debugger.h>
#include <debugger/vm.h>
#include <imgui_eastl.h>

namespace qd::window {
QDB_WINDOW_REGISTER(RegistersView);

RegistersView::RegistersView(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Registers";
}

void RegistersView::drawContent() {
    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 0);

    VM* vm = getDbg()->vm;

    int flags = /*ImGuiTableFlags_Borders |*/ ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##registers", 4, flags, ImVec2(0, 300))) {
        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("A%d", i);
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", vm->cpu->getRegA(i));
            ImGui::TableNextColumn();

            ImGui::Text("D%d", i);
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", vm->cpu->getRegD(i));
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    }
}

};  // namespace qd::window
