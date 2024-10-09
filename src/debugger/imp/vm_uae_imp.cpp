#include "vm_uae_imp.h"
// clang-format off
#include <src/sysconfig.h>
//#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
// clang-format on

extern bool get_custom_color_reg(int colreg, uae_u8* r, uae_u8* g, uae_u8* b);
extern uaecptr bplpt[MAX_PLANES], bplptx[MAX_PLANES];

namespace qd::vm::imp {

UaeEmuVM::UaeEmuVM() {
    uint32_t hiAddr = 0;
    while (hiAddr < MEMORY_BANKS) {
        addrbank* uaeBank = mem_banks[hiAddr];
        if (!uaeBank->allocated_size) {
            ++hiAddr;
            continue;
        }

        bool combined = false;
        for (MemBank& existBank : mem->banks) {
            if (existBank.realAddr == uaeBank->baseaddr) {
                combined = true;
                hiAddr += uaeBank->allocated_size >> 16;
                break;
            }
        }
        if (combined)
            continue;

        MemBank& memBank = mem->banks.emplace_back();
        memBank.id = (int)mem->banks.size() - 1;
        memBank.name = uaeBank->name;
        memBank.label = uaeBank->label;
        memBank.startAddr = uaeBank->start;
        memBank.realAddr = uaeBank->baseaddr;
        memBank.mask = uaeBank->mask;
        memBank.size = uaeBank->allocated_size;
        hiAddr += memBank.size >> 16;
    }
}

bool UaeEmuVM::Blitter::getColor(int n_col, Color& out_color) {
    bool res = ::get_custom_color_reg(n_col, &out_color.r, &out_color.g, &out_color.b);
    return res;
}

void* UaeEmuVM::Blitter::getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) {
    vidbuf_description* vidinfo = &adisplays[mon_id].gfxvidinfo;
    vidbuffer* vb = &vidinfo->drawbuffer;
    if (!vb || !vb->bufmem)
        return nullptr;
    *out_size_w = vb->outwidth;
    *out_size_h = vb->outheight;
    *pitch = vb->rowbytes;
    return vb->bufmem;
}

};  // namespace qd::vm::imp
