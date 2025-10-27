#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void (*set_pixel_f)(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);

typedef void (*lsfx_gen_frame_fn)(uint32_t t_ms, uint32_t led_count, uint8_t brightness, const void* opt_params, set_pixel_f set_pixel);

typedef struct {
    const char* name;
    bool is_one_time;
    lsfx_gen_frame_fn gen_frame;
} lsfx_fx_t;
