#pragma once

#include "lsfx_fx.h"

typedef struct {
    uint8_t r1, g1, b1; // color of the first half
    uint8_t r2, g2, b2; // color of the second half
} lsfx_fx_bicolor_params_t;

extern const lsfx_fx_t lsfx_fx_bicolor_t;