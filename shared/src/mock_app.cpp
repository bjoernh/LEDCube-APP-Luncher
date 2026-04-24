#include "cube/config.hpp"
#include "cube/lv_font_cube_6px.h"
#include "internal.hpp"

namespace cube {

namespace {

void (*s_on_exit)()        = nullptr;
lv_indev_t* s_keypad       = nullptr;
lv_group_t* s_mock_group   = nullptr;

void on_key(lv_event_t* e) {
    const uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_ESC) return;
    // Tear down the mock's own group before handing control back to the
    // launcher. The launcher's on_exit callback re-attaches its own group
    // and performs the reverse slide (which auto-deletes this screen).
    if (s_mock_group) {
        lv_indev_set_group(s_keypad, nullptr);
        lv_group_delete(s_mock_group);
        s_mock_group = nullptr;
    }
    if (s_on_exit) s_on_exit();
}

} // namespace

void mock_app_open(const char* name, lv_indev_t* keypad, void (*on_exit)()) {
    s_on_exit = on_exit;
    s_keypad  = keypad;

    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_size(scr, kScreenW, kScreenH);
    lv_obj_set_style_bg_color(scr, color_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // App name — centered, a few pixels above middle.
    lv_obj_t* name_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(name_lbl, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(name_lbl, color_text(), LV_PART_MAIN);
    lv_label_set_text(name_lbl, name);
    lv_obj_align(name_lbl, LV_ALIGN_CENTER, 0, -6);

    // "ESC TO EXIT" hint, a row below.
    lv_obj_t* hint_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(hint_lbl, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(hint_lbl, color_text(), LV_PART_MAIN);
    lv_label_set_text(hint_lbl, "ESC TO EXIT");
    lv_obj_align(hint_lbl, LV_ALIGN_CENTER, 0, 6);

    // LVGL keypad events only route via a group (indev drops them if group
    // is NULL), so create a one-object group containing the screen itself.
    // The screen's LV_EVENT_KEY handler catches ESC.
    s_mock_group = lv_group_create();
    lv_group_add_obj(s_mock_group, scr);
    lv_indev_set_group(keypad, s_mock_group);
    lv_obj_add_event_cb(scr, on_key, LV_EVENT_KEY, nullptr);

    // Slide-in animation, launcher is kept alive (auto_del=false).
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT,
                        kScreenTransitionMs, 0, false);
}

} // namespace cube
