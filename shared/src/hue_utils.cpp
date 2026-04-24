#include "cube/detail/hue_utils.hpp"

namespace cube::detail {

lv_color_t hsv_to_color(int h, float s, float v) {
    const float c   = v * s;
    const float hh  = static_cast<float>(h) / 60.0f;
    const float hm2 = hh - 2.0f * static_cast<int>(hh / 2.0f);
    const float x   = c * (1.0f - (hm2 < 1.0f ? 1.0f - hm2 : hm2 - 1.0f));
    float r = 0, g = 0, b = 0;
    switch (static_cast<int>(hh) % 6) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        case 5: r = c; g = 0; b = x; break;
    }
    const float m = v - c;
    return lv_color_make(static_cast<uint8_t>((r + m) * 255.0f),
                         static_cast<uint8_t>((g + m) * 255.0f),
                         static_cast<uint8_t>((b + m) * 255.0f));
}

} // namespace cube::detail
