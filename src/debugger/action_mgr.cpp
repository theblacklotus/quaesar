#include "action_mgr.h"
#include <debugger/action/comps.h>
#include <debugger/msg_list.h>
#include <debugger/ui_defs.h>
#include <src/ui/gui_manager.h>


namespace qd {
namespace action {
uint32_t details::qdbActionAutoClassId = 0;


Debugger* Action::getDbg() const {
    return gui->getDbg();
}


void Action::onDrawMainMenuItem(UiDrawEvent::Type event, void* /*= nullptr*/) {
    Action* action = this;
    auto pShortcuts = action->getComp_<comp::ShortcutComp>();
    eastl::string shortcutName;
    if (pShortcuts && pShortcuts->getNumShortcuts() > 0) {
        const Shortcut* pSh = pShortcuts->getShortcut(0);
        shortcutName = pSh->toString();
    }

    bool bSelected = false;
    if (action->supportMtd[UiDrawEvent::MenuItemStateChecked]) {
        action::msg::MenuItemStateGet menuState;
        menuState.menuType = event;
        action->applyMsgProc(&menuState);
        bSelected = menuState.checked > 0 ? menuState.checked != 0 : false;
    }

    bool enabled = true;
    if (ImGui::MenuItem(mName.c_str(), shortcutName.c_str(), &bSelected, enabled)) {
        action::msg::DoAction msg;
        action->applyMsgProc(&msg);
    }
}


void Action::doActionBase() {
    action::msg::DoAction ms;
    applyMsgProc(&ms);
}


//////////////////////////////////////////////////////////////////////////

void ActionManager::create(GuiManager* pGuiMgr, Debugger* pDbg) {
    EASTL_ASSERT(!mInit);
    // create all actions
    action::ActionCreator ca = {pGuiMgr, pDbg};
    auto actionClassMgr = qd::action::details::ActionClassRegistry::get();
    for (auto it : actionClassMgr->mClassInfoMap) {
        action::Action* curAction = actionClassMgr->makeInstance(it.first, &ca);
        mActions.push_back(curAction);
        const qd::action::details::ActionClassRegistry::MetaInfo& meta = it.second;
        if (meta.rtti)
            mTypeToInstance[meta.rtti] = curAction;
    }
    mInit = true;
}


void ActionManager::destroy() {
    mInit = false;
    while (!mActions.empty()) {
        delete mActions.back();
        mActions.pop_back();
    }
}


ActionManager::ListByMtd ActionManager::getFilteredActionsByMtd(UiDrawEvent::Type id) {
    mFilteredActions.clear();
    for (Action* curAction : mActions) {
        if (!curAction->hasMtd(id))
            continue;
        mFilteredActions.push_back(curAction);
    }
    ListByMtd r;
    r.mpMgr = this;
    return r;
}


EFlow ActionManager::applyActionMsg(qd::action::msg::Base* p_msg) const {
    for (action::Action* pCurAction : mActions) {
        if (!pCurAction)
            continue;
        EFlow r = pCurAction->applyMsgProc(p_msg);
        if (r != EFlow::NO_RESULT)
            return r;
    }
    return EFlow::NO_RESULT;
}


};  // namespace action
};  // namespace qd
