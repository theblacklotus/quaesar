#pragma once
#include "debugger/ui/ui_view.h"

namespace qd::window {

class ColorsWnd : public UiWindow {
    QDB_CLASS_ID(WndId::Colors);

public:
    ColorsWnd(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class

};  // namespace qd::window
