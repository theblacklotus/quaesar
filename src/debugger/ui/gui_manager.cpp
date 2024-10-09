#include "gui_manager.h"
#include <debugger/action_mgr.h>

namespace qd {

GuiManager::GuiManager(Debugger* in_dbg) : dbg(in_dbg) {
    windows.resize((size_t)WndId::MostCommonCount);

    // create all windows
    UiViewCreate cv(this);
    auto viewMgr = UiViewClassMgr::get();
    for (auto it : viewMgr->m_classInfoMap) {
        UiView* curView = viewMgr->makeInstance(it.first, &cv);
        addView(curView);
    }

    // crete all actions
    QdbActionCreate ca = {this, dbg};
    auto actionMgr = QdbActionClassMgr::get();
    for (auto it : actionMgr->m_classInfoMap) {
        Action* curAction = actionMgr->makeInstance(it.first, &ca);
        actions.push_back(curAction);
    }
}

GuiManager::~GuiManager() {
    while (actions.empty()) {
        delete actions.back();
        actions.pop_back();
    }
    assert(windows.empty());
}

void GuiManager::drawImGuiFrame() {
    _drawMainMenuBar();

    for (size_t i = 0; i < windows.size(); ++i) {
        UiView* curWnd = windows[i];
        if (!curWnd || !curWnd->mVisible)
            continue;
        curWnd->draw();
    }
}

void GuiManager::_drawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save dump")) {
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            for (Action* curAction : actions) {
                if (!curAction->hasMtd(UiDrawEvent::MainMenu_Debug))
                    continue;
                curAction->onMainMenuItem(UiDrawEvent::MainMenu_Debug);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            for (UiView* curWnd : windows) {
                if (!curWnd)
                    continue;
                ImGui::MenuItem(curWnd->mTitle.c_str(), 0, &curWnd->mVisible);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void GuiManager::destroy() {
    while (!windows.empty()) {
        UiView* curWnd = windows.back();
        windows.pop_back();
        curWnd->destroy();
        delete curWnd;
    }
}

void GuiManager::addView(UiView* view) {
    uint32_t idx = view->classId;
    if (idx < (size_t)WndId::MostCommonCount) {
        assert(!windows[idx] && "already set");
        windows[idx] = view;
    } else
        windows.push_back(view);
}

};  // namespace qd
