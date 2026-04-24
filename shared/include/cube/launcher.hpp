#pragma once

#include "cube/platform.hpp"

namespace cube {

// Build the launcher UI and attach it to the platform's LVGL display + keypad.
// Must be called exactly once, after lv_init() and after the platform is
// fully constructed. Takes an IPlatform& (not a pointer) — the launcher
// doesn't own it; the caller guarantees it outlives launcher_deinit().
void launcher_init(IPlatform& plat);

// Tear down the launcher UI, timers, and any cached screens.
void launcher_deinit();

} // namespace cube
