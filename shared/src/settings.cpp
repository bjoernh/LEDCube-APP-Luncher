#include "cube/config.hpp"
#include "internal.hpp"

namespace cube {

namespace {

enum class Kind { Info, Slider, Checkbox, Submenu, Button };

// Persistent state for stateful entries — survives close/re-open of Settings.
int  s_brightness      = 50;
int  s_imu_sensitivity = 50;
bool s_ap_mode         = false;
bool s_force_1p        = false;

struct SettingsEntry {
    Kind        kind;
    const char* label;
    const char* slider_prefix;  // Slider only, shown while focused (e.g. "BRI..")
    int*        slider_value;   // Slider only
    bool*       checkbox_state; // Checkbox only
};

const SettingsEntry kEntries[] = {
    {Kind::Info,     "INFO",       nullptr,  nullptr,             nullptr},
    {Kind::Slider,   "BRIGHTNESS", "BRI..",  &s_brightness,       nullptr},
    {Kind::Slider,   "IMU SENS",   "SEN..",  &s_imu_sensitivity,  nullptr},
    {Kind::Checkbox, "AP-MODE",    nullptr,  nullptr,             &s_ap_mode},
    {Kind::Checkbox, "FORCE 1P",   nullptr,  nullptr,             &s_force_1p},
    {Kind::Submenu,  "DEBUG",      nullptr,  nullptr,             nullptr},
    {Kind::Button,   "UPDATE",     nullptr,  nullptr,             nullptr},
    {Kind::Button,   "SHUTDOWN",   nullptr,  nullptr,             nullptr},
    {Kind::Button,   "RESTART",    nullptr,  nullptr,             nullptr},
};
constexpr int kEntryCount = static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));

lv_indev_t*  s_keypad     = nullptr;
void       (*s_on_exit)() = nullptr;
lv_obj_t*    s_screen     = nullptr;
lv_group_t*  s_group      = nullptr;

void exit_settings() {
    exit_screen_group(s_group, s_keypad, s_on_exit);
}

void return_to_settings() {
    lv_indev_set_group(s_keypad, s_group);
    lv_screen_load_anim(s_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                        kScreenTransitionMs, 0, true);
}

void on_item_key(lv_event_t* e) {
    if (lv_event_get_key(e) == LV_KEY_ESC) exit_settings();
}

void on_item_click(lv_event_t* e) {
    auto* item = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  idx  = reinterpret_cast<intptr_t>(lv_obj_get_user_data(item));
    const SettingsEntry& entry = kEntries[idx];

    switch (entry.kind) {
        case Kind::Info:
            info_open(s_keypad, return_to_settings);
            break;
        case Kind::Submenu:
            debug_open(s_keypad, return_to_settings);
            break;
        case Kind::Checkbox: {
            *entry.checkbox_state = !*entry.checkbox_state;
            auto* lbl = lv_obj_get_child(item, 0);
            if (lbl) {
                lv_label_set_text_fmt(lbl, "%s: %s", entry.label,
                                      *entry.checkbox_state ? "ON" : "OFF");
            }
            break;
        }
        case Kind::Button:
            mock_app_open(entry.label, s_keypad, return_to_settings);
            break;
        case Kind::Slider:
            // Sliders respond to arrow keys, not clicks.
            break;
    }
}

void on_slider_value_changed(lv_event_t* e) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  idx    = reinterpret_cast<intptr_t>(lv_obj_get_user_data(slider));
    const SettingsEntry& entry = kEntries[idx];
    auto* lbl    = lv_obj_get_child(slider, 0);
    if (!lbl) return;

    int32_t snapped = snap_slider_value(lv_slider_get_value(slider));
    lv_slider_set_value(slider, snapped, LV_ANIM_OFF);
    *entry.slider_value = snapped;
    lv_label_set_text_fmt(lbl, "%s: %d", entry.slider_prefix, snapped);
}

void on_slider_focused(lv_event_t* e) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  idx    = reinterpret_cast<intptr_t>(lv_obj_get_user_data(slider));
    const SettingsEntry& entry = kEntries[idx];
    auto* lbl    = lv_obj_get_child(slider, 0);
    if (!lbl) return;
    lv_label_set_text_fmt(lbl, "%s: %d", entry.slider_prefix,
                          lv_slider_get_value(slider));
}

void on_slider_defocused(lv_event_t* e) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto  idx    = reinterpret_cast<intptr_t>(lv_obj_get_user_data(slider));
    const SettingsEntry& entry = kEntries[idx];
    auto* lbl    = lv_obj_get_child(slider, 0);
    if (!lbl) return;
    lv_label_set_text(lbl, entry.label);
}

lv_obj_t* make_entry(lv_obj_t* parent, const SettingsEntry& entry, int idx) {
    lv_obj_t* item = (entry.kind == Kind::Slider) ? lv_slider_create(parent)
                                                  : lv_button_create(parent);

    lv_obj_set_size(item, kScreenW, kItemH);
    lv_obj_set_style_pad_all(item, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(item, 0, LV_PART_MAIN);

    if (entry.kind == Kind::Slider) {
        lv_slider_set_range(item, 0, 100);
        lv_slider_set_value(item, *entry.slider_value, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(item, color_title(), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(item, 0, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, LV_PART_KNOB);
    }

    remove_widget_chrome(item, entry.kind != Kind::Slider);
    apply_focus_indicator(item, color_title());
    lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = make_styled_label(item, "", color_text());
    if (entry.kind == Kind::Checkbox) {
        lv_label_set_text_fmt(lbl, "%s: %s", entry.label,
                              *entry.checkbox_state ? "ON" : "OFF");
    } else {
        lv_label_set_text(lbl, entry.label);
    }

    lv_obj_set_user_data(item, reinterpret_cast<void*>(static_cast<intptr_t>(idx)));
    if (entry.kind == Kind::Slider) {
        lv_obj_add_event_cb(item, on_slider_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(item, on_slider_focused,       LV_EVENT_FOCUSED,       nullptr);
        lv_obj_add_event_cb(item, on_slider_defocused,     LV_EVENT_DEFOCUSED,     nullptr);
    } else {
        lv_obj_add_event_cb(item, on_item_click, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_add_event_cb(item, on_item_key, LV_EVENT_KEY, nullptr);
    return item;
}

} // namespace

void settings_open(lv_indev_t* keypad, void (*on_exit)()) {
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
    lv_obj_set_style_text_color(title, color_title(), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(title, "SETTINGS");

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
        lv_obj_t* item = make_entry(list, kEntries[i], i);
        lv_group_add_obj(s_group, item);
    }
    lv_indev_set_group(keypad, s_group);

    // Info row (empty — layout consistency with launcher)
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
