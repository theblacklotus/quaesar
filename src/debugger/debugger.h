#pragma once

#include <capstone/capstone.h>
#include <stdint.h>

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

namespace qd {
class GuiManager;
class VM;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DebuggerMode {
    DebuggerMode_Live,
    DebuggerMode_Break,
};

class Debugger {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    csh capstone;
    VM* vm = nullptr;
    GuiManager* gui = nullptr;

public:
    void create();
    void destroy();

    bool isDebugActivated();
    void setDebugMode(DebuggerMode debug_mode);

    void* addrToPtr(uint32_t addr);

    void applyConsoleCmd(const char* cmd);

    qd::VM* getVm() const {
        return vm;
    }
};  // class Debugger

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Debugger* Debugger_create();
void Debugger_update(Debugger* debugger);
void Debugger_update_event(SDL_Event* event);
void Debugger_destroy(Debugger* debugger);
void Debugger_toggle(Debugger* debugger, DebuggerMode mode);
bool Debugger_is_window_visible(Debugger* debugger);

};  // namespace qd
