#pragma once

#include "driver/gpio.h"
#include <stdint.h>

typedef struct {
    gpio_num_t key_gpio;
    uint8_t zb_endpoint;
} macropad_map_t;

static const macropad_map_t MACROPAD_MAP[] = {
    {GPIO_NUM_7, 11}, {GPIO_NUM_6, 21}, {GPIO_NUM_21, 31}, {GPIO_NUM_20, 41}, {GPIO_NUM_22, 51}, {GPIO_NUM_19, 61},
};

#define ARRAY_LEN(x) ((uint32_t)(sizeof(x) / sizeof((x)[0])))
#define MACROPAD_KEY_COUNT ((uint8_t)ARRAY_LEN(MACROPAD_MAP))