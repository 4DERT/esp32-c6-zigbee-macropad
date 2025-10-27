#include "lsfx_fx_rainbow.h"
#include "lsfx_fx.h"

#include <stdint.h>

static void hsv2rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b) {
    // clang-format off
    uint16_t H=h%360; uint8_t reg=H/60; uint16_t f=(H%60)*255/60;
    uint16_t p=v*(255-s)/255, q=v*(255-(f*s)/255)/255, t=v*(255-((255-f)*s)/255)/255;
    switch(reg){case 0:*r=v;*g=t;*b=p;break;case 1:*r=q;*g=v;*b=p;break;case 2:*r=p;*g=v;*b=t;break;
                case 3:*r=p;*g=q;*b=v;break;case 4:*r=t;*g=p;*b=v;break;default:*r=v;*g=p;*b=q;break;}
    // clang-format on
}

static void gen_frame(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel) {
    uint32_t period_ms = 10000; // 10s
    if (opt_params) {
        const lsfx_rainbow_params_t* p = (const lsfx_rainbow_params_t*)opt_params;
        if (p->period_ms)
            period_ms = p->period_ms;
    }

    uint32_t t_local = t_ms % period_ms;
    uint16_t hue = ((uint64_t)t_local * 360) / period_ms;
    uint8_t r, g, b;

    hsv2rgb(hue, 255, brightness, &r, &g, &b);

    for (int i = 0; i < led_count; i++) {
        set_pixel(i, r, g, b);
    }
}

const lsfx_fx_t lsfx_fx_rainbow_t = {
    .name = "rainbow",
    .gen_frame = gen_frame,
};