#include "cube/launcher.hpp"
#include "cube/config.hpp"
#include "cube/lv_font_cube_6px.h"
#include "cube/detail/hostname_utils.hpp"
#include "internal.hpp"

#include <string>

namespace cube {

namespace {

enum class AppState { Launcher, InApp };

IPlatform*  g_plat            = nullptr;
lv_obj_t*   g_launcher_screen = nullptr;
lv_group_t* g_group           = nullptr;
AppState    g_state           = AppState::Launcher;

// -- State transitions ------------------------------------------------------

void return_to_launcher() {
    lv_indev_set_group(g_plat->keypad(), g_group);
    hue_focus_resume();
    g_state = AppState::Launcher;
    // auto_del=true → LVGL deletes the mock screen once the slide finishes.
    lv_screen_load_anim(g_launcher_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                        kScreenTransitionMs, 0, true);
}

void on_item_activate(lv_event_t* e) {
    if (g_state != AppState::Launcher) return;
    auto* item = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  id   = reinterpret_cast<intptr_t>(lv_obj_get_user_data(item));
    if (id < 0 || id >= kAppCount) return;

    hue_focus_pause();
    g_state = AppState::InApp;
    if (std::string_view{kApps[id].label} == "SETTINGS")
        settings_open(g_plat->keypad(), return_to_launcher);
    else
        mock_app_open(kApps[id].label, g_plat->keypad(), return_to_launcher);
}

// -- UI construction --------------------------------------------------------

lv_obj_t* make_list_item(lv_obj_t* parent, const AppEntry& app) {
    // Borderless / shadowless / radiusless button — pure pixel-art look.
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, kScreenW, kItemH);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    // Kill every piece of chrome the default theme adds — border, shadow,
    // bg, outline — across default/pressed/focused/focus-key so nothing
    // bleeds into the neighbouring rows (items are packed 10 px apart and
    // outlines draw OUTSIDE the object).
    for (lv_state_t st : {LV_STATE_DEFAULT, LV_STATE_PRESSED,
                          LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY}) {
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | st);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | st);
        lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | st);
        lv_obj_set_style_outline_pad(btn, 0, LV_PART_MAIN | st);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | st);
    }
    // Focus indicator: 1-pixel blue bars hugging top + bottom of the row.
    // Border is drawn INSIDE the object, so the bars stay within the 10 px
    // item bounds and never spill into neighbouring rows.
    for (lv_state_t st : {LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY}) {
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | st);
        lv_obj_set_style_border_color(btn, color_title(), LV_PART_MAIN | st);
        lv_obj_set_style_border_side(btn,
                                     static_cast<lv_border_side_t>(
                                         LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM),
                                     LV_PART_MAIN | st);
        lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | st);
    }
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, color_text(), LV_PART_MAIN);
    lv_label_set_text(lbl, app.label);
    lv_obj_set_style_pad_all(lbl, 0, LV_PART_MAIN);
    lv_obj_set_height(lbl, kTitleH);  // kTitleH==6 == font glyph height, force symmetric gap
    lv_obj_center(lbl);

    lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(app.id)));
    lv_obj_add_event_cb(btn, on_item_activate, LV_EVENT_CLICKED, nullptr);
    return btn;
}

void build_launcher_screen() {
    g_launcher_screen = lv_obj_create(nullptr);
    lv_obj_t* scr = g_launcher_screen;

    lv_obj_set_size(scr, kScreenW, kScreenH);
    lv_obj_set_style_bg_color(scr, color_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr, 0, LV_PART_MAIN);

    // --- Title row: blue hostname, circular scroll if overflow ---
    lv_obj_t* title = lv_label_create(scr);
    lv_obj_set_size(title, kScreenW, kTitleH);
    lv_obj_set_style_text_font(title, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, color_title(), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(title, cube::detail::normalize_hostname(g_plat->hostname()).c_str());

    // --- List container: 52 px, flex column, scrollable ---
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, kScreenW, kListH);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    g_group = lv_group_create();
    lv_group_set_wrap(g_group, true);
    for (const auto& app : kApps) {
        lv_obj_t* item = make_list_item(list, app);
        lv_group_add_obj(g_group, item);
    }
    lv_indev_set_group(g_plat->keypad(), g_group);

    // --- Info row: red "100%" right-aligned ---
    lv_obj_t* info = lv_obj_create(scr);
    lv_obj_set_size(info, kScreenW, kInfoH);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info, 0, LV_PART_MAIN);
    lv_obj_remove_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* battery = lv_label_create(info);
    lv_obj_set_style_text_font(battery, &lv_font_cube_6px, LV_PART_MAIN);
    lv_obj_set_style_text_color(battery, color_info(), LV_PART_MAIN);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", g_plat->battery_percent());
    lv_label_set_text(battery, buf);
    lv_obj_align(battery, LV_ALIGN_RIGHT_MID, 0, 0);
}

} // namespace

void launcher_init(IPlatform& plat) {
    g_plat = &plat;
    build_launcher_screen();

    lv_screen_load(g_launcher_screen);
    hue_focus_init(g_group);
    // Pick one startup animation (comment out the other):
    startup_anim_scanline_show(g_launcher_screen, nullptr);  // CRT scanline (center→edges)
    // startup_anim_phosphor_show([]() {});                  // phosphor warm-up
}

void launcher_deinit() {
    startup_anim_scanline_deinit();   // match whichever show() is active above
    // startup_anim_phosphor_deinit();
    hue_focus_deinit();
    if (g_group) {
        lv_group_delete(g_group);
        g_group = nullptr;
    }
    if (g_launcher_screen) {
        lv_obj_delete(g_launcher_screen);
        g_launcher_screen = nullptr;
    }
    g_plat = nullptr;
}

} // namespace cube
