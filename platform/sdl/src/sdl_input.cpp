#include "sdl_internal.hpp"

namespace cube::sdl {

namespace {

// State updated by SDL event pump, read by LVGL read_cb.
uint32_t s_last_key       = 0;
bool     s_pressed        = false;
bool     s_quit_requested = false;

uint32_t map_key(SDL_Keycode k) {
    switch (k) {
        // LVGL groups only move focus on NEXT/PREV — arrows alone would
        // just be forwarded to the focused widget and do nothing here.
        case SDLK_UP:     return LV_KEY_PREV;
        case SDLK_DOWN:   return LV_KEY_NEXT;
        case SDLK_LEFT:   return LV_KEY_LEFT;
        case SDLK_RIGHT:  return LV_KEY_RIGHT;
        case SDLK_RETURN: return LV_KEY_ENTER;
        case SDLK_ESCAPE: return LV_KEY_ESC;
        default:          return 0;
    }
}

void read_cb(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    data->key   = s_last_key;
    data->state = s_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
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

void input_pump_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                s_quit_requested = true;
                break;

            case SDL_KEYDOWN: {
                const uint32_t mapped = map_key(ev.key.keysym.sym);
                if (mapped != 0) {
                    s_last_key = mapped;
                    s_pressed  = true;
                }
                break;
            }
            case SDL_KEYUP: {
                const uint32_t mapped = map_key(ev.key.keysym.sym);
                if (mapped != 0 && mapped == s_last_key) {
                    s_pressed = false;
                }
                break;
            }
            default: break;
        }
    }
}

bool input_quit_requested() {
    return s_quit_requested;
}

} // namespace cube::sdl
