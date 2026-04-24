# Plan: Cube 2.0 App Launcher PoC (LVGL + SDL2)

## Context

Build a desktop-simulated PoC for a 64×64 LED-matrix device with gamepad input. The real device is embedded; this PoC runs on macOS/Linux so UI iteration is fast before touching hardware. It simulates the hardware via SDL2 (640×640 window, 10× nearest-neighbor scaled from a 64×64 logical framebuffer) and takes keyboard input standing in for a gamepad. The launcher UI must visually match the provided screenshot (black bg, centered UPPERCASE labels, scrolling blue hostname title, red battery info row, hue-cycling focus) and use the user's custom 4×6 bitmap font.

**Key architectural requirement:** the PoC must strictly separate **UI/logic (shared, platform-agnostic)** from **drivers (platform-specific)** so the same UI code can later be embedded in a mobile companion app, or driven by the real cube hardware, without modification. The desktop SDL binary is just one consumer of the shared core.

Stack: **C++20, CMake, LVGL v9.2, SDL2** (desktop backend only). LVGL vendored as submodule at `lib/lvgl/`.

## Architecture: three layers

```
┌──────────────────────────────────────────┐
│  App binary  (apps/desktop/main.cpp)     │  ← tiny glue: instantiate platform,
│                                          │    call shared init, run loop
├──────────────────────────────────────────┤
│  shared/   (cube_shared static lib)      │  ← pure LVGL + C++; no SDL, no POSIX
│    • launcher UI + state machine         │    dependencies. Reused by mobile
│    • hue focus animation                 │    companion / real hardware.
│    • mock app screens                    │
│    • 6px font data                       │
│    • config constants                    │
│    • IPlatform interface (abstract)      │
├──────────────────────────────────────────┤
│  platform/sdl/  (cube_platform_sdl lib)  │  ← implements IPlatform on SDL2:
│    • SDL display + flush_cb              │    window, renderer, texture, keyboard
│    • SDL keyboard → LVGL indev           │    poll, hostname, battery. A future
│    • hostname / battery                  │    platform/ios or platform/cube lib
│    • LVGL tick callback                  │    would be a drop-in replacement.
├──────────────────────────────────────────┤
│  lib/lvgl/   (submodule, release/v9.2)   │
└──────────────────────────────────────────┘
```

**Dependency rule:** `shared/` never `#include`s SDL, POSIX, or anything platform-specific. `platform/sdl/` depends on `shared/` (to implement its interfaces) and SDL2. The app binary links both.

## Target layout (64×64)

```
┌────────────────────────────────────┐   0
│  MACBOOKAIR-M1-2  ← blue, marquee  │   6px title row
├────────────────────────────────────┤
│            SNAKE                   │
│         BREAKOUT3D   ← focused     │  52px list area
│          PIXELFLOW    (hue cycle)  │  5 visible rows × 10px
│         MATRIXRAIN                 │  (scrolls vertically — 7 items total)
│         VOXELSAND                  │
│           WATER                    │
│          SETTINGS                  │
├────────────────────────────────────┤
│                       100%  ← red  │   6px info row (battery right-aligned)
└────────────────────────────────────┘   64
```

Row math: title 6 + list 52 + info 6 = 64. List holds 7 items at 10px; only 5 fit on screen so the list scrolls — **intentional to demo LVGL's snap/scroll in the PoC**.

**App list:** `SNAKE`, `BREAKOUT3D`, `PIXELFLOW`, `MATRIXRAIN`, `VOXELSAND`, `WATER`, `SETTINGS` (7 items).

## Design decisions locked in

| Area | Decision |
|---|---|
| LVGL version | v9.2.x (submodule at `lib/lvgl/`) |
| Color depth | `LV_COLOR_DEPTH 16` (RGB565) — matches LED-matrix target hardware |
| Language | **C++20** (`-std=c++20`); LVGL C headers consumed via `extern "C"` |
| Font | Convert `6px_font.h` → LVGL v9 `lv_font_cube_6px.c` (Option A) |
| Focus style | HSV hue cycle 0→360° over **4000ms**, continuous while focused |
| Screen transitions | `lv_screen_load_anim` — slide-left entering app (300ms), slide-right returning (300ms) |
| Title color | `#0000FF` constant (`cube::kColorTitle`) for easy tweaking |
| Info color | Red `#FF0000` constant (`cube::kColorInfo`) |
| Focus wrap | Yes (LVGL group default) |
| FPS | 60 (SDL_Delay 16ms + lv_timer_handler) |
| Mock app screen | Black bg, white centered name + "ESC TO EXIT" below |

