#include "gui_manager.h"
#include <dear_imgui/imgui_internal.h>
#include <debugger/action_mgr.h>
#include <src/shortcut/shortcut_mgr.h>

namespace qd {

using namespace action;

GuiManager::GuiManager(Debugger* in_dbg) : dbg(in_dbg) {
    windows.resize((size_t)WndId::MostCommonCount);

    // create all windows
    UiViewCreate cv(this);
    auto viewMgr = UiViewClassRegistry::get();
    for (auto it : viewMgr->mClassInfoMap) {
        UiView* curView = viewMgr->makeInstance(it.first, &cv);
        addView(curView);
    }
}

GuiManager::~GuiManager() {
    assert(windows.empty());
}


void GuiManager::drawImGuiMainFrame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    wndFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    bool open = true;
    if (ImGui::Begin("Quaesar debugger", &open, wndFlags)) {
        _drawMainMenuBar();
        _drawMainToolBar();
        _drawDebuggerWindows();
    }
    ImGui::End();
}


void GuiManager::_drawMainToolBar() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("ToolBar", ImVec2(rgn.x, 20.f), ImGuiChildFlags_None, wndFlags)) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        window->DC.LayoutType = ImGuiLayoutType_Horizontal;

        Debugger* dbg = getDbg();
        ShortcutsMgr* shMgr = ShortcutsMgr::get();
        const Shortcut* pCurShortcut;
        eastl::string hint;

        bool isDbgMode = dbg->isDebugActivated();
        if (ImGui::Checkbox("Trace", &isDbgMode)) {
            dbg->setDebugMode(isDbgMode ? DebuggerMode_Break : DebuggerMode_Live);
        }
        //
        ImGui::Separator();
        //
        ImTextureID my_tex_id = io.Fonts->TexID;
        float my_tex_w = (float)io.Fonts->TexWidth;
        float my_tex_h = (float)io.Fonts->TexHeight;
        ImVec2 size, uv0, uv1;
        size = ImVec2(16.0f, 16.0f);
        uv0 = ImVec2(0.0f, 0.0f);  // TODO ICONS
        uv1 = ImVec2(uv0.x + size.x / my_tex_w, uv1.x + size.y / my_tex_h);

        if (pCurShortcut = shMgr->getShortcut(shortcut::EId::DebugTraceStepInto)) {
            if (ImGui::ImageButton("##StepInto", my_tex_id, size, uv0, uv1, ImVec4(0, 0, 0, 1))) {
                shMgr->triggerShortcut(pCurShortcut);
            }
            ImGui::SetItemTooltipV(pCurShortcut->toString().c_str(), nullptr);
        }
        //
        ImGui::Separator();
        //

        // wait scanlines
        int nScanLines = dbg->getWaitScanLines();
        ImGui::SetNextItemWidth(30);
        if (ImGui::InputInt("##skipScanLines", &nScanLines, -1, -1)) {
            if (nScanLines < 0)
                nScanLines = 1;
            dbg->setWaitScanLines(nScanLines);
        }
        hint.sprintf("Scanlines number(%s)", pCurShortcut->toString().c_str());
        ImGui::SetItemTooltipV(hint.c_str(), nullptr);
        // wait button
        ImGui::SameLine();
        pCurShortcut = shMgr->getShortcut(shortcut::EId::DebugWaitScanLines);
        if (ImGui::Button("Wait Scanlines")) {
            shMgr->triggerShortcut(pCurShortcut);
        }
    }
    ImGui::EndChild();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}


void GuiManager::_drawDebuggerWindows() {
    ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    // Debugger windows
    for (size_t i = 0; i < windows.size(); ++i) {
        UiView* curWnd = windows[i];
        if (!curWnd || !curWnd->mVisible)
            continue;
        curWnd->draw();
    }
}

void GuiManager::_drawMainMenuBar() {
    auto pActionMgr = action::ActionManager::get();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulator")) {
            ActionManager::ListByMtd actionsList = pActionMgr->getFilteredActionsByMtd(UiDrawEvent::MainMenu_Emul);
            for (action::Action* curAction : actionsList) {
                curAction->onDrawMainMenuItem(UiDrawEvent::MainMenu_Emul);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            ActionManager::ListByMtd actionsList = pActionMgr->getFilteredActionsByMtd(UiDrawEvent::MainMenu_Debug);
            for (action::Action* curAction : actionsList) {
                curAction->onDrawMainMenuItem(UiDrawEvent::MainMenu_Debug);
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
    uint32_t idx = view->mClassId;
    if (idx < (size_t)WndId::MostCommonCount) {
        assert(!windows[idx] && "already set");
        windows[idx] = view;
    } else
        windows.push_back(view);
}

};  // namespace qd
