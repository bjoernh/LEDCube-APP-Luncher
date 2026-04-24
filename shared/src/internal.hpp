// Internal symbols shared between the shared-layer .cpp files.
// Not part of the public cube/ API — keep out of include/.
#pragma once

#include "lvgl.h"

namespace cube {

// hue_focus.cpp
void hue_focus_init(lv_group_t* group);
void hue_focus_pause();
void hue_focus_resume();
void hue_focus_deinit();

// mock_app.cpp
// Build a mock app screen for `name` and slide-transition into it.
// We need the keypad so we can temporarily retarget it at a one-item group
// containing just the mock screen — otherwise ESC wouldn't route anywhere
// (LVGL drops keypad events when the indev's group is NULL).
// When the user presses ESC, the mock cleans up its group and calls on_exit();
// the caller performs the reverse transition (auto_del=true deletes this screen).
void mock_app_open(const char* name, lv_indev_t* keypad, void (*on_exit)());

// settings.cpp
// Slide in the settings list screen. on_exit() is called when the user
// presses ESC; the caller is responsible for the reverse slide.
void settings_open(lv_indev_t* keypad, void (*on_exit)());

// startup_anim.cpp — CRT scanline sweep (two lines from center, reveal launcher beneath)
// parent must be the already-loaded launcher screen.
void startup_anim_scanline_show(lv_obj_t* parent, void (*on_complete)());
void startup_anim_scanline_deinit();

// startup_anim_phosphor.cpp — phosphor warm-up (pixels ignite red, spread, heat to white)
void startup_anim_phosphor_show(void (*on_complete)());
void startup_anim_phosphor_deinit();

} // namespace cube
