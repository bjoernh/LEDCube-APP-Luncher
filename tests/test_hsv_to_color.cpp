#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include "cube/detail/hue_utils.hpp"

using cube::detail::hsv_to_color;

static bool color_near(lv_color_t a, lv_color_t b, int tol = 1) {
    auto diff = [](uint8_t x, uint8_t y) { return std::abs((int)x - (int)y); };
    return diff(a.red, b.red) <= tol
        && diff(a.green, b.green) <= tol
        && diff(a.blue, b.blue) <= tol;
}

TEST_CASE("h=0 is pure red", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(0, 1.0f, 1.0f)) == 0xF800u);
}

TEST_CASE("h=120 is pure green", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(120, 1.0f, 1.0f)) == 0x07E0u);
}

TEST_CASE("h=240 is pure blue", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(240, 1.0f, 1.0f)) == 0x001Fu);
}

TEST_CASE("h=60 is yellow", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(60, 1.0f, 1.0f)) == 0xFFE0u);
}

TEST_CASE("h=180 is cyan", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(180, 1.0f, 1.0f)) == 0x07FFu);
}

TEST_CASE("h=300 is magenta", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(300, 1.0f, 1.0f)) == 0xF81Fu);
}

TEST_CASE("h=359 is near-red: red=255, green=0, blue small", "[hsv]") {
    auto c = hsv_to_color(359, 1.0f, 1.0f);
    CHECK(c.red == 255);
    CHECK(c.green == 0);
    CHECK(c.blue <= 5);
}

TEST_CASE("s=0 gives achromatic grey", "[hsv]") {
    auto c = hsv_to_color(180, 0.0f, 0.5f);
    CHECK(color_near(c, lv_color_make(127, 127, 127)));
}

TEST_CASE("v=0 gives black regardless of hue", "[hsv]") {
    CHECK(lv_color_to_u16(hsv_to_color(120, 1.0f, 0.0f)) == 0x0000u);
}
