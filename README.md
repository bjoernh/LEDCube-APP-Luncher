# Cube 2.0 App Launcher

A desktop-simulated app launcher for a 64×64 LED-matrix device, built with
[LVGL v9.2](https://lvgl.io/) and SDL2. The launcher renders a 64×64 logical
framebuffer (RGB565) into an upscaled SDL window so the UI can be iterated on
a desktop before being deployed to embedded hardware.

```
┌────────────────────┐   0
│   MACBOOKAIR-M1    │   6 px title (blue, scrolls if too long)
├────────────────────┤
│       SNAKE        │
│    BREAKOUT3D      │   52 px list — 5 of 7 items visible,
│     PIXELFLOW      │   focused row cycles HSV hue 0→360° / 4 s
│    MATRIXRAIN      │
│     VOXELSAND      │
├────────────────────┤
│             100%   │   6 px info (battery, red)
└────────────────────┘   64
```

## Features

- **64×64 RGB565 framebuffer** with nearest-neighbour upscale (default 4×, 256×256 window).
- **Custom 4×6 bitmap font** (`lv_font_cube_6px`) generated from `tools/6px_font.h`.
- **Hue-cycling focus indicator** at ~30 Hz over a 4 s rainbow period.
- **Sliding screen transitions** (300 ms) between the launcher and app screens.
- **Keyboard input** mapped to LVGL keypad events (arrows / WASD / Enter / Esc).
- **Strict platform separation** — the UI/logic layer has zero platform dependencies,
  so the same code can later run on iOS or real cube hardware.

## Requirements

- **CMake** ≥ 3.16
- **C++20** compiler (Clang or GCC)
- **SDL2** — `brew install sdl2` on macOS, `apt install libsdl2-dev` on Debian/Ubuntu
- **Git** (for the LVGL submodule)

## Quick start

```bash
git clone --recurse-submodules <repo-url>
cd app_luncher_lvgl

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

./build/apps/desktop/cube_launcher
```

If you cloned without `--recurse-submodules`, run
`git submodule update --init --recursive` before building.

## Controls

| Key                     | Action                            |
| ----------------------- | --------------------------------- |
| `↑` / `↓` / `W` / `S`   | Move focus through the app list   |
| `Enter`                 | Open the focused app              |
| `Esc`                   | Return to the launcher            |
| Window close / `⌘Q`     | Quit                              |

## Project layout

```
apps/desktop/      cube_launcher executable (tiny glue around SdlPlatform + launcher)
shared/            cube_shared static lib  — all UI/logic, no platform headers
platform/sdl/      cube_platform_sdl       — SDL2 backend (display, input, sysinfo)
lib/lvgl/          LVGL v9.2 (git submodule)
tests/             Catch2 unit tests (opt-in)
tools/             Bitmap-font source + LVGL font generator script
```

The dependency rule is one-way: `apps/desktop` → `platform/sdl` → `shared` → `lvgl`.
`shared/` must never include SDL, POSIX, or any other platform header.

## Customizing

### Tune layout, colors, or timing
All visual constants live in [`shared/include/cube/config.hpp`](shared/include/cube/config.hpp):
screen size, row heights, title/info/text/background colors, animation periods.

### Change the SDL window scale
Edit `kScale` in [`platform/sdl/src/sdl_internal.hpp`](platform/sdl/src/sdl_internal.hpp).
The default `4` produces a 256×256 window from the 64×64 framebuffer.

### Add or rename apps in the launcher
Edit the `kApps[]` array in `shared/include/cube/config.hpp`. The list renders
automatically; selecting an entry currently opens a placeholder
("ESC TO EXIT") via `mock_app_open()`.

### Add a new platform backend
Create `platform/<name>/` with a CMakeLists.txt and a class implementing
`cube::IPlatform` (see [`shared/include/cube/platform.hpp`](shared/include/cube/platform.hpp)).
The shared library does not need to change.

### Regenerate the bitmap font
After editing `tools/6px_font.h`:

```bash
python3 tools/gen_font.py > shared/src/lv_font_cube_6px.c
```

The generated `.c` is committed so the build never needs Python.

## Tests

Tests are opt-in and pull Catch2 v3 via CMake `FetchContent`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Current coverage: HSV→RGB color conversion and hostname normalization.

## Architecture overview

Three CMake targets, layered top-down:

| Layer                   | Target                | Knows about               |
| ----------------------- | --------------------- | ------------------------- |
| `apps/desktop/`         | `cube_launcher` exe   | shared + sdl backend      |
| `platform/sdl/`         | `cube_platform_sdl`   | shared + LVGL + SDL2      |
| `shared/`               | `cube_shared`         | LVGL only — no OS headers |
| `lib/lvgl/` (submodule) | `lvgl`                | —                         |

Key interfaces:

- `cube::IPlatform` — abstract base in `shared/include/cube/platform.hpp`,
  exposing the LVGL display/keypad, an event pump, sleep, and system info
  (hostname, battery). `SdlPlatform` is the only concrete implementation today.
- `cube::launcher_init(IPlatform&)` — builds the title/list/info screen and
  attaches it to the platform's LVGL display + keypad group.

For deeper architectural notes (display pipeline, input routing, hue-cycle
timer), see [AGENTS.md](AGENTS.md).
