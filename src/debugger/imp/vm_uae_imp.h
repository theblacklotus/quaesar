#pragma once
// clang-format off
#include <src/sysconfig.h>
#include <uae_src/include/sysdeps.h>
#include <uae_src/include/options.h>
#include <uae_src/include/memory.h>
#include <uae_src/include/newcpu.h>
#include <uae_src/include/debug.h>
// clang-format on
#include <EASTL/span.h>
#include <EASTL/vector.h>
#include <debugger/generic/color.h>
#include <debugger/generic/memory.h>
#include <debugger/generic/types.h>

namespace qd::vm::imp {

class UaeEmuVM {
    constexpr static int amiga_width = 754;
    constexpr static int amiga_height = 576;

public:
    UaeEmuVM();

    static int getScreenSizeX() {
        return amiga_width;
    }
    static int getScreenSizeY() {
        return amiga_height;
    }

    struct Cpu {
        static uint32_t getRegA(int i) {
            return m68k_areg(::regs, i);
        }
        static uint32_t getRegD(int i) {
            return m68k_dreg(regs, i);
        }

        static AddrRef getPC() {
            return m68k_getpc();
        }
    } cpuInst;  // struct Cpu
    Cpu* cpu = &cpuInst;

    ///
    struct Memory {
        eastl::vector<qd::MemBank> banks;

        const qd::MemBank* getBankByInd(int ind) const {
            if (ind < banks.size())
                return &banks[ind];
            return nullptr;
        }
        eastl::span<const qd::MemBank> getBanks() const {
            return banks;
        }

        static void* getRealAddr(AddrRef ptr) {
            return memory_get_real_address(ptr);
        }
        static uint16_t getU16(AddrRef addr) {
            return (uint16_t)memory_get_word(addr);
        }
        static void setU16(AddrRef addr, uint16_t v) {
            memory_put_word(addr, v);
        }
        static uint32_t getU32(AddrRef addr) {
            return (uint32_t)memory_get_long(addr);
        }
        static void setU32(AddrRef addr, uint32_t v) {
            memory_put_long(addr, v);
        }

    } memInst;  // struct Memory
    Memory* mem = &memInst;

    ///
    struct Blitter {
        static bool getColor(int n_col, Color& out_color);
        void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch);
    } blitterInst;

    Blitter* blitter = &blitterInst;

};  // class UaeEmuVM
//////////////////////////////////////////////////////////////////////////

};  // namespace qd::vm::imp
