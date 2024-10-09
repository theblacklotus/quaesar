#pragma once
#include <debugger/generic/types.h>
#include <debugger/imp/vm_uae_imp.h>

namespace qd {

//////////////////////////////////////////////////////////////////////////
class VM : public vm::imp::UaeEmuVM {
public:
    static VM* get() {
        static VM instance;
        return &instance;
    }
};  // class VM
};  // namespace qd
