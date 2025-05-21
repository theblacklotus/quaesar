#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <debugger/vm/custom_regs_list.h>
#include <debugger/vm/memory.h>
#include <stdint.h>

namespace qd {

struct CustomFlagsDesc;


struct CustReg {
    enum Type {
#define REG_TO_ENUM(id, addr, specials, mask, desc) id,
        QDB_CUSTOM_REGS_LIST(REG_TO_ENUM) _COUNT_
#undef REG_TO_ENUM
    };
    struct Data {
        eastl::string_view name;
        uint32_t addr;
        int special;
        uint16_t mask[3];
        const char* desc;
    };
    static CustReg::Data cust_reg_data[CustReg::_COUNT_];

public:
    Type mV = CustReg::_COUNT_;

public:
    CustReg() = default;
    template <typename TInt>
    CustReg(TInt v) : mV((Type)v) {
    }
    operator uint16_t() const {
        return mV;
    }
    const eastl::string_view& toString() const {
        return CustReg::cust_reg_data[mV].name;
    }
    const char* toStringC() const {
        return toString().data();
    }
    bool isValid() const {
        return (uint32_t)mV < CustReg::_COUNT_;
    }
    static CustReg getRegByAddr(AddrRef addr);

    const CustomFlagsDesc* getFlagDesc() const;

};  // struct CustReg
//////////////////////////////////////////////////////////////////////////


struct CustomFlagsDesc {
    struct Bits {
        eastl::string_view name;
        uint8_t noBeg = 0;
        uint8_t noEnd = 0;
        uint16_t shiftL = 0x0;
        uint16_t mask = 0xFFFFu;
        eastl::string_view description;
    };
    CustReg reg;
    eastl::fixed_vector<Bits, 16, false> bits;

    CustomFlagsDesc(CustReg p_reg) : reg(p_reg) {
    }

    CustomFlagsDesc& addBit(const char* p_name, uint8_t bits_count, uint8_t shift_l, const char* p_desc = nullptr);
};  // struct CustomFlagsDesc


// BLTCON0 flags
struct BC0F {
    enum Type : uint16_t {
        ABC = 0x80,
        ABNC = 0x40,
        ANBC = 0x20,
        ANBNC = 0x10,
        NABC = 0x8,
        NABNC = 0x4,
        NANBC = 0x2,
        NANBNC = 0x1,
        DEST = 1 << 8,
        SRCC = 1 << 9,
        SRCB = 1 << 10,
        SRCA = 1 << 11,
        SHIFTA = 12,  //  bits to right align ashift value
    };
};


// BLTCON1 flags
struct BC1F {
    enum Type : uint16_t {
        LINEMODE = 1 << 0,
        BLITREVERSE = 1 << 1,
        FILLCARRYIN = 1 << 2,
        INCLUSIVEFILL = 1 << 3,
        EXCLUSIVEFILL = 1 << 4,

        SHIFTB = 12,  //  bits to right align bshift value
    };
};


// DMACON / DMACONR flags
struct DMAC {
    enum Type : uint16_t {
        AUD0EN = 1 << 0,
        AUD1EN = 1 << 1,
        AUD2EN = 1 << 2,
        AUD3EN = 1 << 3,
        DSKEN = 1 << 4,
        SPREN = 1 << 5,
        BLTEN = 1 << 6,
        COPEN = 1 << 7,
        BPLEN = 1 << 8,
        DMAEN = 1 << 9,
        BLTPRI = 1 << 10,
        BZERO = 1 << 13,  // Blitter logic zero status bit (read only)
        BBUSY = 1 << 14,  // Blitter busy status bit (read only)
        SETCLR = 1 << 15,
    };

    static const CustomFlagsDesc& getFlagDesc();

};  // struct DMAC

};  // namespace qd
