#include "internal.hpp"
#include "cube/config.hpp"
#include "lvgl.h"

// CRT scanline warm-up: two black masks cover the already-loaded launcher screen.
// Two bright lines burst from center, sweeping the masks away (up + down) and
// revealing the menu live behind them. No separate splash screen needed.

namespace cube {

namespace {

static constexpr uint32_t kSweepMs = 700;  // time for lines to reach the edges
static constexpr uint32_t kHoldMs  = 100;  // brief pause before overlay is deleted
static constexpr uint32_t kTimerMs = 16;   // ~60 Hz

lv_obj_t*   g_overlay      = nullptr;  // full-screen container parented to launcher
lv_obj_t*   g_mask_top     = nullptr;  // black rect hiding the upper half
lv_obj_t*   g_mask_bot     = nullptr;  // black rect hiding the lower half
lv_obj_t*   g_glow_top     = nullptr;  // bright bloom ahead of upward scanline
lv_obj_t*   g_glow_bot     = nullptr;  // bright bloom ahead of downward scanline
lv_obj_t*   g_scanline_top = nullptr;  // 1 px white line sweeping up
lv_obj_t*   g_scanline_bot = nullptr;  // 1 px white line sweeping down
lv_timer_t* g_timer        = nullptr;
uint32_t    g_start_ms     = 0;
void (*g_on_complete)()    = nullptr;

lv_obj_t* make_rect(lv_obj_t* parent, lv_color_t color) {
    lv_obj_t* r = lv_obj_create(parent);
    lv_obj_set_style_bg_color(r, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(r, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(r, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(r, 0, LV_PART_MAIN);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_set_size(r, 0, 0);
    return r;
}

void anim_tick(lv_timer_t* /*t*/) {
    const uint32_t elapsed = lv_tick_get() - g_start_ms;

    if (elapsed >= kSweepMs + kHoldMs) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
        // Remove overlay — launcher is fully visible now
        lv_obj_delete(g_overlay);
        g_overlay = nullptr;
        if (g_on_complete) g_on_complete();
        return;
    }

    if (elapsed >= kSweepMs) return;  // hold — nothing to move

    const int32_t center = kScreenH / 2;
    const int32_t offset = static_cast<int32_t>((elapsed * center) / kSweepMs);

    const int32_t y_top = center - offset;  // top scanline, sweeping toward row 0
    const int32_t y_bot = center + offset;  // bot scanline, sweeping toward row kScreenH

    // Black masks shrink away, revealing launcher behind them
    lv_obj_set_size(g_mask_top, kScreenW, LV_MAX(0, y_top));
    lv_obj_set_pos(g_mask_bot, 0, LV_MIN(kScreenH, y_bot + 1));
    lv_obj_set_size(g_mask_bot, kScreenW, LV_MAX(0, kScreenH - y_bot - 1));

    // Glow bloom ahead of top scanline (2 px above, direction of travel)
    const int32_t glow_top_y = LV_MAX(0, y_top - 2);
    lv_obj_set_pos(g_glow_top, 0, glow_top_y);
    lv_obj_set_size(g_glow_top, kScreenW, y_top - glow_top_y);

    // Glow bloom ahead of bottom scanline (2 px below)
    const int32_t glow_bot_end = LV_MIN(kScreenH, y_bot + 3);
    lv_obj_set_pos(g_glow_bot, 0, y_bot + 1);
    lv_obj_set_size(g_glow_bot, kScreenW, glow_bot_end - (y_bot + 1));

    // Scanlines — 1 px pure white each
    lv_obj_set_pos(g_scanline_top, 0, y_top);
    lv_obj_set_size(g_scanline_top, kScreenW, 1);

    lv_obj_set_pos(g_scanline_bot, 0, y_bot);
    lv_obj_set_size(g_scanline_bot, kScreenW, 1);
}

} // namespace

void startup_anim_scanline_show(lv_obj_t* parent, void (*on_complete)()) {
    g_on_complete = on_complete;

    // Full-screen overlay container parented to the already-loaded launcher screen.
    // IGNORE_LAYOUT keeps it out of the launcher's flex flow.
    g_overlay = lv_obj_create(parent);
    lv_obj_set_size(g_overlay, kScreenW, kScreenH);
    lv_obj_set_pos(g_overlay, 0, 0);
    lv_obj_set_style_bg_opa(g_overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(g_overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(g_overlay, LV_OBJ_FLAG_SCROLLABLE);

    const int32_t center = kScreenH / 2;

    // Black masks — initially cover the full screen (top half + bottom half)
    g_mask_top = make_rect(g_overlay, color_bg());
    lv_obj_set_size(g_mask_top, kScreenW, center);

    g_mask_bot = make_rect(g_overlay, color_bg());
    lv_obj_set_pos(g_mask_bot, 0, center);
    lv_obj_set_size(g_mask_bot, kScreenW, kScreenH - center);

    // Glow and scanlines start at center (zero size — grow on first tick)
    g_glow_top     = make_rect(g_overlay, lv_color_hex(0x80C4FF));
    g_glow_bot     = make_rect(g_overlay, lv_color_hex(0x80C4FF));
    g_scanline_top = make_rect(g_overlay, lv_color_hex(0xFFFFFF));
    g_scanline_bot = make_rect(g_overlay, lv_color_hex(0xFFFFFF));

    g_start_ms = lv_tick_get();
    g_timer    = lv_timer_create(anim_tick, kTimerMs, nullptr);
}

void startup_anim_scanline_deinit() {
    if (g_timer) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
    }
    if (g_overlay) {
        lv_obj_delete(g_overlay);
        g_overlay = nullptr;
    }
    g_mask_top     = nullptr;
    g_mask_bot     = nullptr;
    g_glow_top     = nullptr;
    g_glow_bot     = nullptr;
    g_scanline_top = nullptr;
    g_scanline_bot = nullptr;
    g_on_complete  = nullptr;
}

} // namespace cube
