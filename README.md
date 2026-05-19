# Snake TUI

A terminal-based Snake game written in C++17, powered by [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

Author: [Qiuizi](https://github.com/Qiuizi)

## Features

- Classic snake movement on a 24 x 18 board
- Random food placement that never overlaps the snake
- Score, level, and snake length display
- Speed increases as your score rises
- Pause, restart, and quit controls
- Keyboard support for arrow keys and WASD

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / WASD | Change direction |
| P | Pause / resume |
| R | Restart |
| Q / Esc | Quit |

## Scoring

Each food item is worth `10 x current level` points. The level increases every 50 points, and the snake moves faster at higher levels.

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
./build/snake
```

**Windows (Developer Command Prompt / PowerShell)**

```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\snake.exe
```

## Project Structure

```text
.
|-- CMakeLists.txt
|-- src/
|   |-- game.hpp
|   |-- game.cpp
|   `-- main.cpp
`-- README.md
```
