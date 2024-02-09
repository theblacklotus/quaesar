# Quaesar /ˈkweɪ.zɑr/ [![ci](https://github.com/theblacklotus/quaesar/actions/workflows/ci.yml/badge.svg)]

Quaesar is an emulator based on [WinUAE](https://github.com/tonioni/WinUAE), aimed primarily at demosceners and demo developers. First off, Quaesar does not intend to replace WinUAE; it should be viewed as an alternative within a very specific niche.

So, what sets Quaesar apart?

 * Fully cross-platform: Runs on Linux, Windows, and macOS, based on the latest WinUAE code, and runs full CI on GitHub for all platforms.
 * Focuses on specific Amiga platforms only: A500(+)/A600, A1200, A1230, A1260, which are the most popular platforms for demos. Features such as graphics card support have been or will be removed.
 * The primary target is A500 512/512, which is the default configuration with accurate emulation settings.
 *  Still possible to configure options such as memory and CPU from the command line if needed.

Coming Features

* No configuration UI. Running should be as easy as `quaesar file.adf / file.exe`.
* Simple command-line options to tweak faster CPU, memory, when needed.
* Built-in debugger (both console and UI window).
* Warp mode (run the emulator as fast as possible) until user code starts running. This will enable a fast startup/shutdown cycle.
* A single press of ESC will exit the emulator.
* Much more coming later!

