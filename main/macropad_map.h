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
#define MACROPAD_KEY_FN_ID 5
#define MACROPAD_KEY_FN_ON_OFF_ID 0
#define MACROPAD_KEY_FN_CYCLE_BRIGHTNESS_ID 1
#define MACROPAD_KEY_FN_FACTORY_RESET 2
#define MACROPAD_KEY_FN_CYCLE_FX_ID 3
#define MACROPAD_KEY_FN_CYCLE_VARIANT_ID 4
#define MACROPAD_LIGHT_EP CONFIG_MACROPAD_LIGHT_EP

// 0: CLASSIC MODE - FN works like 'Shift'. Press down to activate layer immediately. FN key sends NO Zigbee commands.
// 1: DUAL MODE    - Hold FN to activate layer. Short press or double sends Zigbee command (Toggle).
#define MACROPAD_FN_ON_LONG_PRESS CONFIG_MACROPAD_FN_MODE