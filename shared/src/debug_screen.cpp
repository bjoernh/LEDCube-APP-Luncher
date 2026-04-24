#include "cube/config.hpp"
#include "cube/lv_font_cube_6px.h"
#include "internal.hpp"

#include <initializer_list>

namespace cube {

namespace {

struct DebugEntry { const char* label; bool* state; };

// State persists across open/close of the Debug screen.
bool s_show_fps = false;
bool s_show_imu = false;

DebugEntry kEntries[] = {
    {"FPS", &s_show_fps},
    {"IMU", &s_show_imu},
};
constexpr int kEntryCount = static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));

lv_indev_t* s_keypad     = nullptr;
void      (*s_on_exit)() = nullptr;
lv_obj_t*   s_screen     = nullptr;
lv_group_t* s_group      = nullptr;

void exit_debug() {
    exit_screen_group(s_group, s_keypad, s_on_exit);
}

void on_item_key(lv_event_t* e) {
    if (lv_event_get_key(e) == LV_KEY_ESC) exit_debug();
}

void update_label(lv_obj_t* item) {
    auto idx = reinterpret_cast<intptr_t>(lv_obj_get_user_data(item));
    auto* lbl = lv_obj_get_child(item, 0);
    if (!lbl) return;
    lv_label_set_text_fmt(lbl, "%s: %s", kEntries[idx].label,
                          *kEntries[idx].state ? "ON" : "OFF");
}

void on_item_click(lv_event_t* e) {
    auto* item = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  idx  = reinterpret_cast<intptr_t>(lv_obj_get_user_data(item));
    *kEntries[idx].state = !*kEntries[idx].state;
    update_label(item);
}

lv_obj_t* make_entry(lv_obj_t* parent, int idx) {
    lv_obj_t* item = lv_button_create(parent);
    lv_obj_set_size(item, kScreenW, kItemH);
    lv_obj_set_style_pad_all(item, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(item, 0, LV_PART_MAIN);

    remove_widget_chrome(item);
    apply_focus_indicator(item, color_title());
    lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    make_styled_label(item, "", color_text());

    lv_obj_set_user_data(item, reinterpret_cast<void*>(static_cast<intptr_t>(idx)));
    update_label(item);

    lv_obj_add_event_cb(item, on_item_click, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(item, on_item_key,   LV_EVENT_KEY,     nullptr);
    return item;
}

} // namespace

void debug_open(lv_indev_t* keypad, void (*on_exit)()) {
    s_keypad  = keypad;
    s_on_exit = on_exit;

    s_screen = lv_obj_create(nullptr);
    lv_obj_set_size(s_screen, kScreenW, kScreenH);
    lv_obj_set_style_bg_color(s_screen, color_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_screen, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_screen, 0, LV_PART_MAIN);

    // Title row
    lv_obj_t* title = lv_label_create(s_screen);
    lv_obj_set_size(title, kScreenW, kTitleH);
    lv_obj_set_style_text_font(title, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, color_title(), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(title, "DEBUG");

    // List container
    lv_obj_t* list = lv_obj_create(s_screen);
    lv_obj_set_size(list, kScreenW, kListH);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    s_group = lv_group_create();
    lv_group_set_wrap(s_group, true);
    for (int i = 0; i < kEntryCount; ++i) {
        lv_obj_t* item = make_entry(list, i);
        lv_group_add_obj(s_group, item);
    }
    lv_indev_set_group(keypad, s_group);

    // Info row (empty — keeps title/list/info layout consistent)
    lv_obj_t* info = lv_obj_create(s_screen);
    lv_obj_set_size(info, kScreenW, kInfoH);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info, 0, LV_PART_MAIN);
    lv_obj_remove_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    lv_screen_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT,
                        kScreenTransitionMs, 0, false);
}

} // namespace cube
