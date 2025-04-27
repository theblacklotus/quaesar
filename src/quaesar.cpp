#include "quaesar.h"
#include <SDL.h>
#include <debugger/debugger.h>
#include <src/generic/thread.h>
#include <stdarg.h>
#include <stdio.h>

// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "external/cli11/CLI11.hpp"
#include "parse_options.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#ifdef WIN32
#define SDL_MAIN_NEEDED
#include <SDL_main.h>
#else
// WTF SDL2MAIN_LIBRARY - I can't to build it on MacOs
#define SDL_main main
#endif  // WIN32

qd::App* app = nullptr;

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


int uae_thread_main_func(void* /*args*/ = nullptr) {
    real_main(0, nullptr);
    return 0;
}


// Quaesar main
int SDL_main(int argc, char* argv[]) {
    syncbase = 1000000;
    app = qd::App::get();

    Options options;
    CLI::App cliApp{"Quaesar"};

    cliApp.add_option("input", options.input, "Executable or image file (adf, dms)")->check(CLI::ExistingFile);
    cliApp.add_option("-k,--kickstart", options.kickstart, "Path to the kickstart ROM")->check(CLI::ExistingFile);
    cliApp.add_option("--serial_port", options.serial_port, "Serial port path");
    CLI11_PARSE(cliApp, argc, argv);

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
    currprefs.floppy_speed = 100;
    //    currprefs.turbo_emulation = 1; // it disables sound
    currprefs.sound_stereo_separation = 0;
    currprefs.uaeboard = 1;

    strcpy(currprefs.romfile, options.kickstart.c_str());

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    atexit(&SDL_Quit);

    // start UAE in separate thread
    SDL_Thread* uae_thread_handler;
    uae_thread_handler = SDL_CreateThread(&uae_thread_main_func, "UAE emulator", nullptr);

    // wait UAE initialization
    qd::onUaeInitialized = new qd::thread::Event(true);
    qd::onUaeInitialized->wait();

    // quaesar main loop
    ::app->init();
    ::app->mainLoop();

    // quit
    SDL_Log("Waiting UAE thread over ...");
    app->mDebugger->execConsoleCmd("q");

    // wait UAE done
    SDL_WaitThread(uae_thread_handler, nullptr);
    SAFE_DELETE(qd::onUaeInitialized);

    ::app->destroy();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
namespace qd {

qd::thread::Event* onUaeInitialized = nullptr;

void App::requestToQuit() {
    mQuitRequestPosted = true;
}


bool App::hasQuitRequest() const {
    return mQuitRequestPosted;
}


void App::init() {
    createUaeWindow();
    mDebugger = Debugger::get();
    mDebugger->init();
    mDebugger->toggleWndVisible(qd::DebuggerMode_Live);
}


void App::mainLoop() {
    SDL_Event event;
    while (true) {
        if (mQuitRequestPosted) {
            ::quit_program = UAE_QUIT;
            ::currprefs.cpu_cycle_exact = 0;
            break;
        }
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT: {
                    requestToQuit();
                    break;
                }
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        // requestToQuit();
                        break;
                    } else if (event.key.keysym.sym == SDLK_F12) {
                        // activate_debugger();
                        // qd::Debugger_toggle(mDebugger, qd::DebuggerMode_Live);
                    }
                    break;
                case SDL_WINDOWEVENT: {
                    Uint8 wndEvent = event.window.event;
                    if (wndEvent == SDL_WINDOWEVENT_CLOSE) {
                        requestToQuit();
                        break;
                    }
                    break;
                }
                default:
                    break;
            }
            mDebugger->sdlEventProc(&event);
        }

        if (mDebugger->isVisible()) {
            mDebugger->update();
            mDebugger->render();
        }
        renderUaeWindow();
    }
}


void App::destroyUaeWindow() {
    SDL_DestroyTexture(mUaeScrTexture);
    mUaeScrTexture = nullptr;
    SDL_DestroyRenderer(mUaeRenderer);
    mUaeRenderer = nullptr;
    SDL_DestroyWindow(mUaeWindow);
    mUaeWindow = nullptr;
}


