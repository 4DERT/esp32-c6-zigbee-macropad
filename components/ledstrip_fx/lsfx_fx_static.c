#include "lsfx_fx_static.h"
#include "lsfx_fx.h"

#include <stdint.h>

static void gen_frame(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel) {

    uint8_t base_red = 255;
    uint8_t base_green = 255;
    uint8_t base_blue = 255;

    if (opt_params) {
        const lsfx_fx_static_params_t* p = (const lsfx_fx_static_params_t*)opt_params;
        base_red = p->red;
        base_green = p->green;
        base_blue = p->blue;
    }

    uint8_t final_red = ((uint32_t)base_red * brightness) / 255;
    uint8_t final_green = ((uint32_t)base_green * brightness) / 255;
    uint8_t final_blue = ((uint32_t)base_blue * brightness) / 255;

    for (uint32_t i = 0; i < led_count; ++i) {
        set_pixel(i, final_red, final_green, final_blue);
    }
}

const lsfx_fx_t lsfx_fx_static_t = {
    .name = "static",
    .is_one_time = true,
    .gen_frame = gen_frame,
};