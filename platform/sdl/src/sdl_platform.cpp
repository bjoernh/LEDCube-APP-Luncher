#include "cube/sdl/sdl_platform.hpp"
#include "sdl_internal.hpp"

namespace cube {

SdlPlatform::SdlPlatform() {
    // Tick source first — lv_tick_get() must work before any display /
    // animation code runs (display_init and input_init both hit LVGL).
    install_tick_source(&SDL_GetTicks);
    sdl::display_init();
    sdl::input_init();
}

SdlPlatform::~SdlPlatform() {
    sdl::input_deinit();
    sdl::display_deinit();
}

lv_display_t* SdlPlatform::display() {
    // LVGL v9 tracks a "default" display per thread — use that so we don't
    // have to store a pointer ourselves.
    return lv_display_get_default();
}

lv_indev_t* SdlPlatform::keypad() {
    // Same trick: LVGL walks its indev list; we only created one keypad.
    return lv_indev_get_next(nullptr);
}

void SdlPlatform::pump_events() {
    sdl::input_pump_events();
}

bool SdlPlatform::should_quit() const {
    return sdl::input_quit_requested();
}

void SdlPlatform::sleep_ms(uint32_t ms) {
    SDL_Delay(ms);
}

std::string SdlPlatform::hostname() {
    return sdl::read_hostname();
}

int SdlPlatform::battery_percent() {
    return sdl::read_battery_percent();
}

} // namespace cube
