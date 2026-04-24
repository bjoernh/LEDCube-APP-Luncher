#include "cube/platform.hpp"

namespace cube {

void install_tick_source(uint32_t (*tick_ms_fn)()) {
    // LVGL v9 takes a C function pointer with signature `uint32_t (void)`.
    // The platform's tick source must match — no wrapping needed.
    lv_tick_set_cb(tick_ms_fn);
}

} // namespace cube
