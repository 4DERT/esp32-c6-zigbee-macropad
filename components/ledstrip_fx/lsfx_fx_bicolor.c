#include "lsfx_fx_bicolor.h"
#include "lsfx_fx.h"

#include <stdint.h>

static void gen_frame(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel) {
    uint8_t r1 = 255;
    uint8_t g1 = 0;
    uint8_t b1 = 0;

    uint8_t r2 = 0;
    uint8_t g2 = 0;
    uint8_t b2 = 255;

    if (opt_params) {
        const lsfx_fx_bicolor_params_t* p = (const lsfx_fx_bicolor_params_t*)opt_params;
        r1 = p->r1;
        g1 = p->g1;
        b1 = p->b1;

        r2 = p->r2;
        g2 = p->g2;
        b2 = p->b2;
    }

    uint8_t fr1 = ((uint32_t)r1 * brightness) / 255;
    uint8_t fg1 = ((uint32_t)g1 * brightness) / 255;
    uint8_t fb1 = ((uint32_t)b1 * brightness) / 255;

    uint8_t fr2 = ((uint32_t)r2 * brightness) / 255;
    uint8_t fg2 = ((uint32_t)g2 * brightness) / 255;
    uint8_t fb2 = ((uint32_t)b2 * brightness) / 255;

    for (uint32_t i = 0; i < led_count/2; ++i) {
        set_pixel(i, fr1, fg1, fb1);
    }

    for (uint32_t i = led_count/2; i < led_count; ++i) {
        set_pixel(i, fr2, fg2, fb2);
    }
}

const lsfx_fx_t lsfx_fx_bicolor_t = {
    .name = "bicolor",
    .is_one_time = true,
    .gen_frame = gen_frame,
};