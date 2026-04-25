#include "cube/config.hpp"
#include "internal.hpp"

namespace cube {

lv_obj_t* make_styled_label(lv_obj_t* parent, const char* text, lv_color_t color) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_pad_all(lbl, 0, LV_PART_MAIN);
    lv_obj_set_height(lbl, kTitleH);
    lv_obj_center(lbl);
    return lbl;
}

} // namespace cube
