#pragma once
#include <EASTL/vector.h>
#include <debugger/debugger.h>
#include <generic/base.h>
#include <src/ui/ui_view.h>


FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, action, Action);


namespace qd {


class GuiManager {
    eastl::vector<UiView*> windows;
    Debugger* dbg = nullptr;

public:
    GuiManager(Debugger* dbg);
    ~GuiManager();

    void drawImGuiMainFrame();

    void _drawMainToolBar();
    void _drawDebuggerWindows();
    void destroy();

    Debugger* getDbg() const {
        return dbg;
    }

    template <class T>
    inline T* getWnd_() const {
        const size_t idx = T::CLASS_ID;
        UiView* curView = windows[idx];
        return static_cast<T*>(curView);
    }

    void addView(UiView* view);

private:
    void _drawMainMenuBar();

};  // class GUIManager

};  // namespace qd
