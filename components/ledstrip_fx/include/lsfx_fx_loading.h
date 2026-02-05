#pragma once

#include "lsfx_fx.h"

typedef struct {
    uint8_t red, green, blue;
    uint32_t period_ms;
} lsfx_fx_loading_params_t;

extern const lsfx_fx_t lsfx_fx_loading_t;