void App::createUaeWindow() {
    SDL_AtomicSet(&scrFrameNo, 0);

    // Create a window
    app->mUaeWindow = SDL_CreateWindow("Quaesar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mAmigaWidth, mAmigaHeight,
                                       SDL_WINDOW_RESIZABLE);

    if (!app->mUaeWindow) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return;
    }

    app->mUaeRenderer = SDL_CreateRenderer(app->mUaeWindow, -1, SDL_RENDERER_ACCELERATED);

    if (!app->mUaeRenderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(app->mUaeWindow);
        return;
    }

    mUaeScrTexture = SDL_CreateTexture(mUaeRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                       mAmigaWidth, mAmigaHeight);

    if (!mUaeScrTexture) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(mUaeRenderer);
        SDL_DestroyWindow(mUaeWindow);
        return;
    }

    mAmigaBuffer = new uint32_t[mAmigaWidth * mAmigaHeight];
}

 // Function to recreate a dynamic texture with new dimensions
void App::recreateTexture(int newWidth, int newHeight) {
    int currentWidth = 0;
    int currentHeight = 0;

    // Get the format of the old texture
    Uint32 format;
    int access;
    SDL_QueryTexture(mUaeScrTexture, &format, &access, &currentWidth, &currentHeight);

    if (newWidth == currentWidth && newHeight == currentHeight) {
        return;
    }

    // Destroy the old texture
    SDL_DestroyTexture(mUaeScrTexture);

    // Create a new texture with the desired dimensions
    mUaeScrTexture = SDL_CreateTexture(
        mUaeRenderer,
        format,
        access,  // Using the same access pattern as the original
        newWidth,
        newHeight
    );
}

void App::renderUaeWindow() {
    // render UAE texture screen
    int curFrame = SDL_AtomicGet(&scrFrameNo);
    if (curFrame == renderedFrameNo) {
        return;
    }

    renderedFrameNo = curFrame;

    int new_width = 0;
    int new_height = 0;
    int window_width, window_height;
    SDL_GetWindowSize(mUaeWindow, &window_width, &window_height);

    // Maintain aspect ratio
    float image_aspect = (float)mAmigaWidth / (float)mAmigaHeight;
    float window_aspect = (float)window_width / (float)window_height;

    if (window_aspect < image_aspect) {
        new_width = window_width;
        new_height = (int)(window_width / image_aspect);
    } else {
        new_height = window_height;
        new_width = (int)(window_height * image_aspect);
    }
    SDL_Rect rect = {(window_width - new_width) / 2, (window_height - new_height) / 2, new_width, new_height};
    SDL_RenderClear(mUaeRenderer);
    if (mUaeScrTextureMutex.tryLock()) {
        recreateTexture(mAmigaWidth, mAmigaHeight); // Recreate texture if needed
        uint32_t* texture_pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(mUaeScrTexture, NULL, (void**)&texture_pixels, &pitch) == 0) {
            for (int y = 0; y < mAmigaHeight; y++) {
                uint8_t* dest = (uint8_t*)&texture_pixels[y * mAmigaWidth];
                memcpy(dest, &mAmigaBuffer[y * mAmigaWidth], mAmigaWidth * 4);
            }
            SDL_UnlockTexture(mUaeScrTexture);
        }

        SDL_RenderCopy(mUaeRenderer, mUaeScrTexture, NULL, &rect);
        SDL_RenderPresent(mUaeRenderer);
        mUaeScrTextureMutex.unlock();
    }
}


uint32_t* App::lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    mUaeScrTextureMutex.lock();
    uint32_t* pixels = nullptr;

    if (amiga_width > mAmigaWidth || mAmigaHeight > amiga_height) {
        delete [] mAmigaBuffer;
        mAmigaBuffer = new uint32_t[mAmigaWidth * mAmigaHeight];
    }

    mAmigaWidth = amiga_width;
    mAmigaHeight = amiga_height;

    return pixels;
}


void App::unlockUaeScreenTexBuf() {
    SDL_UnlockTexture(mUaeScrTexture);
    mUaeScrTextureMutex.unlock();
    SDL_AtomicIncRef(&scrFrameNo);
}

namespace uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace uae

void App::destroy() {
    SAFE_DESTROY(mDebugger);
    destroyUaeWindow();

    qd::uae::on_app_exit_debug();
    qd::uae::on_app_exit_drawing();
}


};  // namespace qd
