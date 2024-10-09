#pragma once

enum class WndId {
    MemoryView,
    Disassembly,
    Registers,
    Console,
    Screen,
    Colors,
    MemoryGraph,

    ImGuiDemo,
    MostCommonCount,
};

namespace UiDrawEvent {
enum Idx {
    MainMenu_File,
    MainMenu_Debug,

    Count,
};
};  // namespace UiDrawEvent
