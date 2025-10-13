#pragma once

#include "driver/gpio.h"
#include <stdint.h>

typedef struct {
    gpio_num_t key_gpio;
    uint8_t zb_endpoint;
} macropad_map_t;

/* Values come from Kconfig */
static const macropad_map_t MACROPAD_MAP[] = {
#if CONFIG_MACROPAD_KEYS_NUM >= 1
    {(gpio_num_t)CONFIG_MACROPAD_KEY0_GPIO, CONFIG_MACROPAD_KEY0_EP},
#endif
#if CONFIG_MACROPAD_KEYS_NUM >= 2
    {(gpio_num_t)CONFIG_MACROPAD_KEY1_GPIO, CONFIG_MACROPAD_KEY1_EP},
#endif
#if CONFIG_MACROPAD_KEYS_NUM >= 3
    {(gpio_num_t)CONFIG_MACROPAD_KEY2_GPIO, CONFIG_MACROPAD_KEY2_EP},
#endif
#if CONFIG_MACROPAD_KEYS_NUM >= 4
    {(gpio_num_t)CONFIG_MACROPAD_KEY3_GPIO, CONFIG_MACROPAD_KEY3_EP},
#endif
#if CONFIG_MACROPAD_KEYS_NUM >= 5
    {(gpio_num_t)CONFIG_MACROPAD_KEY4_GPIO, CONFIG_MACROPAD_KEY4_EP},
#endif
#if CONFIG_MACROPAD_KEYS_NUM >= 6
    {(gpio_num_t)CONFIG_MACROPAD_KEY5_GPIO, CONFIG_MACROPAD_KEY5_EP},
#endif
};

#define ARRAY_LEN(x) ((uint32_t)(sizeof(x) / sizeof((x)[0])))
#define MACROPAD_KEY_COUNT ((uint8_t)ARRAY_LEN(MACROPAD_MAP))