## File layout

```
app_luncher_lvgl/
├── .gitignore
├── .gitmodules
├── CMakeLists.txt                          (top-level — orchestrates the 3 layers)
├── README.md
├── 6px_font.h                              (source — already present)
├── lib/
│   └── lvgl/                               (submodule, release/v9.2)
├── shared/                                 ─── platform-agnostic core ───
│   ├── CMakeLists.txt                      (builds cube_shared)
│   ├── include/
│   │   ├── lv_conf.h                       (LVGL config — no SDL deps)
│   │   └── cube/
│   │       ├── config.hpp                  (layout, colors, timings, app list)
│   │       ├── platform.hpp                (IPlatform abstract interface)
│   │       ├── launcher.hpp                (public UI API)
│   │       └── lv_font_cube_6px.h          (extern "C" font decl)
│   └── src/
│       ├── launcher.cpp                    (UI + state machine + screen transitions)
│       ├── hue_focus.cpp                   (hue-cycle animation logic)
│       ├── mock_app.cpp                    (mock app screen builder)
│       └── lv_font_cube_6px.c              (GENERATED — pure data, compiled as C)
├── platform/
│   └── sdl/                                ─── SDL2 backend ───
│       ├── CMakeLists.txt                  (builds cube_platform_sdl)
│       ├── include/cube/sdl/
│       │   └── sdl_platform.hpp            (SdlPlatform : public IPlatform)
│       └── src/
│           ├── sdl_platform.cpp            (constructor wires everything)
│           ├── sdl_display.cpp             (SDL window/renderer/texture + flush_cb)
│           ├── sdl_input.cpp               (SDL key poll → LVGL indev read_cb)
│           └── sdl_system_info.cpp         (gethostname, static battery)
├── apps/
│   └── desktop/                            ─── executable ───
│       ├── CMakeLists.txt                  (builds cube_launcher)
│       └── src/main.cpp                    (tiny glue: init + run loop)
└── tools/
    └── gen_font.py                         (one-shot: 6px_font.h → lv_font_cube_6px.c)
```

## Interface contract (`shared/include/cube/platform.hpp`)

```cpp
#pragma once
#include <cstdint>
#include <string>
#include "lvgl.h"

namespace cube {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    // Owned by the platform: LVGL display + input device.
    // Shared UI code uses these only through LVGL's own APIs.
    virtual lv_display_t* display() = 0;
    virtual lv_indev_t*   keypad()  = 0;

    // Called once per main-loop iteration: drain OS events.
    virtual void pump_events() = 0;

    // True once the OS requests quit (window close, ⌘Q, SIGTERM, …).
    virtual bool should_quit() const = 0;

    // Blocking sleep used by the run loop — platforms that need a specific
    // sleep (e.g. mobile must yield to the OS runloop) override here.
    virtual void sleep_ms(uint32_t ms) = 0;

    // System info for the UI strip.
    virtual std::string hostname() = 0;
    virtual int battery_percent() = 0;
};

} // namespace cube
```

The shared library exposes:

```cpp
// shared/include/cube/launcher.hpp
namespace cube {
void launcher_init(IPlatform& plat);   // builds UI, attaches input group, starts hue timer
void launcher_deinit();                // tears down UI, timers, cached screens
} // namespace cube
```

The shared library also provides the LVGL tick. Rather than push `lv_tick_inc` from the platform, we expose a single entry point:

```cpp
// shared/include/cube/platform.hpp (continued)
namespace cube {
// Platform must call this ONCE during init, passing a function that returns
// monotonic milliseconds. Implemented via lv_tick_set_cb under the hood.
void install_tick_source(uint32_t (*tick_ms_fn)());
} // namespace cube
```

This keeps the tick contract in shared code; the SDL platform just passes `SDL_GetTicks` and a future iOS platform passes its own clock.

## Shared layer — config (`shared/include/cube/config.hpp`)

