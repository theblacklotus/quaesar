#pragma once
#include "debugger/ui/ui_view.h"

namespace qd::window {

class ScreenWnd : public UiWindow {
    QDB_CLASS_ID(WndId::Screen);

    ImTextureID mTextureId = 0;

public:
    ScreenWnd(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class

};  // namespace qd::window
