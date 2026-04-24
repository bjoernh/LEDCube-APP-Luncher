#include "sdl_internal.hpp"

#include <cstdio>
#include <cstdlib>

namespace cube::sdl {

namespace {

SDL_Window*   s_window   = nullptr;
SDL_Renderer* s_renderer = nullptr;
SDL_Texture*  s_texture  = nullptr;

// Single full-frame draw buffer (LVGL PARTIAL mode): 64 * 64 * 2 = 8192 B @ RGB565.
constexpr uint32_t kBufPixels = 64 * 64;
alignas(8) uint16_t s_draw_buf[kBufPixels];

void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const int x = area->x1;
    const int y = area->y1;
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;

    const SDL_Rect rect{ x, y, w, h };
    // Upload only the dirty sub-rect. Pitch = w * sizeof(uint16_t) because
    // LVGL's partial buffer is packed per-area, not per-screen.
    SDL_UpdateTexture(s_texture, &rect, px_map, w * 2);

    // Re-render the whole window — SDL handles the 2× upscale with nearest
    // neighbour thanks to SDL_HINT_RENDER_SCALE_QUALITY=0.
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, nullptr, nullptr);
    SDL_RenderPresent(s_renderer);

    lv_display_flush_ready(disp);
}

} // namespace

lv_display_t* display_init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        std::abort();
    }

    // Nearest-neighbour scaling — essential for the crisp pixel-art look.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    s_window = SDL_CreateWindow(kWindowTitle,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                kWindowW, kWindowH, SDL_WINDOW_SHOWN);
    if (!s_window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        std::abort();
    }

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);
    if (!s_renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        std::abort();
    }

    // Match LV_COLOR_DEPTH=16 exactly so no per-pixel conversion is needed.
    s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING, 64, 64);
    if (!s_texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        std::abort();
    }

    lv_display_t* disp = lv_display_create(64, 64);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, s_draw_buf, nullptr,
                           sizeof(s_draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}

void display_deinit() {
    if (s_texture)  { SDL_DestroyTexture(s_texture);   s_texture  = nullptr; }
    if (s_renderer) { SDL_DestroyRenderer(s_renderer); s_renderer = nullptr; }
    if (s_window)   { SDL_DestroyWindow(s_window);     s_window   = nullptr; }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

} // namespace cube::sdl
