# Tank Battle TUI

A terminal-based tank battle game written in C++17, powered by [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

Author: [Qiuizi](https://github.com/Qiuizi)

## Features

- Player tank with directional movement and shell firing
- Enemy tanks that move, turn, and fire automatically
- Destructible brick walls and indestructible steel walls
- HQ base defense objective
- Text-based level loading from `maps/level1.txt`
- Enemy AI pathfinding with breadth-first search (BFS)
- Three escalating waves with a capped number of active enemies
- Score, lives, wave, remaining enemies, and temporary respawn shield
- Pause, restart, victory, and game-over states

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / WASD | Move and face direction |
| Space / J | Fire shell |
| P | Pause / resume |
| R | Restart |
| Q / Esc | Quit |

## Goal

Destroy all enemy tanks before they destroy your HQ or take all of your lives. Brick walls can be shot through, steel walls block shells, and enemy tanks keep spawning until the current wave is cleared. Clear all three waves to win.

## Level File

The game loads `maps/level1.txt` at startup. If the file is missing or invalid, it falls back to the built-in map.

| Symbol | Meaning |
| --- | --- |
| `.` or space | Empty road |
| `B` | Brick wall, destroyed by shells |
| `#` | Steel wall, blocks tanks and shells |
| `H` | HQ base |
| `P` | Player spawn |
| `E` | Enemy spawn |

The board must be exactly 30 columns by 20 rows.

## AI

Enemy tanks use BFS to search the current map and choose the next step toward the player. If no path exists because walls or tanks block the route, they fall back to a simple directional chase. This keeps the implementation explainable for a C++ course project while still giving the enemies visible intelligence.

## Build

### Requirements

| Tool | Minimum Version |
| --- | --- |
| CMake | 3.20 |
| C++ compiler | C++17 (GCC 8+, Clang 7+, MSVC 2019 16.8+) |

FTXUI is fetched automatically via CMake FetchContent.

### Compile and Run

**macOS / Linux**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tank_battle
```

**Windows with MinGW**

```powershell
cmake -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="D:\mingw64\bin\g++.exe"
cmake --build build-mingw
.\build-mingw\tank_battle.exe
```

**Windows with Visual Studio / MSVC**

```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\tank_battle.exe
```

## Project Structure

```text
.
|-- CMakeLists.txt
|-- maps/
|   `-- level1.txt
|-- src/
|   |-- game.hpp
|   |-- game.cpp
|   `-- main.cpp
`-- README.md
```
