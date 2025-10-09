#include "keys.h"
#include "button_gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "iot_button.h" // from espressif/button
#include <inttypes.h>

static const char* TAG = "keys";

button_event_args_t a = {.long_press.press_time = KEYS_LONG_PRESS_MS};

static button_handle_t s_btn[KEYS_MAX_NUM];
static keys_event_cb_t s_cb;
static void* s_ctx;
static uint8_t s_num;
static bool s_inited;

/**
 * @brief Internal dispatcher from espressif/button callbacks to user callback.
 * @param id 0..s_num-1 – logical key index.
 * @param evt semantic event (single/double/long).
 */
static void dispatch(uint8_t id, key_evt_t evt) {
    if (s_cb)
        s_cb(id, evt, s_ctx);
}

// Button library callbacks: usr carries packed key id.
static void on_single(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_SINGLE); }
static void on_double(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_DOUBLE); }
static void on_long(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_LONG); }

static esp_err_t register_key_callbacks(uint8_t idx) {
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_btn[idx], BUTTON_SINGLE_CLICK, NULL, on_single, (void*)(uintptr_t)idx), TAG,
                        "register SINGLE failed (key=%u)", idx);
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_btn[idx], BUTTON_DOUBLE_CLICK, NULL, on_double, (void*)(uintptr_t)idx), TAG,
                        "register DOUBLE failed (key=%u)", idx);
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_btn[idx], BUTTON_LONG_PRESS_START, &a, on_long, (void*)(uintptr_t)idx), TAG,
                        "register LONG failed (key=%u)", idx);
    return ESP_OK;
}

esp_err_t keys_init(const gpio_num_t* gpio_list, uint8_t num_keys, bool is_active_on_low, keys_event_cb_t cb, void* user_ctx) {
    // Basic validation
    ESP_RETURN_ON_FALSE(!s_inited, ESP_ERR_INVALID_STATE, TAG, "already initialized");
    ESP_RETURN_ON_FALSE(gpio_list != NULL, ESP_ERR_INVALID_ARG, TAG, "gpio_list is NULL");
    ESP_RETURN_ON_FALSE(num_keys > 0, ESP_ERR_INVALID_ARG, TAG, "num_keys == 0");
    ESP_RETURN_ON_FALSE(num_keys <= KEYS_MAX_NUM, ESP_ERR_INVALID_ARG, TAG, "num_keys > KEYS_MAX_NUM");
    ESP_RETURN_ON_FALSE(cb != NULL, ESP_ERR_INVALID_ARG, TAG, "callback is NULL");

    s_cb = cb;
    s_ctx = user_ctx;
    s_num = num_keys;

    for (uint8_t i = 0; i < num_keys; ++i) {
        button_config_t cfg = {};
        button_gpio_config_t gpio_cfg = {
            .gpio_num = gpio_list[i],
            .active_level = is_active_on_low ? 0 : 1,
        };

        esp_err_t err = iot_button_new_gpio_device(&cfg, &gpio_cfg, &s_btn[i]);
        if (err != ESP_OK || !s_btn[i]) {
            ESP_LOGE(TAG, "create button failed (key=%u, gpio=%" PRId32 "): %s", i, (int32_t)gpio_list[i], esp_err_to_name(err));
            return err != ESP_OK ? err : ESP_FAIL;
        }

        ESP_RETURN_ON_ERROR(register_key_callbacks(i), TAG, "register callbacks failed (key=%u)", i);
    }

    s_inited = true;
    ESP_LOGI(TAG, "Initialized %u button(s)", s_num);
    return ESP_OK;
}

void keys_deinit(void) {
    for (uint8_t i = 0; i < s_num; ++i) {
        if (s_btn[i]) {
            iot_button_delete(s_btn[i]);
            s_btn[i] = NULL;
        }
    }
    s_num = 0;
    s_cb = NULL;
    s_ctx = NULL;
    s_inited = false;
}

uint8_t keys_count(void) { return s_num; }