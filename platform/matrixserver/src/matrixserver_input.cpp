#include "matrixserver_internal.hpp"

#include <Joystick.h>

namespace cube::ms {

namespace {

// JoystickManager owns up to 8 joystick slots — mirrors MainMenu/mainmenu.cpp.
JoystickManager s_joymngr(8);

// LVGL keypad state machine.
// The *Press accessors in JoystickManager are already edge-triggered (cleared
// by clearAllButtonPresses), so we simply latch the key for one read cycle:
//   pump_joystick() sets s_pending_key + s_state = PRESSED
//   read_cb() delivers PRESSED on that call, then clears to RELEASED
uint32_t                s_pending_key = 0;
lv_indev_state_t        s_state       = LV_INDEV_STATE_RELEASED;

void read_cb(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    data->key   = s_pending_key;
    data->state = s_state;

    // One-shot: after delivering PRESSED, immediately revert to RELEASED so
    // LVGL fires exactly one navigation event per joystick press.
    if (s_state == LV_INDEV_STATE_PRESSED) {
        s_state = LV_INDEV_STATE_RELEASED;
    }
}

} // namespace

lv_indev_t* input_init() {
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, read_cb);
    return indev;
}

void input_deinit() {
    // Nothing SDL-owned here; LVGL indevs are cleaned up in lv_deinit().
}

// Poll the JoystickManager and latch the highest-priority key for this frame.
// Priority: vertical axis > horizontal axis > button 0 > button 1.
// Matches MainMenu/mainmenu.cpp drain-each-frame pattern.
void pump_joystick() {
    const float vAxis = s_joymngr.getAxisPress(1);  // vertical
    const float hAxis = s_joymngr.getAxisPress(0);  // horizontal

    uint32_t key = 0;

    if (vAxis > 0.0f) {
        key = LV_KEY_NEXT;
    } else if (vAxis < 0.0f) {
        key = LV_KEY_PREV;
    } else if (hAxis > 0.0f) {
        key = LV_KEY_RIGHT;
    } else if (hAxis < 0.0f) {
        key = LV_KEY_LEFT;
    } else if (s_joymngr.getButtonPress(0)) {
        key = LV_KEY_ENTER;
    } else if (s_joymngr.getButtonPress(1)) {
        key = LV_KEY_ESC;
    }

    if (key != 0) {
        s_pending_key = key;
        s_state       = LV_INDEV_STATE_PRESSED;
    }

    // Drain edge-triggered button/axis presses — same as mainmenu.cpp line 189.
    s_joymngr.clearAllButtonPresses();
}

} // namespace cube::ms
