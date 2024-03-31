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

#include "autoconf.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"
#include "threaddep/thread.h"

extern void real_main(int argc, TCHAR** argv);
extern void keyboard_settrans();

#include "native2amiga.h"

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
    rb->size = 0x10000;
    rb->start_address = 0xf10000;
    rb->end_address = 0xf20000;
    strcpy(rb->lf.loadfile, "bootrom.bin");

    currprefs.start_debugger = 1;
    currprefs.uaeboard = 1;

    real_main(argc, argv);

    return 0;
}

TrapContext* currentContext = 0;

static void* MapToReal(APTR addr) {
    return memory_get_real_address(addr);
}

static APTR AllocAmiga(uint32_t size, uint32_t flags) {
    size += 4;  // add space to store size

    TrapContext* ctx = currentContext;
    uaecptr ret = uae_AllocMem(ctx, size + 4, flags, trap_get_long(ctx, 4));

    memset(MapToReal(ret), 0x00, size);
    uint32_t* p = (uint32_t*)MapToReal(ret);
    do_put_mem_long(p, size);  // fake allocvec

    return ret + 4;
}

static uint32_t Read(void* readhandle, void* buffer, uint32_t length) {
    return fread(buffer, 1, length, (FILE*)readhandle);
}

uae_u32 REGPARAM2 dehunk_payload(TrapContext* ctx) {
    FILE* fh = fopen(options.input.c_str(), "rb");

    if (!fh)
        return 0;

    currentContext = ctx;

    struct LoadSegFuncs funcs;
    funcs.read = Read;
    funcs.alloc = AllocAmiga;
    funcs.map = MapToReal;
    BPTR segList = CustomLoadSeg(fh, &funcs);

    currentContext = 0;

    fclose(fh);

    return do_byteswap_32(segList);
}
