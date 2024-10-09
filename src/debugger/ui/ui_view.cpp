#include "ui_view.h"
#include <debugger/ui/gui_manager.h>

namespace qd {

Debugger* UiView::getDbg() const {
    return ui->getDbg();
}

void UiWindow::draw() {
    bool vis = ImGui::Begin(mTitle.c_str(), &mVisible, ImGuiWindowFlags_NoScrollbar);
    if (vis)
        drawContent();
    ImGui::End();
}

namespace window {

QDB_WINDOW_REGISTER(window::ImGuiDemoWindow);

void ImGuiDemoWindow::draw() {
    ImGui::ShowDemoWindow(&mVisible);
}

};  // namespace window

};  // namespace qd
