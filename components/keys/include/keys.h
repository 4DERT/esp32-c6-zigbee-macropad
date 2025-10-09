#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define KEYS_MAX_NUM 32

typedef enum {
    KEY_EVT_SINGLE,
    KEY_EVT_DOUBLE,
    KEY_EVT_LONG,
} key_evt_t;

typedef void (*keys_event_cb_t)(uint8_t key_id, key_evt_t evy, void* user_ctx);

esp_err_t keys_init(const int32_t* gpio_list, uint8_t num_keys, bool is_active_on_low, keys_event_cb_t cb, void* user_ctx);