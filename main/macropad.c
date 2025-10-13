#include "macropad.h"
#include "macropad_led.h"
#include "macropad_map.h"
#include "macropad_zigbee.h"

#include "esp_check.h"

static const char* TAG = "macropad";

/* --------- Keys callback --------- */

static void on_key(uint8_t id, key_evt_t evt, void* ctx) {
    const uint8_t ep = MACROPAD_MAP[id].zb_endpoint;

    switch (evt) {
    case KEY_EVT_PRESS_DOWN:
        ESP_LOGI(TAG, "key %u: pressed", id);
        macropad_led_on();
        break;
    case KEY_EVT_SINGLE:
        ESP_LOGI(TAG, "key %u: single", id);
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID);
        break;
    case KEY_EVT_DOUBLE:
        ESP_LOGI(TAG, "key %u: double", id);
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_ON_ID);
        break;
    case KEY_EVT_LONG:
        ESP_LOGI(TAG, "key %u: long", id);
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID);
        break;
    case KEY_EVT_PRESS_UP:
        ESP_LOGI(TAG, "key %u: released", id);
        macropad_led_off();
        break;
    }
}

/* --------- Public init --------- */

esp_err_t macropad_init() {
    // init LED
    macropad_led_init();

    // init zigbee
    macropad_zb_init(NULL);

    // Create endpoints
    for (uint8_t i = 0; i < MACROPAD_KEY_COUNT; ++i) {
        bool is_duplicated = false;
        for (uint8_t j = 0; j < i; ++j) {
            if (MACROPAD_MAP[j].zb_endpoint == MACROPAD_MAP[i].zb_endpoint) {
                is_duplicated = true;
                ESP_LOGW(TAG,
                         "Duplicate Zigbee endpoint %u detected for key %u (GPIO %u); "
                         "this endpoint was already registered earlier — skipping.",
                         MACROPAD_MAP[i].zb_endpoint, i, MACROPAD_MAP[i].key_gpio);
                break;
            }
        }
        if (!is_duplicated) {
            ESP_RETURN_ON_ERROR(macropad_zb_create_endpoint(MACROPAD_MAP[i].zb_endpoint), TAG, "create_endpoint failed");
        }
    }

    // Build Keys GPIO array
    static gpio_num_t keys_gpio[MACROPAD_KEY_COUNT];
    for (uint8_t i = 0; i < MACROPAD_KEY_COUNT; ++i) {
        keys_gpio[i] = MACROPAD_MAP[i].key_gpio;
    }

    ESP_RETURN_ON_ERROR(keys_init(keys_gpio, MACROPAD_KEY_COUNT, true, on_key, NULL), TAG, "keys_init failed");
    ESP_LOGI(TAG, "Macropad ready (%u keys)", MACROPAD_KEY_COUNT);

    // Start zigbee
    macropad_zb_start();

    while (!macropad_zb_is_connected()) {
        macropad_led_blink();
    }

    ESP_LOGI(TAG, "Zigbee connected!");
    return ESP_OK;
}