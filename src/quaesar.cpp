#include <stdarg.h>
#include <stdio.h>

// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "external/cli11/CLI11.hpp"
#include "parse_options.h"
#include "options.h"
#include <SDL.h>
#include "adf.h"
// clang-format on

// WTF SDL!
#undef main

#include "memory.h"
#include "reloc.h"

extern void real_main(int argc, TCHAR** argv);
extern void keyboard_settrans();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Move this somewhere else

bool ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    // Compare the end of the string with the suffix
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

Options options;

// dummy main
int main(int argc, char** argv) {
    syncbase = 1000000;

    CLI::App app{"Quaesar"};

    app.add_option("input", options.input, "Executable or image file (adf, dms)")->check(CLI::ExistingFile);
    app.add_option("-k,--kickstart", options.kickstart, "Path to the kickstart ROM")->check(CLI::ExistingFile);
    app.add_option("--serial_port", options.serial_port, "Serial port path");
    CLI11_PARSE(app, argc, argv);

    keyboard_settrans();
    default_prefs(&currprefs, true, 0);
    fixup_prefs(&currprefs, true);

    if (!options.input.empty()) {
        // TODO: cleanup
        if (ends_with(options.input.c_str(), ".exe") || !ends_with(options.input.c_str(), ".adf")) {
            Adf::create_for_exefile(options.input.c_str());
            strcpy(currprefs.floppyslots[0].df, "dummy.adf");
        } else {
            strcpy(currprefs.floppyslots[0].df, options.input.c_str());
        }
    }

    if (!options.serial_port.empty()) {
        currprefs.use_serial = 1;
        strcpy(currprefs.sername, options.serial_port.c_str());
    }

    // Most compatible mode
    currprefs.cpu_cycle_exact = 1;
    currprefs.cpu_memory_cycle_exact = 1;
    currprefs.blitter_cycle_exact = 1;
    currprefs.turbo_emulation = 0;
    currprefs.sound_stereo_separation = 0;

    strcpy(currprefs.romfile, options.kickstart.c_str());

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // TODO make this automatic and/or a cmdline arg
    struct romboard* rb = &currprefs.romboards[0];
    rb->size = 0x20000;
    rb->start_address = 0xf00000;
    rb->end_address = 0xf20000;
    strcpy(rb->lf.loadfile, "bootrom.bin");

    currprefs.start_debugger = 1;

    real_main(argc, argv);

    return 0;
}

// TODO should init this based on the actual memory map
static uint32_t chip_ptr = 0x010000;
static uint32_t fast_ptr = 0xc10000;

static void* MapToReal(APTR addr) {
    // this should use the real api
    // but first we need to get just-in-time dehunking via trap calls...
    void* p = memory_get_real_address(addr);

    uae_u8* base = 0;
    if (chipmem_bank.start <= addr && addr <= chipmem_bank.start + chipmem_bank.allocated_size)
        base = chipmem_bank.baseaddr - chipmem_bank.start;
    else if (bogomem_bank.start <= addr && addr <= bogomem_bank.start + bogomem_bank.allocated_size)
        base = bogomem_bank.baseaddr - bogomem_bank.start;
    void* ret = base + addr;
    return ret;
}

// this MUST call AllocVec for compatibility reasons
static APTR AllocAmiga(uint32_t size, uint32_t flags) {
    size += 4;  // store size
    uint32_t ret = 0;
    if (flags & (1UL << 1)) {
        ret = chip_ptr;
        chip_ptr += size;
    } else {
        ret = fast_ptr;
        fast_ptr += size;
    }
    memset(MapToReal(ret), 0x00, size);
    uint32_t* p = (uint32_t*)MapToReal(ret);
    do_put_mem_long(p, size);  // fake allocvec
    return ret + 4;
}

static uint32_t Read(void* readhandle, void* buffer, uint32_t length) {
    return fread(buffer, 1, length, (FILE*)readhandle);
}

void unpack_payload() {
    FILE* fh = fopen(options.input.c_str(), "rb");

    struct LoadSegFuncs funcs;
    funcs.read = Read;
    funcs.alloc = AllocAmiga;
    funcs.map = MapToReal;
    BPTR segList = CustomLoadSeg(fh, &funcs);

    fclose(fh);

    // uint32_t be;
    // do_put_mem_long(&be, segList);
    // uint32_t jmpAddr = (be) << 2;
    // do_put_mem_long((uae_u32*)chipmem_bank.baseaddr, jmpAddr);
    *((uae_u32*)chipmem_bank.baseaddr) = segList;
}
