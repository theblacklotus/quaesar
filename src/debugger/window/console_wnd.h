#pragma once
#include "debugger/ui/ui_view.h"

namespace qd::window {

class ConsoleWnd : public UiWindow {
    QDB_CLASS_ID(WndId::Console);
    eastl::string inputStr;

public:
    ConsoleWnd(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class

};  // namespace qd::window