```cpp
#pragma once
#include "lvgl.h"

namespace cube {

// Display
constexpr int kScreenW = 64;
constexpr int kScreenH = 64;

// Rows
constexpr int kTitleH = 6;
constexpr int kInfoH  = 6;
constexpr int kListH  = kScreenH - kTitleH - kInfoH;  // 52
constexpr int kItemH  = 10;

// Colors (stored as lv_color_t constants built from #RRGGBB)
inline lv_color_t color_title() { return lv_color_hex(0x0000FF); }
inline lv_color_t color_info()  { return lv_color_hex(0xFF0000); }
inline lv_color_t color_bg()    { return lv_color_hex(0x000000); }
inline lv_color_t color_text()  { return lv_color_hex(0xFFFFFF); }

// Timings
constexpr uint32_t kFocusHuePeriodMs    = 4000;
constexpr uint32_t kScreenTransitionMs  = 300;

// App list
struct AppEntry { const char* label; int id; };
constexpr AppEntry kApps[] = {
    {"SNAKE",      0},
    {"BREAKOUT3D", 1},
    {"PIXELFLOW",  2},
    {"MATRIXRAIN", 3},
    {"VOXELSAND",  4},
    {"WATER",      5},
    {"SETTINGS",   6},
};
constexpr int kAppCount = sizeof(kApps) / sizeof(kApps[0]);

} // namespace cube
```

## Shared layer — launcher (`shared/src/launcher.cpp`)

- Builds `launcher_screen` once; kept alive for the session.
- Title label — 64×6, blue, `LV_LABEL_LONG_SCROLL_CIRCULAR`, text set via `plat.hostname()` (stripped after first `.`).
- List container — 64×52, flex column, cross-axis centered, `pad_all=0`, `pad_row=0`, `LV_OBJ_FLAG_SCROLLABLE` on, `lv_obj_set_scroll_snap_y(LV_SCROLL_SNAP_CENTER)`, holds 7 list items at 10px.
- Info row — 64×6, flex row right-aligned, red "100%" label (font lacks a battery glyph; text-only per spec). Content set once from `plat.battery_percent()`.
- Group — `lv_group_create()` + `lv_indev_set_group(plat.keypad(), group)`, all 7 buttons added; `lv_group_set_wrap(group, true)`.
- Item factory — `lv_obj_t* make_item(parent, label, id)` — borderless/shadowless/radiusless button, 64×10, centered label in `lv_font_cube_6px`, white default text, click event → `on_item_activate(id)`.
- Hue-cycle timer — 33ms, recomputes `hue = (lv_tick_get() * 360 / kFocusHuePeriodMs) % 360`, HSV→RGB, applies to currently focused item's label color. Paused on IN_APP, resumed on return. Focus-change callback restores defocused item to white.

### State machine

```cpp
enum class AppState { Launcher, InApp };
static AppState g_state = AppState::Launcher;
```

- **Enter app** (`on_item_activate`):
  1. Build `app_screen` as top-level via `lv_obj_create(nullptr)` (black bg, centered name label in 6px font + "ESC TO EXIT" white label below) — lives in `shared/src/mock_app.cpp`.
  2. `lv_screen_load_anim(app_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false)` — `auto_del=false` keeps launcher alive.
  3. `lv_indev_set_group(keypad, nullptr)` — detach nav.
  4. Add root-level `LV_EVENT_KEY` handler on `app_screen` for `LV_KEY_ESC`.
  5. Pause hue timer; set state = `InApp`.
- **Exit app** (ESC handler):
  1. `lv_screen_load_anim(launcher_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, true)` — `auto_del=true` → LVGL destroys the old `app_screen` when the transition completes (no leak, no manual `lv_obj_delete` needed).
  2. Re-attach group; resume hue timer; state = `Launcher`.

## Shared layer — LVGL config (`shared/include/lv_conf.h`)

