#include "cube/config.hpp"
#include "cube/lv_font_cube_6px.h"
#include "internal.hpp"

namespace cube {

namespace {

struct InfoRow { const char* label; const char* value; };
constexpr InfoRow kInfo[] = {
    {"VERSION", "v0.4.8"},
    {"IP WLAN", "192.168.1.5"},
    {"IP LAN",  "10.10.10.6"},
};
constexpr int     kInfoCount = static_cast<int>(sizeof(kInfo) / sizeof(kInfo[0]));
constexpr int32_t kRowH      = kTitleH * 2; // two 6-px label rows per element

lv_indev_t* s_keypad     = nullptr;
void      (*s_on_exit)() = nullptr;
lv_obj_t*   s_screen     = nullptr;
lv_group_t* s_group      = nullptr;

void exit_info() {
    exit_screen_group(s_group, s_keypad, s_on_exit);
}

void on_screen_key(lv_event_t* e) {
    if (lv_event_get_key(e) == LV_KEY_ESC) exit_info();
}

lv_obj_t* make_row(lv_obj_t* parent, const InfoRow& row) {
    lv_obj_t* cell = lv_obj_create(parent);
    lv_obj_set_size(cell, kScreenW, kRowH);
    lv_obj_set_style_pad_all(cell, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cell, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cell, 0, LV_PART_MAIN);
    lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = lv_label_create(cell);
    lv_obj_set_style_text_font(lbl, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, color_title(), LV_PART_MAIN);
    lv_label_set_text(lbl, row.label);
    lv_obj_set_style_pad_all(lbl, 0, LV_PART_MAIN);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* val = lv_label_create(cell);
    lv_obj_set_style_text_font(val, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(val, color_text(), LV_PART_MAIN);
    lv_label_set_text(val, row.value);
    lv_obj_set_style_pad_all(val, 0, LV_PART_MAIN);
    lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, 0);

    return cell;
}

} // namespace

void info_open(lv_indev_t* keypad, void (*on_exit)()) {
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
    lv_label_set_text(title, "INFO");

    // List container (scrollable so more entries could be added later)
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
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < kInfoCount; ++i) {
        make_row(list, kInfo[i]);
    }

    // Info row (empty — preserves the title/list/info skeleton)
    lv_obj_t* info = lv_obj_create(s_screen);
    lv_obj_set_size(info, kScreenW, kInfoH);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info, 0, LV_PART_MAIN);
    lv_obj_remove_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    // Keypad events only route via a group — put the screen alone in one so
    // ESC reaches on_screen_key and returns control to Settings.
    s_group = lv_group_create();
    lv_group_add_obj(s_group, s_screen);
    lv_indev_set_group(keypad, s_group);
    lv_obj_add_event_cb(s_screen, on_screen_key, LV_EVENT_KEY, nullptr);

    lv_screen_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT,
                        kScreenTransitionMs, 0, false);
}

} // namespace cube
