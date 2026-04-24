#include "cube/config.hpp"
#include "cube/detail/hue_utils.hpp"
#include "lvgl.h"

namespace cube {

namespace {

lv_group_t* g_group        = nullptr;
lv_timer_t* g_timer        = nullptr;
lv_obj_t*   g_last_focused = nullptr;

// The focused list item is a button containing a single label.
// Color the label, not the button itself (matches the screenshot).
lv_obj_t* label_of(lv_obj_t* item) {
    if (!item || lv_obj_get_child_count(item) == 0) return nullptr;
    return lv_obj_get_child(item, 0);
}

void tick_cb(lv_timer_t* /*t*/) {
    if (!g_group) return;
    lv_obj_t* focused = lv_group_get_focused(g_group);

    // Focus changed → restore previous item's label color to white.
    if (focused != g_last_focused) {
        if (auto* prev_label = label_of(g_last_focused)) {
            lv_obj_set_style_text_color(prev_label, color_text(), LV_PART_MAIN);
        }
        g_last_focused = focused;
    }

    if (auto* lbl = label_of(focused)) {
        const uint32_t ms = lv_tick_get();
        const int h = static_cast<int>((ms * 360u / kFocusHuePeriodMs) % 360u);
        lv_obj_set_style_text_color(lbl, cube::detail::hsv_to_color(h), LV_PART_MAIN);
    }
}

} // namespace

void hue_focus_init(lv_group_t* group) {
    g_group        = group;
    g_last_focused = nullptr;
    g_timer        = lv_timer_create(tick_cb, kHueTimerPeriodMs, nullptr);
}

void hue_focus_pause() {
    if (g_timer) lv_timer_pause(g_timer);
}

void hue_focus_resume() {
    if (g_timer) lv_timer_resume(g_timer);
}

void hue_focus_deinit() {
    if (g_timer) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
    }
    g_group        = nullptr;
    g_last_focused = nullptr;
}

} // namespace cube
