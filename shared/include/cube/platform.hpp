#pragma once

#include <cstdint>
#include <string>
#include "lvgl.h"

namespace cube {

// Abstract platform interface. The shared UI layer depends ONLY on this.
// Concrete platform backends (SDL2, iOS, real hardware, …) implement it.
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // LVGL objects owned by the platform. Shared code interacts with them
    // only through LVGL's own APIs (no platform-specific calls).
    virtual lv_display_t* display() = 0;
    virtual lv_indev_t*   keypad()  = 0;

    // Called once per main-loop iteration: drain OS events.
    virtual void pump_events() = 0;

    // True once the OS requests quit (window close, ⌘Q, SIGTERM, …).
    virtual bool should_quit() const = 0;

    // Blocking sleep used by the run loop — platforms may need a specific
    // sleep (e.g. mobile must yield to the OS runloop) and override here.
    virtual void sleep_ms(uint32_t ms) = 0;

    // System info for the UI strip.
    virtual std::string hostname()        = 0;
    virtual int         battery_percent() = 0;
};

// Each platform calls this once during init, passing a function that returns
// monotonic milliseconds. The shared layer wires it into LVGL via
// lv_tick_set_cb, keeping LVGL's tick source platform-agnostic.
void install_tick_source(uint32_t (*tick_ms_fn)());

} // namespace cube
