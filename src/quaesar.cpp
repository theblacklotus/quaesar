#include <stdio.h>
#include <stdarg.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "external/cli11/CLI11.hpp"
#include "parse_options.h"
#include "options.h"
#include <SDL.h>

// WTF SDL!
#undef main

extern void real_main(int argc, TCHAR** argv);
extern void keyboard_settrans();

// dummy main
int main(int argc, char** argv) {
    syncbase = 1000000;

    Options options;
    CLI::App app {"Quaesar"};

    app.add_option("input", options.input, "Executable or image file (adf, dms)")->check(CLI::ExistingFile);
    app.add_option("-k,--kickstart", options.kickstart, "Path to the kickstart ROM")->check(CLI::ExistingFile);
    CLI11_PARSE(app, argc, argv);

    strcpy(changed_prefs.romfile, options.kickstart.c_str());

    printf("input: %s\n", options.input.c_str());

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

    keyboard_settrans();
    real_main(argc, argv);

	return 0;
}

