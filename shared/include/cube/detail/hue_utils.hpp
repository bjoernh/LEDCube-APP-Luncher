#pragma once
#include "lvgl.h"

namespace cube::detail {

lv_color_t hsv_to_color(int h, float s = 1.0f, float v = 1.0f);

} // namespace cube::detail
