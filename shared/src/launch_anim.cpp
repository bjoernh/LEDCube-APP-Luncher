#include "internal.hpp"
#include "cube/config.hpp"
#include "lvgl.h"

// App-launch sequence:
//  1) invert the focused row (white bg, black text) for kInvertMs while
//     hue_focus is paused so it doesn't fight the inverted color,
//  2) reverse CRT scanline sweep — two scanlines start at the screen edges
//     and converge on center, growing black masks behind them until the
//     screen is fully black except for the 1 px center scanlines,
//  3) brief hold, then on_complete() (caller fires launch_app + resumes hue).

namespace cube {

namespace {

static constexpr uint32_t kInvertMs = 300;  // row inversion phase
static constexpr uint32_t kSweepMs  = 700;  // scanline edge -> center
static constexpr uint32_t kHoldMs   = 100;  // black hold before on_complete
static constexpr uint32_t kTimerMs  = 16;   // ~60 Hz

// Phase 1 — row inversion
lv_obj_t*   g_focused_item  = nullptr;  // row currently inverted
lv_obj_t*   g_focused_label = nullptr;  // its label child (recolored to black)
lv_timer_t* g_invert_timer  = nullptr;  // one-shot timer that ends phase 1

// Phase 2 — scanline sweep overlay (mirrors startup_anim.cpp)
lv_obj_t*   g_overlay      = nullptr;
lv_obj_t*   g_mask_top     = nullptr;
lv_obj_t*   g_mask_bot     = nullptr;
lv_obj_t*   g_glow_top     = nullptr;
lv_obj_t*   g_glow_bot     = nullptr;
lv_obj_t*   g_scanline_top = nullptr;
lv_obj_t*   g_scanline_bot = nullptr;
lv_timer_t* g_sweep_timer  = nullptr;
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

// Restore the row to its canonical pre-invert appearance: transparent bg,
// white label text. The launcher creates rows in exactly this state.
void restore_row_styles() {
    if (g_focused_item) {
        lv_obj_set_style_bg_opa(g_focused_item, LV_OPA_TRANSP, LV_PART_MAIN);
    }
    if (g_focused_label) {
        lv_obj_set_style_text_color(g_focused_label, color_text(), LV_PART_MAIN);
    }
}

void sweep_tick(lv_timer_t* /*t*/) {
    const uint32_t elapsed = lv_tick_get() - g_start_ms;

    if (elapsed >= kSweepMs + kHoldMs) {
        lv_timer_delete(g_sweep_timer);
        g_sweep_timer = nullptr;
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

        hue_focus_resume();

        void (*cb)() = g_on_complete;
        g_on_complete = nullptr;
        g_focused_item  = nullptr;
        g_focused_label = nullptr;
        if (cb) cb();
        return;
    }

    if (elapsed >= kSweepMs) return;  // hold — geometry frozen at full black

    const int32_t center = kScreenH / 2;
    const int32_t offset = static_cast<int32_t>((elapsed * center) / kSweepMs);

    // Reverse of startup_anim: top scanline travels 0 -> center,
    // bottom scanline travels (kScreenH - 1) -> center.
    const int32_t y_top = offset;
    const int32_t y_bot = kScreenH - 1 - offset;

    // Black masks grow inward from the edges
    lv_obj_set_pos(g_mask_top, 0, 0);
    lv_obj_set_size(g_mask_top, kScreenW, LV_MAX(0, y_top));

    lv_obj_set_pos(g_mask_bot, 0, LV_MIN(kScreenH, y_bot + 1));
    lv_obj_set_size(g_mask_bot, kScreenW, LV_MAX(0, kScreenH - y_bot - 1));

    // Glow bloom 2 px ahead of each scanline, in direction of travel (toward center).
    // Top scanline moves down, so glow sits BELOW it.
    const int32_t glow_top_end = LV_MIN(kScreenH, y_top + 3);
    lv_obj_set_pos(g_glow_top, 0, y_top + 1);
    lv_obj_set_size(g_glow_top, kScreenW, LV_MAX(0, glow_top_end - (y_top + 1)));

    // Bottom scanline moves up, so glow sits ABOVE it.
    const int32_t glow_bot_y = LV_MAX(0, y_bot - 2);
    lv_obj_set_pos(g_glow_bot, 0, glow_bot_y);
    lv_obj_set_size(g_glow_bot, kScreenW, LV_MAX(0, y_bot - glow_bot_y));

    // Pure white 1 px scanlines spanning the full width
    lv_obj_set_pos(g_scanline_top, 0, y_top);
    lv_obj_set_size(g_scanline_top, kScreenW, 1);

    lv_obj_set_pos(g_scanline_bot, 0, y_bot);
    lv_obj_set_size(g_scanline_bot, kScreenW, 1);
}

void start_sweep_phase() {
    lv_obj_t* parent = lv_screen_active();

    g_overlay = lv_obj_create(parent);
    lv_obj_set_size(g_overlay, kScreenW, kScreenH);
    lv_obj_set_pos(g_overlay, 0, 0);
    lv_obj_set_style_bg_opa(g_overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(g_overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(g_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Masks, glow, scanlines all start zero-sized — sweep_tick grows them.
    g_mask_top     = make_rect(g_overlay, color_bg());
    g_mask_bot     = make_rect(g_overlay, color_bg());
    g_glow_top     = make_rect(g_overlay, lv_color_hex(0x80C4FF));
    g_glow_bot     = make_rect(g_overlay, lv_color_hex(0x80C4FF));
    g_scanline_top = make_rect(g_overlay, lv_color_hex(0xFFFFFF));
    g_scanline_bot = make_rect(g_overlay, lv_color_hex(0xFFFFFF));

    // Place scanlines at the edges immediately so the first frame isn't blank.
    lv_obj_set_pos(g_scanline_top, 0, 0);
    lv_obj_set_size(g_scanline_top, kScreenW, 1);
    lv_obj_set_pos(g_scanline_bot, 0, kScreenH - 1);
    lv_obj_set_size(g_scanline_bot, kScreenW, 1);

    g_start_ms = lv_tick_get();
    g_sweep_timer = lv_timer_create(sweep_tick, kTimerMs, nullptr);
}

void invert_done(lv_timer_t* /*t*/) {
    lv_timer_delete(g_invert_timer);
    g_invert_timer = nullptr;
    restore_row_styles();
    start_sweep_phase();
}

} // namespace

void launch_animation_play(lv_obj_t* focused_item, void (*on_complete)()) {
    g_on_complete = on_complete;

    // Stop the rainbow hue cycle so it doesn't repaint the inverted label.
    hue_focus_pause();

    g_focused_item  = focused_item;
    g_focused_label = nullptr;
    if (focused_item && lv_obj_get_child_count(focused_item) > 0) {
        g_focused_label = lv_obj_get_child(focused_item, 0);
    }

    // Phase 1 — invert: white opaque bg, black label text.
    if (g_focused_item) {
        lv_obj_set_style_bg_color(g_focused_item, color_text(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_focused_item, LV_OPA_COVER, LV_PART_MAIN);
    }
    if (g_focused_label) {
        lv_obj_set_style_text_color(g_focused_label, color_bg(), LV_PART_MAIN);
    }

    g_invert_timer = lv_timer_create(invert_done, kInvertMs, nullptr);
}

void launch_animation_deinit() {
    if (g_invert_timer) {
        lv_timer_delete(g_invert_timer);
        g_invert_timer = nullptr;
    }
    if (g_sweep_timer) {
        lv_timer_delete(g_sweep_timer);
        g_sweep_timer = nullptr;
    }
    if (g_overlay) {
        lv_obj_delete(g_overlay);
        g_overlay = nullptr;
    }
    g_mask_top      = nullptr;
    g_mask_bot      = nullptr;
    g_glow_top      = nullptr;
    g_glow_bot      = nullptr;
    g_scanline_top  = nullptr;
    g_scanline_bot  = nullptr;
    g_focused_item  = nullptr;
    g_focused_label = nullptr;
    g_on_complete   = nullptr;
    g_start_ms      = 0;
}

} // namespace cube
