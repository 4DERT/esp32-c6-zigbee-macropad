#include "macropad.h"
#include "macropad_led.h"
#include "macropad_map.h"
#include "macropad_zigbee.h"

#include "esp_check.h"

static const char* TAG = "macropad";

/* --------- Keys callback --------- */

static bool s_is_fn = false;

static const char* evt_name(key_evt_t e) {
    // clang-format off
    switch (e) {
        case KEY_EVT_PRESS_DOWN: return "press_down";
        case KEY_EVT_SINGLE:     return "single";
        case KEY_EVT_DOUBLE:     return "double";
        case KEY_EVT_LONG:       return "long";
        case KEY_EVT_PRESS_UP:   return "press_up";
        default:                 return "unknown";
    }
    // clang-format on
}

static void handle_fn_layer(uint8_t id, key_evt_t evt, void* ctx) {
    if (evt == KEY_EVT_PRESS_DOWN || evt == KEY_EVT_PRESS_UP)
        ESP_LOGI(TAG, "fn+key %u: %s", id, evt_name(evt));

    if(id == MACROPAD_KEY_FN_FACTORY_RESET && evt == KEY_EVT_LONG) {
        macropad_zb_factory_reset();
    }

    if (evt != KEY_EVT_PRESS_DOWN)
        return;

    switch (id) {
    case MACROPAD_KEY_FN_ON_OFF_ID:
        macropad_led_toggle_enabled();
        macropad_zb_update_light_status(macropad_led_get_enabled());
        break;
    case MACROPAD_KEY_FN_CYCLE_BRIGHTNESS_ID:
        macropad_led_cycle_brightness();
        break;
    case MACROPAD_KEY_FN_CYCLE_FX_ID:
        macropad_led_cycle_effects();
        break;
    case MACROPAD_KEY_FN_CYCLE_VARIANT_ID:
        macropad_led_cycle_effect_variants();
        break;
    default:
        break;
    }

    return;
}

static void handle_nornal_layer(uint8_t id, key_evt_t evt, void* ctx) {
#if MACROPAD_FN_ON_LONG_PRESS == 0
    if(MACROPAD_KEY_FN_ID == id)
        return;
#endif

    if (evt != KEY_EVT_PRESS_DOWN && evt != KEY_EVT_PRESS_UP)
        ESP_LOGI(TAG, "key %u: %s", id, evt_name(evt));

    const uint8_t ep = MACROPAD_MAP[id].zb_endpoint;

    switch (evt) {
    case KEY_EVT_SINGLE:
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID);
        break;
    case KEY_EVT_DOUBLE:
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_ON_ID);
        break;
    case KEY_EVT_LONG:
        macropad_zb_send(ep, ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID);
        break;
    default:
        break;
    }
}

static bool handle_fn_key(uint8_t id, key_evt_t evt) {
    if (id != MACROPAD_KEY_FN_ID)
        return false;

    // Enter FN on long-press; leave on release.
    switch (evt) {
#if MACROPAD_FN_ON_LONG_PRESS == 1
    case KEY_EVT_LONG:
#else
    case KEY_EVT_PRESS_DOWN:
#endif
        if (!s_is_fn) {
            s_is_fn = true;
            ESP_LOGI(TAG, "FN mode enabled");
        }
        return true;
    case KEY_EVT_PRESS_UP:
        if (s_is_fn) {
            s_is_fn = false;
            ESP_LOGI(TAG, "FN mode disabled");
        }
        return true;
    default:
        break;
    }
    return false;
}

static void on_key(uint8_t id, key_evt_t evt, void* ctx) {
    if (id >= MACROPAD_KEY_COUNT) {
        ESP_LOGW(TAG, "key %u out of range, evt=%s", id, evt_name(evt));
        return;
    }

    // Check whether to enable FN mode
    if (handle_fn_key(id, evt))
        return;

    // handle proper layer
    if (s_is_fn) {
        handle_fn_layer(id, evt, ctx);
    } else {
        handle_nornal_layer(id, evt, ctx);
    }
}

/* --------- Public init --------- */

esp_err_t macropad_init() {
    // init LED
    macropad_led_init();
    macropad_led_start_loading();

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

    // Add Light endpoint
    ESP_RETURN_ON_ERROR(macropad_zb_create_light_endpoint(MACROPAD_LIGHT_EP), TAG, "Light endpoint failed");

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
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    macropad_led_stop_loading();

    macropad_zb_update_light_status(macropad_led_get_enabled());
    ESP_LOGI(TAG, "Zigbee connected!");
    return ESP_OK;
}