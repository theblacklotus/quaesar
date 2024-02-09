# Quaesar /ˈkweɪ.zɑr/ ![ci](https://github.com/theblacklotus/quaesar/actions/workflows/ci.yml/badge.svg)

<img src="https://raw.githubusercontent.com/theblacklotus/quaesar/main/bin/quaesar.png">

Quaesar is an emulator based on [WinUAE](https://github.com/tonioni/WinUAE), aimed primarily at demosceners and demo developers. First off, Quaesar does not intend to replace WinUAE; it should be viewed as an alternative within a very specific niche.

So, what sets Quaesar apart?

 * Fully cross-platform: Runs on Linux, Windows, and macOS, based on the latest WinUAE code, and runs full CI on GitHub for all platforms.
 * Focuses on specific Amiga platforms only: A500(+)/A600, A1200, A1230, A1260, which are the most popular platforms for demos. Features such as graphics card support have been or will be removed.
 * The primary target is A500 512/512, which is the default configuration with accurate emulation settings.
 *  Still possible to configure options such as memory and CPU from the command line if needed.

## 🔍 Status

It's very early days for the emulator. Currently it starts up and can display the kickstart screen, but not much can be done currently. The next steps are to implement basic commandline parsing so kickstart, floppy image, etc, can be passed in.

## ✨ Coming features

* No configuration UI. Running should be as easy as `quaesar file.adf / file.exe`.
* Simple command-line options to tweak faster CPU, memory, when needed.
* Built-in debugger (both console and UI window).
* Warp mode (run the emulator as fast as possible) until user code starts running. This will enable a fast startup/shutdown cycle.
* A single press of ESC will exit the emulator.
* Much more coming later!

## 🏗️ Build

Quaesar uses [CMake](https://cmake.org) to configure the build step. See each platform below for the required steps needed.

### Windows

Visual Studio (with the Windows SDK) needs to be installed and the Community edition works fine. Currently Visual Studio 2019 and 2022 is being tested, but older versions may work also.

1. Run or double click `scripts\open_vs_solution.cmd`
2. Build the solution and start it as with any other program. 

### Linux 

Linux version depends on CMake and SDL2. Each distro has various ways on installing, but this is how Ubuntu/Debian would do it. Ninja build here is optional, but recommended.

```
apt-get install libsdl2-dev cmake ninja-build
```

To build (using Ninja)

```
mkdir output && cd output && cmake .. -G Ninja && ninja
```

To build (using make)

```
mkdir output && cd output && cmake .. && make -j$(nproc)
```

### macOS

The steps for macOS are identical to Linux, except you usually use [Homebrew](https://brew.sh) to install packages. 

```
brew install sdl2 cmake ninja
```

