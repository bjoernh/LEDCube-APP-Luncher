#include "internal.hpp"
#include "cube/config.hpp"
#include "lvgl.h"

// Phosphor warm-up: random pixels ignite in dim red, spread to neighbors,
// and heat through red → orange → yellow → white before transitioning.

namespace cube {

namespace {

static constexpr uint32_t kTimerMs          = 33;    // ~30 Hz
static constexpr uint32_t kIgnitionMs       = 1800;  // random sparks phase
static constexpr uint32_t kTotalMs          = 3200;  // full animation duration
static constexpr int      kIgnitionsPerTick = 5;     // new sparks per frame
static constexpr uint8_t  kSpreadMin        = 8;     // heat required to spread
static constexpr uint8_t  kSpreadStart      = 6;     // heat assigned to freshly spread px
static constexpr uint8_t  kHeatRate         = 4;     // per-tick increase for all heated px

// 4-byte aligned canvas buffer for RGB565 (2 bytes/px)
static uint32_t g_canvas_buf[(kScreenW * kScreenH * 2 + 3) / 4];
static uint8_t  g_heat[kScreenH][kScreenW];

lv_obj_t*   g_scr    = nullptr;
lv_obj_t*   g_canvas = nullptr;
lv_timer_t* g_timer  = nullptr;
uint32_t    g_start_ms   = 0;
uint32_t    g_rng        = 0x1234ABCD;
void (*g_on_complete)()  = nullptr;

uint32_t rng_next() {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return g_rng;
}

lv_color_t heat_to_color(uint8_t h) {
    if (h == 0) return lv_color_black();
    uint8_t r, g, b;
    if (h < 50) {
        // black → dim red
        r = static_cast<uint8_t>((static_cast<uint16_t>(h) * 80) / 50);
        g = 0; b = 0;
    } else if (h < 130) {
        // dim red → bright red
        r = static_cast<uint8_t>(80 + (static_cast<uint16_t>(h - 50) * 175) / 80);
        g = 0; b = 0;
    } else if (h < 190) {
        // red → orange
        r = 255;
        g = static_cast<uint8_t>((static_cast<uint16_t>(h - 130) * 140) / 60);
        b = 0;
    } else if (h < 230) {
        // orange → yellow
        r = 255;
        g = static_cast<uint8_t>(140 + (static_cast<uint16_t>(h - 190) * 115) / 40);
        b = 0;
    } else {
        // yellow → white
        r = 255; g = 255;
        uint16_t bv = (static_cast<uint16_t>(h - 230) * 255) / 25;
        b = (bv > 255) ? 255 : static_cast<uint8_t>(bv);
    }
    return lv_color_make(r, g, b);
}

void update_heat(uint32_t elapsed) {
    // 1. Spread from hot pixels to unheated neighbors
    for (int y = 0; y < kScreenH; ++y) {
        for (int x = 0; x < kScreenW; ++x) {
            if (g_heat[y][x] > 0) continue;
            uint8_t mx = 0;
            if (x > 0            && g_heat[y][x-1] > mx) mx = g_heat[y][x-1];
            if (x < kScreenW - 1 && g_heat[y][x+1] > mx) mx = g_heat[y][x+1];
            if (y > 0            && g_heat[y-1][x] > mx) mx = g_heat[y-1][x];
            if (y < kScreenH - 1 && g_heat[y+1][x] > mx) mx = g_heat[y+1][x];
            if (mx >= kSpreadMin) g_heat[y][x] = kSpreadStart;
        }
    }

    // 2. Random new sparks during ignition phase
    if (elapsed < kIgnitionMs) {
        for (int i = 0; i < kIgnitionsPerTick; ++i) {
            int px = static_cast<int>(rng_next() % static_cast<uint32_t>(kScreenW));
            int py = static_cast<int>(rng_next() % static_cast<uint32_t>(kScreenH));
            if (g_heat[py][px] == 0) g_heat[py][px] = kSpreadMin + 1;
        }
    }

    // 3. Heat up all burning pixels
    for (int y = 0; y < kScreenH; ++y) {
        for (int x = 0; x < kScreenW; ++x) {
            if (g_heat[y][x] > 0) {
                uint16_t nh = static_cast<uint16_t>(g_heat[y][x]) + kHeatRate;
                g_heat[y][x] = (nh > 255) ? 255 : static_cast<uint8_t>(nh);
            }
        }
    }
}

void draw_frame() {
    for (int y = 0; y < kScreenH; ++y)
        for (int x = 0; x < kScreenW; ++x)
            lv_canvas_set_px(g_canvas, x, y, heat_to_color(g_heat[y][x]), LV_OPA_COVER);
}

void anim_tick(lv_timer_t* /*t*/) {
    const uint32_t elapsed = lv_tick_get() - g_start_ms;

    if (elapsed >= kTotalMs) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
        g_scr   = nullptr;  // will be auto-deleted via auto_del=true
        if (g_on_complete) g_on_complete();
        return;
    }

    update_heat(elapsed);
    draw_frame();
}

} // namespace

void startup_anim_phosphor_show(void (*on_complete)()) {
    g_on_complete = on_complete;
    g_rng = lv_tick_get() ^ 0xDEADBEEFu;
    lv_memset(g_heat, 0, sizeof(g_heat));
    lv_memset(g_canvas_buf, 0, sizeof(g_canvas_buf));

    g_scr = lv_obj_create(nullptr);
    lv_obj_set_size(g_scr, kScreenW, kScreenH);
    lv_obj_set_style_bg_color(g_scr, color_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_scr, 0, LV_PART_MAIN);
    lv_obj_remove_flag(g_scr, LV_OBJ_FLAG_SCROLLABLE);

    g_canvas = lv_canvas_create(g_scr);
    lv_canvas_set_buffer(g_canvas, g_canvas_buf, kScreenW, kScreenH,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(g_canvas, 0, 0);
    lv_obj_set_style_pad_all(g_canvas, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_canvas, 0, LV_PART_MAIN);

    lv_screen_load(g_scr);

    g_start_ms = lv_tick_get();
    g_timer    = lv_timer_create(anim_tick, kTimerMs, nullptr);
}

void startup_anim_phosphor_deinit() {
    if (g_timer) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
    }
    if (g_scr) {
        lv_obj_delete(g_scr);
        g_scr = nullptr;
    }
    g_canvas      = nullptr;
    g_on_complete = nullptr;
}

} // namespace cube
