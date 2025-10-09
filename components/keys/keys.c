#include "keys.h"
#include "button_gpio.h"
#include "esp_log.h"
#include "iot_button.h" // from espressif/button

static const char* TAG = "keys";

static button_handle_t s_btn[KEYS_MAX_NUM];
static keys_event_cb_t s_cb;
static void* s_ctx;
static uint8_t s_num;

static void dispatch(uint8_t id, key_evt_t evt) {
    if (s_cb)
        s_cb(id, evt, s_ctx);
}

static void on_single(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_SINGLE); }
static void on_double(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_DOUBLE); }
static void on_long(void* btn_handle, void* usr) { dispatch((uint8_t)(uintptr_t)usr, KEY_EVT_LONG); }

esp_err_t keys_init(const int32_t* gpio_list, uint8_t num_keys, bool is_active_on_low, keys_event_cb_t cb, void* user_ctx) {
    if (!gpio_list || num_keys == 0 || num_keys > KEYS_MAX_NUM)
        return ESP_ERR_INVALID_ARG;

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
        if (err != ESP_OK || !s_btn[i])
            return ESP_FAIL;

        iot_button_register_cb(s_btn[i], BUTTON_SINGLE_CLICK, NULL, on_single, (void*)(uintptr_t)i);
        iot_button_register_cb(s_btn[i], BUTTON_DOUBLE_CLICK, NULL, on_double, (void*)(uintptr_t)i);
        button_event_args_t a = {.long_press.press_time = 700};
        iot_button_register_cb(s_btn[i], BUTTON_LONG_PRESS_START, &a, on_long, (void*)(uintptr_t)i);
    }
    ESP_LOGI(TAG, "Initialized %u button(s)", s_num);
    return ESP_OK;
}