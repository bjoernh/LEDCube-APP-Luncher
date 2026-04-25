// Internal symbols shared between the shared-layer .cpp files.
// Not part of the public cube/ API — keep out of include/.
#pragma once

#include "lvgl.h"
#include <initializer_list>

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

// debug_screen.cpp — Debug submenu under Settings.
void debug_open(lv_indev_t* keypad, void (*on_exit)());

// info_screen.cpp — Info screen (label/value list).
void info_open(lv_indev_t* keypad, void (*on_exit)());

// startup_anim.cpp — CRT scanline sweep (two lines from center, reveal launcher beneath)
// parent must be the already-loaded launcher screen.
void startup_anim_scanline_show(lv_obj_t* parent, void (*on_complete)());
void startup_anim_scanline_deinit();

// startup_anim_phosphor.cpp — phosphor warm-up (pixels ignite red, spread, heat to white)
void startup_anim_phosphor_show(void (*on_complete)());
void startup_anim_phosphor_deinit();

// launch_anim.cpp — app-launch sequence: invert focused row 300 ms, then
// reverse CRT scanline sweep to black, then on_complete().
// Caller is responsible for whatever happens after black (typically: fire
// IPlatform::launch_app and resume hue_focus).
void launch_animation_play(lv_obj_t* focused_item, void (*on_complete)());
void launch_animation_deinit();

// ---------------------------------------------------------------------------
// Shared UI helpers (implementations are inline — no separate .cpp needed)
// ---------------------------------------------------------------------------

// Detach keypad from group, delete group, null it out, then call on_exit.
// Mirrors the identical exit pattern in mock_app, settings, debug, info screens.
inline void exit_screen_group(lv_group_t*& group, lv_indev_t* keypad,
                              void (*on_exit)()) {
    if (group) {
        lv_indev_set_group(keypad, nullptr);
        lv_group_delete(group);
        group = nullptr;
    }
    if (on_exit) on_exit();
}

// Strip all default-theme chrome (border/shadow/outline/bg) from every
// LVGL widget state so buttons look plain in a pixel-art UI.
// Pass clear_bg=false for slider items where bg_opa must stay set (slider track).
inline void remove_widget_chrome(lv_obj_t* obj, bool clear_bg = true) {
    for (lv_state_t st : {LV_STATE_DEFAULT, LV_STATE_PRESSED,
                          LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY}) {
        if (clear_bg)
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | st);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | st);
        lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | st);
        lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | st);
        lv_obj_set_style_outline_pad(obj, 0, LV_PART_MAIN | st);
    }
}

// Apply a 1-pixel top+bottom focus border on focused/focus-key states.
inline void apply_focus_indicator(lv_obj_t* obj, lv_color_t color) {
    for (lv_state_t st : {LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY}) {
        lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | st);
        lv_obj_set_style_border_color(obj, color, LV_PART_MAIN | st);
        lv_obj_set_style_border_side(obj,
                                     static_cast<lv_border_side_t>(
                                         LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM),
                                     LV_PART_MAIN | st);
        lv_obj_set_style_border_opa(obj, LV_OPA_COVER, LV_PART_MAIN | st);
    }
}

// Create a label with the standard item-row style (white text,
// zero padding, height=kTitleH, centered in parent).
// Defined in ui_helpers.cpp to access kTitleH.
lv_obj_t* make_styled_label(lv_obj_t* parent, const char* text, lv_color_t color);

// helpers (settings.cpp only)
// Snap a slider value to the nearest multiple of 10 with hysteresis:
// values that landed 1 above a decade snap up; values 1 below snap down.
// This prevents jitter when arrow-key steps straddle a decade boundary.
inline int32_t snap_slider_value(int32_t val) {
    int32_t snapped;
    if (val % 10 == 1) snapped = ((val / 10) * 10) + 10;
    else if (val % 10 == 9) snapped = (val / 10) * 10;
    else snapped = ((val + 5) / 10) * 10;
    if (snapped < 0)   snapped = 0;
    if (snapped > 100) snapped = 100;
    return snapped;
}

} // namespace cube
