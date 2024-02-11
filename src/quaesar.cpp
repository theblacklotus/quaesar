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

extern void real_main(int argc, TCHAR** argv);
extern void keyboard_settrans();

// dummy main
int main(int argc, char** argv) {
    syncbase = 1000000;

    Options options;
    CLI::App app{"Quaesar"};

    Adf::create_for_exefile("test.exe");

    app.add_option("input", options.input, "Executable or image file (adf, dms)")->check(CLI::ExistingFile);
    app.add_option("-k,--kickstart", options.kickstart, "Path to the kickstart ROM")->check(CLI::ExistingFile);
    CLI11_PARSE(app, argc, argv);

    keyboard_settrans();

    default_prefs(&currprefs, true, 0);
    fixup_prefs(&currprefs, true);

    strcpy(currprefs.floppyslots[0].df, options.input.c_str());

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

    real_main(argc, argv);

    return 0;
}
