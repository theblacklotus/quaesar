#pragma once
#include <EASTL/span.h>
#include <debugger/action_mgr.h>
#include <src/shortcut/shortcut_mgr.h>


namespace qd {
namespace action {
namespace comp {

class ShortcutComp : public comp::Base {
public:
    static constexpr uint32_t CLASSID = (uint32_t)EActionCompsClassId::Shortcuts;
    eastl::fixed_vector<const Shortcut*, 2> mShortcuts;

public:
    ShortcutComp(action::ActionCreator* cp) : comp::Base(cp, CLASSID) {
    }
    virtual ~ShortcutComp() {
    }

    eastl::span<const Shortcut* const> getShortcuts() const {
        const Shortcut* const* ptrBeg = mShortcuts.data();
        eastl::span<const Shortcut* const> sp(ptrBeg, mShortcuts.size());
        return sp;
    }

    void addShortcut(const Shortcut* sh) {
        EASTL_ASSERT(sh);
        if (!sh)
            return;
        mShortcuts.push_back(sh);
    }

    const Shortcut* getShortcut(int ind) const {
        if ((size_t)ind >= mShortcuts.size())
            return nullptr;
        return mShortcuts[ind];
    }

    int getNumShortcuts() const {
        return (int)mShortcuts.size();
    }

};  // class Shortcuts
};  // namespace comp
};  // namespace action
};  // namespace qd


inline void qd::action::Action::addShortcut(shortcut::EId sid) {
    auto pMgr = qd::ShortcutsMgr::get();
    const Shortcut* pShortcut = pMgr->getShortcut(sid);
    EASTL_ASSERT(pShortcut);
    auto pShortComp = createComp_<qd::action::comp::ShortcutComp>();
    pShortComp->addShortcut(pShortcut);
}
