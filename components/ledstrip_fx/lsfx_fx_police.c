#include "lsfx_fx_police.h"
#include "lsfx_fx.h"

#include <stdint.h>

static void gen_frame(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel) {
    uint32_t phase = (t_ms / 1000) % 2; // 0 → blue, 1 → red
    uint8_t r, g, b;

    if (phase == 0) {
        r = 0;
        g = 0;
        b = brightness;
    } else {
        r = brightness;
        g = 0;
        b = 0;
    }

    for (uint32_t i = 0; i < led_count; ++i) {
        set_pixel(i, r, g, b);
    }
}

const lsfx_fx_t lsfx_fx_police_t = {
    .name = "police",
    .gen_frame = gen_frame,
};