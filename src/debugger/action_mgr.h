#pragma once
#include <EASTL/bitset.h>
#include <EASTL/vector_map.h>
#include <debugger/class_reg.h>
#include <debugger/ui_defs.h>
#include <src/generic/types.h>
#include <src/shortcut/shortcut_list.h>

namespace qd {

class Debugger;
class GuiManager;

namespace action {

// Action's component classId
enum class EActionCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct ActionCreator {
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;
};

//////////////////////////////////////////////////////////////////////////
namespace msg {

template <int>
struct Base_;


struct Base {
    int mId = -1;
    template <int TClassId>
    friend struct Base_;

    template <class T>
    T* cast() {
        if (!c_def(this))
            return nullptr;
        if (mId != T::ID)
            return nullptr;
        return static_cast<T*>(this);
    }

private:
    inline Base(int msg_id) : mId(msg_id) {
    }
};  // struct msg::Base
};  // namespace msg
//////////////////////////////////////////////////////////////////////////


namespace comp {
class Base {
public:
    uint32_t mCompId = -1;
    bool mCompInit = false;
    bool mCompDone = false;

public:
    Base(action::ActionCreator* /*cp*/, uint32_t comp_id) : mCompId(comp_id){};
    virtual EFlow applyMsgProc(action::msg::Base* /*msg*/) {
        return EFlow::NO_RESULT;
    }
    virtual void compInit() {
    }
    virtual void compDestroy() {
    }
    virtual ~Base() = default;

};  // struct comp::Base
//////////////////////////////////////////////////////////////////////////

};  // namespace comp
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class Action {
public:
    eastl::fixed_vector<action::comp::Base*, 6> m_pComps;
    uint32_t mClassId = -1;
    eastl::bitset<UiDrawEvent::Count> supportMtd;
    eastl::string mName;
    eastl::string mDesc;
    GuiManager* gui = nullptr;

public:
    Action() = default;

    void onCreate(ActionCreator* cp) {
        gui = cp->gui;
        supportMtd.none();
        mName = "NO NAME";
    }

    virtual void destroy() {
    }

    virtual ~Action() {
        while (!m_pComps.empty()) {
            comp::Base* curComp = m_pComps.back();
            delete curComp;
            m_pComps.pop_back();
        }
    }

    bool hasMtd(UiDrawEvent::Type id) const {
        return supportMtd[id];
    }

    virtual void onDrawMainMenuItem(UiDrawEvent::Type event, void* = nullptr);

    void doActionBase();

    virtual EFlow applyMsgProc(action::msg::Base* msg) {
        return _applyMsgProcDefImp(msg);
    }

    decltype(auto) getCompsBegin() {
        return m_pComps.begin();
    }
    decltype(auto) getCompsEnd() {
        return m_pComps.end();
    }

    template <class TComp, typename... TArgs>
    TComp* createComp_(TArgs&&... args) {
        if (TComp* pExist = getComp_<TComp>())
            return pExist;
        action::ActionCreator cp;
        TComp* newComp = new TComp(&cp, args...);
        m_pComps.push_back(newComp);
        return newComp;
    }

    comp::Base* findComp(const uint32_t& id) const {
        auto it = eastl::find_if(m_pComps.begin(), m_pComps.end(), [id](const comp::Base* pCurComp) {
            return pCurComp ? pCurComp->mCompId == id : false;
        });
        if (it != m_pComps.end())
            return *it;
        return nullptr;
    }

    template <class TComp>
    inline TComp* getComp_() const {
        comp::Base* pComp = findComp(TComp::CLASSID);
        return static_cast<TComp*>(pComp);
    }

    Debugger* getDbg() const;

    void addShortcut(shortcut::EId sid);

protected:
    EFlow _applyMsgProcDefImp(action::msg::Base* pBaseMtd) {
        for (comp::Base* curComp : m_pComps) {
            EFlow flow = curComp->applyMsgProc(pBaseMtd);
            if (flow != EFlow::NO_RESULT)
                return flow;
        }
        return EFlow::NO_RESULT;
    }

};  // class Action
//////////////////////////////////////////////////////////////////////////


namespace details {
using ActionClassRegistry = ClassInfoRegistry_<Action>;
extern uint32_t qdbActionAutoClassId;

template <class TClass>
struct AutoRegistrator {
    AutoRegistrator(uint32_t class_id) {
        EASTL_ASSERT(class_id != 0);
        ActionClassRegistry::MetaInfo metaInfo;
        metaInfo.classId = class_id != 0 ? class_id : ++qdbActionAutoClassId;
        metaInfo.createCallback = (void*)&createClassCb;
        metaInfo.rtti = &typeid(TClass);
        ActionClassRegistry::get()->registerClass(eastl::move(metaInfo));
    }

    static Action* createClassCb(const ActionClassRegistry::MetaInfo& meta, ActionCreator* cp) {
        TClass* pInst = new TClass();
        pInst->mClassId = meta.classId;
        pInst->onCreate(cp);
        pInst->TClass::setup();
        return pInst;
    }
};  // struct AutoRegistrator
//////////////////////////////////////////////////////////////////////////

#define QDB_ACTION_REGISTER(TClass, classId) \
    static action::details::AutoRegistrator<TClass> EA_PREPROCESSOR_JOIN(_rgact_no_, __COUNTER__)((uint32_t)classId);

};  // namespace details
//////////////////////////////////////////////////////////////////////////


class ActionManager {
    typedef eastl::vector<action::Action*> ActionsList;
    eastl::vector<action::Action*> mActions;
    eastl::vector<action::Action*> mFilteredActions;
    eastl::vector_map<const std::type_info*, action::Action*> mTypeToInstance;
    bool mInit = false;

public:
    void create(GuiManager* p_gui_mgr, Debugger* p_dbg);
    void destroy();

    EFlow applyActionMsg(qd::action::msg::Base* p_msg) const;

    action::Action* findAction(uint32_t class_id) const {
        for (action::Action* pCurAction : mActions) {
            if (!pCurAction || pCurAction->mClassId != class_id)
                continue;
            return pCurAction;
        }
        return nullptr;
    }

    template <typename TClass>
    action::Action* getAction_() const {
        auto it = mTypeToInstance.find(&typeid(TClass));
        if (it == mTypeToInstance.end())
            return nullptr;
        return static_cast<TClass*>(it->second);
    }

    const ActionsList& getActions() const {
        return mActions;
    }

    friend struct ListByMtd;
    struct ListByMtd {
        const ActionManager* mpMgr;
        decltype(auto) begin() {
            return mpMgr->mFilteredActions.begin();
        }
        decltype(auto) end() {
            return mpMgr->mFilteredActions.end();
        }
    };
    ListByMtd getFilteredActionsByMtd(UiDrawEvent::Type id);

    static ActionManager* get() {
        static ActionManager instance;
        return &instance;
    }

private:
    ActionManager() = default;
    ~ActionManager() {
        assert(!mInit);
    }
};  // class ActionManager


};  // namespace action
};  // namespace qd