- `LV_COLOR_DEPTH 16` (RGB565)
- `LV_COLOR_16_SWAP 0` (matches SDL's `SDL_PIXELFORMAT_RGB565`)
- `LV_MEM_SIZE (32 * 1024)`
- `LV_USE_LOG 1`, log level warn
- `LV_TICK_CUSTOM 1` with `LV_TICK_CUSTOM_INCLUDE "cube/tick_bridge.h"` and `LV_TICK_CUSTOM_SYS_TIME_EXPR (cube_tick_ms())` — `cube_tick_ms()` is a shim defined in `shared/src/tick_bridge.cpp` that forwards to the fn-ptr installed via `cube::install_tick_source(...)`. Keeps LVGL fully platform-agnostic.
- Fonts: disable all built-ins except `LV_FONT_MONTSERRAT_8` (safety fallback). Custom font is linked separately.
- `LV_USE_FLEX 1`. Animations always-on in v9.
- Themes, file system, images off.

## Font conversion (Option A)

- `tools/gen_font.py` — parses `Vector2i(x,y)` coordinate lists from `6px_font.h` into 4×6 bitmaps, emits LVGL v9 font format as a `.c` file.
- Output: 1bpp packed bitmap per glyph (padded to byte boundaries per LVGL spec) + `lv_font_fmt_txt_dsc_t` + single `lv_font_fmt_txt_cmap_t` + `lv_font_t lv_font_cube_6px` with `line_height=7`, `base_line=0`, `subpx=LV_FONT_SUBPX_NONE`.
- **Coverage:** `0-9`, `A-Z`, `.`, `,`, `:`, `-`, `+`, `_`, `!`, `?`, space. Lowercase absent — all UI text forced uppercase.
- Run once: `python3 tools/gen_font.py > shared/src/lv_font_cube_6px.c`. Output committed; build never invokes Python.

## Platform layer — SDL (`platform/sdl/`)

All SDL code lives **only** in this directory. It implements `cube::IPlatform`.

### `sdl_display.cpp`
- `SDL_Init(SDL_INIT_VIDEO)`
- `SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")` — nearest-neighbor
- `SDL_CreateWindow(640, 640)`, `SDL_CreateRenderer`
- `SDL_CreateTexture(SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 64, 64)` — matches `LV_COLOR_DEPTH 16`, so the LVGL buffer uploads directly with no per-pixel conversion
- LVGL draw buffer: one 64×64 partial buffer (`LV_DISPLAY_RENDER_MODE_PARTIAL`)
- `flush_cb`: `SDL_UpdateTexture` with LVGL's buffer, `SDL_RenderCopy` (10× upscale), `SDL_RenderPresent`, `lv_display_flush_ready()`

### `sdl_input.cpp`
- `lv_indev_create()` type `LV_INDEV_TYPE_KEYPAD`
- `read_cb` reads a small internal key-state struct updated by `pump_events()`:
  - `SDLK_UP→LV_KEY_UP`, `SDLK_DOWN→LV_KEY_DOWN`, `SDLK_LEFT→LV_KEY_LEFT`, `SDLK_RIGHT→LV_KEY_RIGHT`
  - `SDLK_RETURN→LV_KEY_ENTER`, `SDLK_ESCAPE→LV_KEY_ESC`
- `SDL_QUIT` → sets internal `quit_requested` flag read by `should_quit()`

### `sdl_system_info.cpp`
- `hostname()` → `gethostname()`, strip after first `.`, upper-case
- `battery_percent()` → returns `100` (PoC placeholder)

### `sdl_platform.cpp`
Orchestrates the three above; constructor calls `cube::install_tick_source(&SDL_GetTicks)` as its first act.

## App binary (`apps/desktop/src/main.cpp`)

Minimal glue — no LVGL primitives beyond `lv_init`/`lv_timer_handler`:

```cpp
#include "cube/sdl/sdl_platform.hpp"
#include "cube/launcher.hpp"

int main() {
    lv_init();
    cube::SdlPlatform plat;          // creates window, display, indev, installs tick
    cube::launcher_init(plat);       // builds UI from shared config

    while (!plat.should_quit()) {
        plat.pump_events();
        uint32_t t = lv_timer_handler();
        plat.sleep_ms(t < 16 ? 16 - t : 1);
    }
    cube::launcher_deinit();
    return 0;
}
```

Swapping to a future iOS companion: write `cube::IosPlatform`, link it instead of `cube_platform_sdl`, reuse `cube_shared` and `apps/...` unchanged.

## CMake structure

**Top-level `CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.16)
project(cube_launcher LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# LVGL picks lv_conf.h from shared/include via this var
set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/shared/include/lv_conf.h" CACHE STRING "" FORCE)
add_subdirectory(lib/lvgl)

add_subdirectory(shared)
add_subdirectory(platform/sdl)
add_subdirectory(apps/desktop)
```

**`shared/CMakeLists.txt`**
- `add_library(cube_shared STATIC ...)` — `.cpp` + the generated `.c`
- `target_link_libraries(cube_shared PUBLIC lvgl)` — no SDL
- `target_include_directories(cube_shared PUBLIC include)`
- Warnings: `-Wall -Wextra -Wpedantic`

**`platform/sdl/CMakeLists.txt`**
- `find_package(SDL2 REQUIRED)`
- `add_library(cube_platform_sdl STATIC ...)` — the four `.cpp` files
- `target_link_libraries(cube_platform_sdl PUBLIC cube_shared SDL2::SDL2)`
- `target_include_directories(cube_platform_sdl PUBLIC include)`

**`apps/desktop/CMakeLists.txt`**
- `add_executable(cube_launcher src/main.cpp)`
- `target_link_libraries(cube_launcher PRIVATE cube_platform_sdl)` — pulls shared transitively

This layering is **enforced by CMake**: `cube_shared` has no SDL link dependency, so a stray `#include <SDL.h>` in `shared/src/*.cpp` fails to build. That is the guard rail keeping the abstraction honest.

## Git setup

- `git init`, `.gitignore` (`build/`, `.DS_Store`, `.cache/`)
- `git submodule add https://github.com/lvgl/lvgl.git lib/lvgl`
- `cd lib/lvgl && git checkout release/v9.2 && cd ../..`
- Commit generated `shared/src/lv_font_cube_6px.c` so builds don't need Python.

## Verification

1. **Build**
   ```bash
   brew install sdl2 cmake        # macOS (Linux: apt install libsdl2-dev cmake)
   cmake -S . -B build
   cmake --build build -j
   ```
2. **Run**: `./build/apps/desktop/cube_launcher`
3. **Smoke test:**
   - Window 640×640, crisp/pixelated — confirms `SDL_HINT_RENDER_SCALE_QUALITY=0`
   - Top row: hostname in `#0000FF`, scrolling if >16 chars
   - List: SNAKE / BREAKOUT3D / PIXELFLOW / MATRIXRAIN / VOXELSAND / WATER / SETTINGS — white, centered, 5 visible
   - Scrolling past MATRIXRAIN reveals VOXELSAND / WATER / SETTINGS; wraps at ends
   - Bottom row: `100%` in red, right-aligned
   - Focused item hue-cycles with a **4-second** rainbow period
   - Enter: **slides left** to black screen with app name + "ESC TO EXIT"
   - ESC: **slides right** back to launcher, focus restored
   - ⌘Q / close-button quits cleanly
4. **Layering check (critical for the shared/platform goal):**
   ```bash
   grep -r "SDL" shared/src shared/include || echo "PASS: no SDL references in shared/"
   ```
   Must return the PASS line — proves `shared/` is truly platform-agnostic.
5. **Memory**: `leaks --atExit -- ./build/apps/desktop/cube_launcher` (macOS) after cycling enter/esc on all 7 apps several times → no growth.

## Critical files to create

- `CMakeLists.txt` (top-level), `.gitignore`
- `shared/CMakeLists.txt`, `shared/include/lv_conf.h`
- `shared/include/cube/{config,platform,launcher,lv_font_cube_6px}.{hpp,h}`
- `shared/src/{launcher,hue_focus,mock_app,tick_bridge}.cpp`, `shared/src/lv_font_cube_6px.c` (generated)
- `platform/sdl/CMakeLists.txt`
- `platform/sdl/include/cube/sdl/sdl_platform.hpp`
- `platform/sdl/src/{sdl_platform,sdl_display,sdl_input,sdl_system_info}.cpp`
- `apps/desktop/CMakeLists.txt`, `apps/desktop/src/main.cpp`
- `tools/gen_font.py`, `README.md`

## Out of scope

- Real app implementations — only mock screens
- Battery icon glyph (text-only "100%" since font lacks one)
- Live hostname/battery polling — read once at startup, battery static 100%
- Hardware GPIO / LED matrix driver
- Additional platform backends (iOS, Android, real cube) — structure supports them, not included in this PoC


Last step before context window was full:

Updated todos
Created
main.cpp
+21
-0

/Users/bjoern/Working/cube2.0/app_luncher_lvgl/
apps/desktop/src/main.cpp
