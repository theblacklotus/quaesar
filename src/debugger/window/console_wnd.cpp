#include "console_wnd.h"
#include <debugger/debugger.h>
#include <imgui_eastl.h>

namespace qd::window {

QDB_WINDOW_REGISTER(ConsoleWnd);

ConsoleWnd::ConsoleWnd(UiViewCreate* cp) : UiWindow(cp) {
    mTitle = "Console";
}

void ConsoleWnd::drawContent() {
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("> ", &inputStr)) {
        getDbg()->applyConsoleCmd(inputStr.c_str());
    }
}

};  // namespace qd::window
