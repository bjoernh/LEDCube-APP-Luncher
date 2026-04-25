// Coordination between the matrixserver platform translation units.
// Not exposed outside platform/matrixserver/.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "lvgl.h"

class Screen;

namespace cube::ms {

// Display: 64×64 RGB565 framebuffer (8 KB).
// Shared between matrixserver_display.cpp and matrixserver_platform.cpp.
inline constexpr int kW = 64;
inline constexpr int kH = 64;

// matrixserver_display.cpp
lv_display_t* display_init();
void          display_deinit();
void          copy_to_screens(std::vector<std::shared_ptr<Screen>>& screens);

// matrixserver_input.cpp
lv_indev_t* input_init();
void        input_deinit();
void        pump_joystick();

// matrixserver_system_info.cpp
std::string read_hostname();
int         read_battery_percent();

// Tick source — monotonic milliseconds from steady_clock.
uint32_t monotonic_ms();

} // namespace cube::ms
