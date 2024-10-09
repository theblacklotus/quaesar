#pragma once
#include <EASTL/bitset.h>
#include <debugger/class_mgr.h>
#include <debugger/ui/base.h>

namespace qd {

class Debugger;
class GuiManager;

struct QdbActionCreate {
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//
class Action {
public:
    eastl::bitset<UiDrawEvent::Count> supportMtd;
    GuiManager* gui = nullptr;

public:
    Action(QdbActionCreate* cp) : gui(cp->gui) {
        supportMtd.none();
    }
    virtual ~Action() = default;

    bool hasMtd(UiDrawEvent::Idx id) const {
        return supportMtd[id];
    }

    virtual void onMainMenuItem(UiDrawEvent::Idx event, void* = nullptr) {
    }

    Debugger* getDbg() const;

};  // class Action
//////////////////////////////////////////////////////////////////////////

using QdbActionClassMgr = ClassInfoMgr_<Action>;
extern uint32_t qdbActionAutoClassId;

template <class TClass>
struct QdbActionClassRegistrator_ {
    inline QdbActionClassRegistrator_() {
        auto classMgr = QdbActionClassMgr::get();
        uint32_t classId = ++qdbActionAutoClassId;
        classMgr->registerClass(classId, &createClassCb);
    }

    static Action* createClassCb(QdbActionCreate* cp) {
        return new TClass(cp);
    }
};  // struct AutoRegistrator

#define QDB_ACTION_REGISTER(className) \
    static QdbActionClassRegistrator_<className> EA_PREPROCESSOR_JOIN(reg, __COUNTER__);

};  // namespace qd
