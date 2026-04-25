#include "cube/matrixserver/matrixserver_platform.hpp"
#include "matrixserver_internal.hpp"

#include <memory>
#include <vector>
#include <chrono>
#include <unistd.h>   // usleep

class Screen;

namespace cube::ms {

uint32_t monotonic_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
    );
}

} // namespace cube::ms

namespace cube {

MatrixServerPlatform::MatrixServerPlatform(
    std::vector<std::shared_ptr<Screen>>& screens)
    : screens_(screens)
{
    // Tick source must be installed before any LVGL display/animation code
    // (display_init and input_init both call into LVGL internals).
    install_tick_source(&ms::monotonic_ms);

    display_ = ms::display_init();
    keypad_  = ms::input_init();
}

MatrixServerPlatform::~MatrixServerPlatform() {
    ms::input_deinit();
    ms::display_deinit();
}

lv_display_t* MatrixServerPlatform::display() {
    return display_;
}

lv_indev_t* MatrixServerPlatform::keypad() {
    return keypad_;
}

void MatrixServerPlatform::pump_events() {
    ms::pump_joystick();
}

bool MatrixServerPlatform::should_quit() const {
    // Lifecycle owned by MatrixApplication — never quit from platform side.
    return false;
}

void MatrixServerPlatform::sleep_ms(uint32_t ms) {
    ::usleep(static_cast<useconds_t>(ms) * 1000u);
}

std::string MatrixServerPlatform::hostname() {
    return ms::read_hostname();
}

int MatrixServerPlatform::battery_percent() {
    return ms::read_battery_percent();
}

void MatrixServerPlatform::copy_framebuf_to_screens() {
    ms::copy_to_screens(screens_);
}

} // namespace cube
