#pragma once
#include <debugger/generic/types.h>
#include "debugger/ui/ui_view.h"

namespace qd::window {

class MemoryGraphWnd : public UiWindow {
    QDB_CLASS_ID(WndId::MemoryGraph);

    ImTextureID mTextureId = 0;
    IVec2 mTextureSize = {640, 320};
    IVec2 mPrevTextureSize = {0, 0};
    float mLastTextureCreateTime = FLT_MIN;
    AddrRef mBankOffset = 0x0;
    int mOffset = 0;
    int mCurBank = 0;

public:
    MemoryGraphWnd(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class

};  // namespace qd::window
