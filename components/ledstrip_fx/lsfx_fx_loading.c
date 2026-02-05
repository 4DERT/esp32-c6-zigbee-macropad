#include "lsfx_fx_loading.h"
#include "lsfx_fx.h"

#include <stdint.h>

static void gen_frame(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel) {
    uint32_t period = 300;
    uint8_t r = 255, g = 255, b = 255;

    if (opt_params) {
        const lsfx_fx_loading_params_t* p = (const lsfx_fx_loading_params_t*)opt_params;
        period = p->period_ms;
        r = p->red;
        g = p->green;
        b = p->blue;
    }

    if (led_count == 0 || period == 0) {
        return;
    }
    
    uint32_t step = (t_ms * led_count) / period;
    uint32_t pos  = step % led_count;

    for (uint32_t i = 0; i < led_count; ++i) {
        set_pixel(i, 0, 0, 0);
    }

    uint8_t rr = (r * brightness) / 255;
    uint8_t gg = (g * brightness) / 255;
    uint8_t bb = (b * brightness) / 255;

    set_pixel(pos, rr, gg, bb);
}

const lsfx_fx_t lsfx_fx_loading_t = {
    .name = "loading",
    .is_one_time = false,
    .gen_frame = gen_frame,
};