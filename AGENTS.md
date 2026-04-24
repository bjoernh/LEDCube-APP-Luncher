# AGENTS.md

This file provides guidance to Coding Agents when working with code in this repository.

## Build

```bash
# Configure (first time or after CMakeLists changes)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j

# Run
./build/apps/desktop/cube_launcher
```

Requires SDL2 installed (`brew install sdl2` on macOS).

## Tests

Tests are **opt-in** — not built by default.

```bash
# Configure with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# Build and run
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Uses Catch2 v3 (fetched automatically via FetchContent). Test sources are in `tests/`.

## Verify UI changes

Use the `/verify-with-screenshot-macos` skill to rebuild and screenshot the running window automatically.

## Architecture

Three layers, each a separate CMake target:

```
apps/desktop/          → cube_launcher executable (tiny glue only)
shared/                → cube_shared static lib   (all UI/logic, no platform deps)
platform/sdl/          → cube_platform_sdl static lib (SDL2 backend)
lib/lvgl/              → lvgl submodule (v9.2)
```

**Dependency rule:** `shared/` must never include SDL, POSIX, or any platform header. All platform-specific code lives in `platform/sdl/`.

### Key interfaces

- `shared/include/cube/platform.hpp` — `IPlatform` abstract base; `SdlPlatform` in `platform/sdl/` is the only concrete implementation.
- `shared/include/cube/config.hpp` — all layout constants (`kScreenW/H = 64`, row heights, colors, animation timing) and the app list (`kApps[]`). **This is the single place to tune the UI.**
- `shared/include/lv_conf.h` — LVGL configuration (`LV_COLOR_DEPTH 16` / RGB565, 64 KB heap).

### Display pipeline

- Logical framebuffer: 64×64 px, RGB565.
- SDL window is `64 × kScale` pixels; `kScale` lives in `platform/sdl/src/sdl_internal.hpp` (currently `4` → 256×256 window).
- `sdl_display.cpp`: single full-frame draw buffer, nearest-neighbour upscale via `SDL_HINT_RENDER_SCALE_QUALITY=0`.

### Input

Arrow/WASD keys and Enter/Escape are mapped to LVGL keypad events in `platform/sdl/src/sdl_input.cpp`. LVGL groups route these to the focused widget; whenever a screen transition occurs, `lv_indev_set_group()` is called to retarget the keypad.

### UI structure

`launcher_init()` builds one screen (title row / scrollable list / info row) and hands it to LVGL. Selecting an item calls `mock_app_open()`, which slides in a placeholder screen and temporarily hijacks the keypad group. ESC slides back to the launcher.

`hue_focus.cpp` runs an LVGL timer (~30 Hz) that cycles the focused label's color through HSV hue 0→360° over 4 000 ms.

### Adding a new platform

Create `platform/<name>/` with a CMakeLists.txt and a class that implements `cube::IPlatform`. The shared lib requires no changes.

### Adding apps to the launcher

Edit the `kApps[]` array in `shared/include/cube/config.hpp`. The list renders automatically; `mock_app_open()` is the placeholder until real app screens are wired in.
