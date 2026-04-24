// Coordination between the SDL platform translation units.
// Not exposed outside platform/sdl/.
#pragma once

#include <SDL.h>
#include <string>
#include "lvgl.h"

namespace cube::sdl {

// Window / renderer constants
inline constexpr int   kScale       = 4;
inline constexpr int   kWindowW     = 64 * kScale;
inline constexpr int   kWindowH     = 64 * kScale;
inline constexpr char  kWindowTitle[] = "Cube Launcher (PoC)";

// sdl_display.cpp
lv_display_t* display_init();
void          display_deinit();

// sdl_input.cpp
lv_indev_t* input_init();
void        input_deinit();
void        input_pump_events();     // drain SDL events; must be called each loop
bool        input_quit_requested();

// sdl_system_info.cpp
std::string read_hostname();
int         read_battery_percent();

} // namespace cube::sdl
