#pragma once
#include <EASTL/optional.h>
#include "debugger/ui/ui_view.h"

namespace qd::window {

class DisassemblyView : public UiWindow {
    QDB_CLASS_ID(WndId::Disassembly);
    eastl::string addrInputStr;
    eastl::optional<uint32_t> disasmAddr;

public:
    DisassemblyView(UiViewCreate* cp);

    virtual void drawContent() override;

};  // class DisassemblyView

};  // namespace qd::window
