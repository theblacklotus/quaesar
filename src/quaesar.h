#pragma once
#include <SDL_atomic.h>
#include <src/generic/base.h>
#include <src/generic/thread.h>

FORWARD_DECLARATION_3(qd, thread, Event);
FORWARD_DECLARATION_2(qd, Debugger);
struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;


namespace qd {
extern qd::thread::Event* onUaeInitialized;

class App {
    SDL_Texture* mUaeScrTexture = nullptr;
    qd::thread::Mutex mUaeScrTextureMutex;
    SDL_atomic_t scrFrameNo = {};
    int renderedFrameNo = -1;
    bool mQuitRequestPosted = false;
    int mAmigaWidth = 754;
    int mAmigaHeight = 576;
    uint32_t* mAmigaBuffer = nullptr;

public:
    qd::Debugger* mDebugger = nullptr;
    SDL_Window* mUaeWindow = nullptr;
    SDL_Renderer* mUaeRenderer = nullptr;

public:
    void init();
    void mainLoop();
    void destroy();
    void requestToQuit();
    bool hasQuitRequest() const;

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();

    qd::Debugger* getDbg() const {
        return mDebugger;
    }

    static App* get() {
        static App inst;
        return &inst;
    }

private:
    void createUaeWindow();
    void renderUaeWindow();
    void destroyUaeWindow();
    void recreateTexture(int newWidth, int newHeight);


};  // class App
//////////////////////////////////////////////////////////////////////////


namespace uae {
extern void do_console_cmd_immediate(const char* cmd);
};

};  // namespace qd

extern qd::App* app;
