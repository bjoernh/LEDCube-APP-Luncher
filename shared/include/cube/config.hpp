#pragma once

#include <cstdint>
#include "lvgl.h"

namespace cube {

// --- Display geometry ---
inline constexpr int32_t kScreenW = 64;
inline constexpr int32_t kScreenH = 64;

// --- Row layout (title / list / info) ---
inline constexpr int32_t kTitleH = 6;
inline constexpr int32_t kInfoH  = 6;
inline constexpr int32_t kListH  = kScreenH - kTitleH - kInfoH;   // 52
inline constexpr int32_t kItemH  = 10;                             // 5 items visible in 52 px

// --- Colors (tune freely; plan says these are constants for easy adjustment) ---
inline lv_color_t color_title() { return lv_color_hex(0x0000FF); } // pure blue per spec
inline lv_color_t color_info()  { return lv_color_hex(0xFF0000); } // red
inline lv_color_t color_bg()    { return lv_color_hex(0x000000); } // black
inline lv_color_t color_text()  { return lv_color_hex(0xFFFFFF); } // white

// --- Animation / timing ---
inline constexpr uint32_t kFocusHuePeriodMs    = 4000;  // full rainbow cycle
inline constexpr uint32_t kHueTimerPeriodMs    = 33;    // ~30 Hz update
inline constexpr uint32_t kScreenTransitionMs  = 300;

} // namespace cube
