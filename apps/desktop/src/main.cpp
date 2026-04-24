#include "cube/launcher.hpp"
#include "cube/sdl/sdl_platform.hpp"
#include "lvgl.h"

int main() {
    lv_init();
    cube::SdlPlatform plat;        // creates window, display, indev, installs tick
    cube::launcher_init(plat);     // builds UI from shared config

    while (!plat.should_quit()) {
        plat.pump_events();
        uint32_t t = lv_timer_handler();
        // Cap frame rate at ~60 FPS; lv_timer_handler returns "ms until next
        // timer" — if it's already beyond our frame budget, yield briefly.
        plat.sleep_ms(t < 16 ? 16 - t : 1);
    }

    cube::launcher_deinit();
    // plat destructor tears down SDL
    return 0;
}
