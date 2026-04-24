#pragma once

#include "cube/platform.hpp"

namespace cube {

// Concrete IPlatform for desktop SDL2.
// Construction order: SDL_Init → window + renderer + texture → LVGL display
// wired to a flush_cb → LVGL keypad indev wired to a key-poll state →
// install_tick_source(SDL_GetTicks).
//
// The constructor throws if SDL init fails. The destructor tears everything
// down in reverse.
class SdlPlatform : public IPlatform {
public:
    SdlPlatform();
    ~SdlPlatform() override;

    SdlPlatform(const SdlPlatform&)            = delete;
    SdlPlatform& operator=(const SdlPlatform&) = delete;

    lv_display_t* display() override;
    lv_indev_t*   keypad()  override;
    void          pump_events() override;
    bool          should_quit() const override;
    void          sleep_ms(uint32_t ms) override;
    std::string   hostname() override;
    int           battery_percent() override;
};

} // namespace cube
