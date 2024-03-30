#pragma once

#include <capstone/capstone.h>
#include <stdint.h>

#include "memory_view.h"

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;
struct DisassemblyView;
struct RegisterView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Debugger {
    SDL_Window* window;
    SDL_Renderer* renderer;
    csh capstone;
    MemoryView* memory_view;
    DisassemblyView* d_view;
    RegisterView* register_view;
};

enum DebuggerMode {
    DebuggerMode_Live,
    DebuggerMode_Break,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Debugger* Debugger_create();
void Debugger_update(Debugger* debugger);
void Debugger_update_event(SDL_Event* event);
void Debugger_destroy(Debugger* debugger);
void Debugger_toggle(Debugger* debugger, DebuggerMode mode);
void Debugger_step(Debugger* debugger);
