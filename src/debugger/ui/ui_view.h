#pragma once
#include <debugger/class_mgr.h>
#include <debugger/ui/base.h>
#include <imgui_eastl.h>

namespace qd {

class GuiManager;
class Debugger;

struct UiViewCreate {
    GuiManager* gui;
    bool visible = true;

    UiViewCreate(GuiManager* _ui) : gui(_ui) {
    }
};  // struct CreateUiViewParams

#define QDB_CLASS_ID(wnd_id)                           \
public:                                                \
    static const uint32_t CLASS_ID = (uint32_t)wnd_id; \
                                                       \
private:                                               \
    typedef UiView TSuper;

//////////////////////////////////////////////////////////////////////////
//
// Base class of all ui
//
class UiView {
public:
    eastl::string mTitle;
    bool mVisible = true;
    GuiManager* ui = nullptr;
    uint32_t classId = 0;

public:
    UiView(UiViewCreate* cp) : mVisible(cp->visible), ui(cp->gui) {
    }

protected:
    virtual void updateBeforeDraw() {
    }
    virtual void drawContent() {
    }

public:
    virtual ~UiView() {
    }

    virtual void destroy() {
    }

    virtual void draw() {
        drawContent();
    }

    Debugger* getDbg() const;

};  // class UiView
//////////////////////////////////////////////////////////////////////////

class UiWindow : public UiView {
public:
    // QDB_CLASS_ID();
    UiWindow(UiViewCreate* cp) : UiView(cp) {
    }

    virtual void draw() override;

};  // class UiWindow
//////////////////////////////////////////////////////////////////////////

namespace window {
class ImGuiDemoWindow : public UiWindow {
    QDB_CLASS_ID(WndId::ImGuiDemo);

public:
    ImGuiDemoWindow(UiViewCreate* cp) : UiWindow(cp) {
        mTitle = "ImGui Demo";
        mVisible = false;
    }

    virtual void draw() override;

};  // class
//////////////////////////////////////////////////////////////////////////

};  // namespace window

using UiViewClassMgr = ClassInfoMgr_<UiView>;

template <class TClass>
struct UiViewClassRegistrator_ {
    inline UiViewClassRegistrator_() {
        auto classMgr = UiViewClassMgr::get();
        uint32_t classId = TClass::CLASS_ID;
        classMgr->registerClass(classId, &createClassCb);
    }

    static UiView* createClassCb(UiViewCreate* cp) {
        TClass* newInst = new TClass(cp);
        newInst->classId = TClass::CLASS_ID;
        return newInst;
    }
};  // struct

#define QDB_WINDOW_REGISTER(className) static UiViewClassRegistrator_<className> EA_PREPROCESSOR_JOIN(reg, __COUNTER__);

};  // namespace qd
