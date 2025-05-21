#pragma once
#include <EASTL/fixed_function.h>
#include <EASTL/fixed_set.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <debugger/class_reg.h>
#include <imgui.h>
#include <src/shortcut/shortcut_list.h>


FORWARD_DECLARATION_3(qd, action, Action);

namespace qd {

class Key {
public:
    ImGuiKey mKey = ImGuiKey_None;

public:
    constexpr Key() = default;
    constexpr Key(ImGuiKey key) : mKey(key) {
    }

    constexpr ImGuiKey getKeyCode() const {
        return mKey;
    }

    eastl::string toString() const {
        return ImGui::GetKeyName(mKey);
    }

    bool operator<(const Key& rh) const {
        return mKey < rh.mKey;
    }

    bool operator==(ImGuiKey key) const {
        return mKey == key;
    }
};  // class Key
//////////////////////////////////////////////////////////////////////////


class Shortcut {
public:
    shortcut::EId mId = shortcut::EId::UNDEF;
    typedef eastl::fixed_set<qd::Key, 4, false> Keys;
    Keys mKeys;
    bool mRepeat = false;

public:
    Shortcut() = default;
    ~Shortcut();
    qd::shortcut::EId getId() const {
        return mId;
    }
    const Shortcut::Keys& getKeys() const {
        return mKeys;
    }
    ImGuiKeyChord getChord() const;

    Shortcut& addKey(ImGuiKey key);
    Shortcut& setRepeat(bool val = true) {
        mRepeat = val;
        return *this;
    }

    eastl::string toString() const;

};  // class Shortcut
//////////////////////////////////////////////////////////////////////////


class ShortcutsMgr {
    eastl::vector_map<int /*shortcut::ETypeId*/, Shortcut*> mShortcuts;

public:
    static ShortcutsMgr* get() {
        static ShortcutsMgr instance;
        return &instance;
    }

    void init();
    void done();
    void update();

    const Shortcut* getShortcut(shortcut::EId shortcut_id) const;
    action::Action* findActionByShortcut(const Shortcut* pShortcut) const;

    bool isShortcutTriggered(const qd::Shortcut* shortcut) const;
    bool triggerShortcut(qd::shortcut::EId id);
    bool triggerShortcut(const qd::Shortcut* shortcut) {
        if (!shortcut)
            return false;
        return triggerShortcut(shortcut->getId());
    }

    ~ShortcutsMgr();

private:
    ShortcutsMgr() {
        init();
    }

};  // class ShortcutsManager
//////////////////////////////////////////////////////////////////////////


};  // namespace qd
