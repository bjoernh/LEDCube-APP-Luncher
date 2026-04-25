#include "matrixserver_internal.hpp"

#include <Screen.h>

#include <cstring>
#include <memory>
#include <vector>

namespace cube::ms {

namespace {

// Two separate buffers:
//   s_lv_buf   — LVGL's working scratch area (registered via lv_display_set_buffers).
//                LVGL writes each dirty region's pixels here starting at offset 0.
//   s_framebuf — our composited frame, read by copy_to_screens. Preserves contents
//                across frames so a partial flush only touches the dirty rows.
// Using s_framebuf for both roles is broken: when LVGL renders a partial region
// (e.g. one scrolled row), it clobbers s_framebuf[0..] with the new strip data
// before flush_cb runs, leaving the rows above the dirty area corrupted.
constexpr uint32_t kBufPixels = kW * kH;
alignas(8) uint16_t s_lv_buf[kBufPixels];
alignas(8) uint16_t s_framebuf[kBufPixels];

// LVGL flush callback: copy the dirty sub-rectangle from LVGL's scratch buffer
// (px_map) into our persistent s_framebuf at the correct on-screen offset.
// We do NOT push to screens here — the wrapper app does that after
// lv_timer_handler() returns.
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const int x1 = area->x1;
    const int y1 = area->y1;
    const int x2 = area->x2;
    const int y2 = area->y2;
    const int w  = x2 - x1 + 1;

    // px_map is packed per-area (LVGL PARTIAL mode), so each row is `w` pixels.
    const uint16_t* src = reinterpret_cast<const uint16_t*>(px_map);
    for (int y = y1; y <= y2; ++y) {
        std::memcpy(&s_framebuf[y * kW + x1], src, static_cast<size_t>(w) * 2);
        src += w;
    }

    lv_display_flush_ready(disp);
}

} // namespace

lv_display_t* display_init() {
    lv_display_t* disp = lv_display_create(kW, kH);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, s_lv_buf, nullptr,
                           sizeof(s_lv_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}

void display_deinit() {
    // Nothing to free — s_framebuf is static. LVGL objects are cleaned up
    // by the caller (lv_deinit or display object deletion).
}

// Convert s_framebuf (RGB565) to RGB888 once, then push to the four side
// screens (front=0, right=1, back=2, left=3) via Screen::setPixel.
// Screens 4 (top) and 5 (bottom) are intentionally never touched.
void copy_to_screens(std::vector<std::shared_ptr<Screen>>& screens) {
    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            const uint16_t p  = s_framebuf[y * kW + x];
            const uint8_t  r5 = (p >> 11) & 0x1F;
            const uint8_t  g6 = (p >>  5) & 0x3F;
            const uint8_t  b5 = (p      ) & 0x1F;

            // Bit-replication: exact endpoints, no division.
            const uint8_t r8 = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
            const uint8_t g8 = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
            const uint8_t b8 = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));

            // Mirror to front(0), right(1), back(2), left(3).
            // Screens 4 (top) and 5 (bottom) are never indexed.
            for (std::size_t s = 0; s < 4 && s < screens.size(); ++s) {
                screens[s]->setPixel(x, y, r8, g8, b8);
            }
        }
    }
}

} // namespace cube::ms
