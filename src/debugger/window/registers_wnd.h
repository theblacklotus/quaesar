#pragma once
#include "debugger/ui/ui_view.h"

namespace qd::window {
class RegistersView : public UiWindow {
    QDB_CLASS_ID(WndId::Registers);

public:
    RegistersView(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class

};  // namespace qd::window
