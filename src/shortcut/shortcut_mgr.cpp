#include "shortcut_mgr.h"
#include <debugger/action/comps.h>
#include <debugger/action_mgr.h>
#include <debugger/msg_list.h>
#include <generic/thread.h>
#include <src/generic/base.h>


namespace qd {


qd::Shortcut& Shortcut::addKey(ImGuiKey key) {
    mKeys.insert(key);
    return *this;
}


eastl::string Shortcut::toString() const {
    eastl::string result;
    for (auto it = mKeys.begin(); it != mKeys.end(); ++it) {
        const qd::Key& curKey = *it;
        if (it != mKeys.begin())
            result += '+';
        result += curKey.toString();
    }
    return result;
}


Shortcut::~Shortcut() {
}


ImGuiKeyChord Shortcut::getChord() const {
    ImGuiKeyChord nChord = 0;
    const Shortcut::Keys& keysList = getKeys();
    for (const Key& curKey : keysList) {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift))
            nChord |= ImGuiMod_Shift;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl))
            nChord |= ImGuiMod_Ctrl;
        else
            nChord |= curKey.getKeyCode();
    }
    return nChord;
}


bool ShortcutsMgr::isShortcutTriggered(const qd::Shortcut* p_shortcut) const {
    if (!p_shortcut)
        return false;
    const Shortcut::Keys& keysList = p_shortcut->getKeys();
    if (keysList.empty())
        return false;
    const ImGuiIO& io = ImGui::GetIO();
    size_t nKeysMatch = 0;
    for (const Key& curKey : keysList) {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift) && io.KeyShift)
            nKeysMatch++;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl) && io.KeyCtrl)
            nKeysMatch++;
        else if (ImGui::IsKeyPressed(curKey.getKeyCode(), p_shortcut->mRepeat))
            nKeysMatch++;
    }
    if (keysList.size() == nKeysMatch) {
        return true;
    }
    return false;
}


bool ShortcutsMgr::triggerShortcut(qd::shortcut::EId id) {
    const Shortcut* pShortcut = getShortcut(id);
    if (action::Action* pAction = findActionByShortcut(pShortcut)) {
        action::msg::DoAction t;
        pAction->applyMsgProc(&t);
        return true;
    }
    return false;
}


ShortcutsMgr::~ShortcutsMgr() {
    done();
}


void ShortcutsMgr::init() {
    assert(qd::thread::isMainThread());
    done();
    for (int id = 0; id < (int)shortcut::EId::MAX_COUNT; ++id) {
        Shortcut* curShortcut = shortcut::makeInstance((shortcut::EId)id);
        if (!curShortcut)
            continue;
        EASTL_ASSERT((int)curShortcut->mId == id);
        if (getShortcut((shortcut::EId)id)) {
            ASSERT_F(0, "Shortcut ID:%i already registered", id);
            continue;
        }
        mShortcuts[id] = curShortcut;
    }
}


void ShortcutsMgr::done() {
    assert(qd::thread::isMainThread());
    while (!mShortcuts.empty()) {
        auto& p = mShortcuts.back();
        delete p.second;
        mShortcuts.pop_back();
    }
}


void ShortcutsMgr::update() {
    auto pActionMgr = action::ActionManager::get();
    for (action::Action* pCurAction : pActionMgr->getActions()) {
        action::comp::ShortcutComp* pShortcuts = pCurAction->getComp_<action::comp::ShortcutComp>();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (isShortcutTriggered(curShortcut)) {
                action::msg::DoAction t;
                pCurAction->applyMsgProc(&t);
            }
        }
    }
}


const qd::Shortcut* ShortcutsMgr::getShortcut(shortcut::EId shortcut_id) const {
    auto it = mShortcuts.find((int)shortcut_id);
    if (it == mShortcuts.end())
        return nullptr;
    return it->second;
}


qd::action::Action* ShortcutsMgr::findActionByShortcut(const qd::Shortcut* pShortcut) const {
    auto pActionMgr = action::ActionManager::get();
    for (action::Action* pCurAction : pActionMgr->getActions()) {
        action::comp::ShortcutComp* pShortcuts = pCurAction->getComp_<action::comp::ShortcutComp>();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (curShortcut == pShortcut) {
                return pCurAction;
            }
        }
    }
    return nullptr;
}


};  // namespace qd
