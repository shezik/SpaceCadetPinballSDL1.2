<!-- markdownlint-disable-file MD033 -->

# iriver Dicple D88 (SDL 1.2) port

*forked from [k4zmu2a/SpaceCadetPinball](https://github.com/k4zmu2a/SpaceCadetPinball)*

## Changes and optimizations

- Sprites are blitted to `vscreen` bitmap in 8 bpp indexed format, deprecating cached 32 bpp textures which is significantly slower. `vscreen` is assigned with a palette and later drawn onto the screen at the end of a frame.

- Thread sleeping is commented out in the main loop, and `UpdateToFrameRatio` is multiplied by `1.7` to reach an optimal speed of around 100 updates per second, close enough to the original 120 ups while maintaining a reasonable framerate of 30 fps. *There should be a cleaner way instead of this hack but hey it works!*

- ImGUI drawing is also disabled. It is called every frame and is way too resource intensive.

- Controller support related stuff is removed from the codebase.

## Known issues

- MIDI music playback does not work.

- Many features that require the presence of GUI are gone, e.g. multiplayer, control options, highscore dialog, demo mode, etc.

- If you re-enable ImGUI and switch language, the game crashes.

- Window resizing is buggy as hell.

- No controller support, but how'd you attach a controller to a dictionary??

- Does not build on platforms other than Linux. Partially copied `SDL_GetPrefPath` and stuff in `SDLCompatibilityLayer.cpp` are to blame.

## Tips

- If you have run the game before and have altered the default key bindings in the code since then, make sure to run the game with `-reset` command line option to reset the configuration file otherwise it would not take effect.

## How to build

- Follow [the RetroFW guide](https://github.com/retrofw/retrofw.github.io/wiki/Configuring-a-Toolchain) to build your own toolchain (painful on latest Linux distros), install it into `/opt/rs97apps` then add it to `PATH`.

- I'm doing this on Ubuntu 22.04 for WSL2, with VSCode connected to it.

- Simply run `./build-D88.sh release` or with other options to cross-compile for Dicple D88. Available options are `debug, Debug, release, Release, dbgrelease, DbgRelease`.

- If you'd like to build and run on your PC for debugging purposes, install packages `libsdl1.2-dev, libsdl-mixer1.2-dev, libsdl-ttf2.0-dev, libsdl-gfx1.2-dev` first, then run
```
./build-native clean
./build-native.sh <your option here>
```
or just use the `Run and Debug` sidebar in VSCode.

- It is expected to see an error dialog saying *property 'program' is missing or empty* if you invoke the Clean option in debug menu in VSCode.

--------------------------------

# SpaceCadetPinball

## Summary

Reverse engineering of `3D Pinball for Windows - Space Cadet`, a game bundled with Windows.

## How to play

Place compiled executable into a folder containing original game resources (not included).\
Supports data files from Windows and Full Tilt versions of the game.

## Known source ports

| Platform           | Author          | URL                                                                                                        |
| ------------------ | --------------- | ---------------------------------------------------------------------------------------------------------- |
| PS Vita            | Axiom           | <https://github.com/suicvne/SpaceCadetPinball_Vita>                                                        |
| Emscripten         | alula           | <https://github.com/alula/SpaceCadetPinball> <br> Play online: <https://alula.github.io/SpaceCadetPinball> |
| Nintendo Switch    | averne          | <https://github.com/averne/SpaceCadetPinball-NX>                                                           |
| webOS TV           | mariotaku       | <https://github.com/webosbrew/SpaceCadetPinball>                                                           |
| Android (WIP)      | Iscle           | https://github.com/Iscle/SpaceCadetPinball                                                                 |
| Nintendo Wii       | MaikelChan      | https://github.com/MaikelChan/SpaceCadetPinball                                                            |
| Nintendo 3DS       | MaikelChan      | https://github.com/MaikelChan/SpaceCadetPinball/tree/3ds                                                   |
| Nintendo Wii U     | IntriguingTiles | https://github.com/IntriguingTiles/SpaceCadetPinball-WiiU                                                  |
| MorphOS            | BeWorld         | https://www.morphos-storage.net/?id=1688897                                                                |
| AmigaOS 4          | rjd324          | http://aminet.net/package/game/actio/spacecadetpinball-aos4                                                |
| Android (WIP)      | fexed           | https://github.com/fexed/Pinball-on-Android                                                                |

Platforms covered by this project: desktop Windows, Linux and macOS.

<br>
<br>
<br>
<br>
<br>
<br>

## Source

* `pinball.exe` from `Windows XP` (SHA-1 `2A5B525E0F631BB6107639E2A69DF15986FB0D05`) and its public PDB
* `CADET.EXE` 32bit version from `Full Tilt! Pinball` (SHA-1 `3F7B5699074B83FD713657CD94671F2156DBEDC4`)

## Tools used

`Ghidra`, `Ida`, `Visual Studio`

## What was done

* All structures were populated, globals and locals named.
* All subs were decompiled, C pseudo code was converted to compilable C++. Loose (namespace?) subs were assigned to classes.

## Compiling

Project uses `C++11` and depends on `SDL2` libs.

### On Windows

Download and unpack devel packages for `SDL2` and `SDL2_mixer`.\
Set paths to them in `CMakeLists.txt`, see suggested placement in `/Libs`.\
Compile with Visual Studio; tested with 2019.

### On Linux

Install devel packages for `SDL2` and `SDL2_mixer`.\
Compile with CMake; tested with GCC 10, Clang 11.\
To cross-compile for Windows, install a 64-bit version of mingw and its `SDL2` and `SDL2_mixer` distributions, then use the `mingwcc.cmake` toolchain.

[![Packaging status](https://repology.org/badge/tiny-repos/spacecadetpinball.svg)](https://repology.org/project/spacecadetpinball/versions) 

Some distributions provide a package in their repository. You can use those for easier dependency management and updates.

This project is available as Flatpak on [Flathub](https://flathub.org/apps/details/com.github.k4zmu2a.spacecadetpinball).

### On macOS

Install XCode (or at least Xcode Command Line Tools with `xcode-select --install`) and CMake.

**Manual compilation:**

* **Homebrew**: Install the `SDL2`, `SDL2_mixer` homebrew packages.
* **MacPorts**: Install the `libSDL2`, `libSDL2_mixer` macports packages.

Compile with CMake. Ensure that `CMAKE_OSX_ARCHITECTURES` variable is set for either `x86_64` Apple Intel or `arm64` for Apple Silicon.

Tested with: macOS Big Sur (Intel) with Xcode 13 & macOS Montery Beta (Apple Silicon) with Xcode 13.

**Automated compilation:**

Run the `build-mac-app.sh` script from the root of the repository. The app will be available in a DMG file named `SpaceCadetPinball-<version>-mac.dmg`.

Tested with: macOS Ventura (Apple Silicon) with Xcode Command Line Tools 14 & macOS Big Sur on GitHub Runner (Intel) with XCode 13.

## Plans

* ~~Decompile original game~~
* ~~Resizable window, scaled graphics~~
* ~~Loader for high-res sprites from CADET.DAT~~
* ~~Cross-platform port using SDL2, SDL2_mixer, ImGui~~
* Full Tilt Cadet features
* Localization support
* Maybe: Support for the other two tables - Dragon and Pirate
* Maybe: Game data editor

## On 64-bit bug that killed the game

I did not find it, decompiled game worked in x64 mode on the first try.\
It was either lost in decompilation or introduced in x64 port/not present in x86 build.\
Based on public description of the bug (no ball collision), I guess that the bug was in `TEdgeManager::TestGridBox`
