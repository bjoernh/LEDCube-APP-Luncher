#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* Monospace 5x5 font by maurycyz (https://maurycyz.com/projects/mcufont/),
 * wrapped as an LVGL font. Advance width is 6 px (5 px glyph + 1 px space),
 * line height is 6 px, so it drops into the launcher's 6 px title/info rows.
 *
 * Coverage: space, A-Z, a-z, 0-9, and - + . , ? ! : = ' %
 *
 * Regenerate with: python3 tools/gen_5px_font.py
 */
extern const lv_font_t lv_font_cube_5px;

#ifdef __cplusplus
}
#endif
