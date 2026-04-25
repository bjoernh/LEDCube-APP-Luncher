#pragma once

#include "cube/platform.hpp"
#include <memory>
#include <vector>

class Screen;

namespace cube {

// Concrete IPlatform for matrixserver (LED cube hardware).
// Renders the 64×64 LVGL canvas to the four side screens (front/right/back/left).
// The top and bottom screens are never touched.
//
// Construction order: install_tick_source → display_init → input_init.
// The caller must call lv_init() before constructing this object.
//
// The wrapper app's loop() must call:
//   1. pump_events()          — poll joystick, update LVGL keypad state
//   2. lv_timer_handler()     — drive LVGL (may invoke flush_cb internally)
//   3. copy_framebuf_to_screens() — blit the RGB565 framebuf to 4 side screens
//
// screens is passed explicitly so the platform never needs to access the
// protected MatrixApplication::screens member directly.  The wrapper app
// (which inherits MatrixApplication) accesses its own protected member and
// forwards the reference here — no framework modification needed.
class MatrixServerPlatform : public IPlatform {
public:
    explicit MatrixServerPlatform(
        std::vector<std::shared_ptr<Screen>>& screens);
    ~MatrixServerPlatform() override;

    MatrixServerPlatform(const MatrixServerPlatform&)            = delete;
    MatrixServerPlatform& operator=(const MatrixServerPlatform&) = delete;

    lv_display_t* display() override;
    lv_indev_t*   keypad()  override;

    // Poll JoystickManager and update the LVGL keypad state machine.
    void pump_events() override;

    // Always returns false — lifecycle is owned by MatrixApplication.
    bool should_quit() const override;

    // Busy-sleep via usleep.
    void sleep_ms(uint32_t ms) override;

    std::string hostname() override;
    int         battery_percent() override;

    // Convert the internal RGB565 framebuffer to RGB888 and push to the four
    // side screens (front=0, right=1, back=2, left=3) via Screen::setPixel.
    // Screens 4 (top) and 5 (bottom) are never indexed.
    void copy_framebuf_to_screens();

private:
    std::vector<std::shared_ptr<Screen>>& screens_;
    lv_display_t*                         display_ = nullptr;
    lv_indev_t*                           keypad_  = nullptr;
};

} // namespace cube
