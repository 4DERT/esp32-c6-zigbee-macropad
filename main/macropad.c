#include "macropad.h"
#include "macropad_map.h"

#include "esp_check.h"

#include "zb_core.h"

static const char* TAG = "macropad";

/* Basic manufacturer information */
#define ESP_MANUFACTURER_NAME                                                                                                                        \
    "\x09"                                                                                                                                           \
    "ESPRESSIF"                                       /* Customized manufacturer name */
#define ESP_MODEL_IDENTIFIER "\x07" CONFIG_IDF_TARGET /* Customized model identifier */

void zbc_endpoint_attribute_handler(const esp_zb_zcl_set_attr_value_message_t* message) { ESP_LOGI(TAG, "zbc_endpoint_attribute_cb_t"); }

static const char* cmd_str(esp_zb_zcl_on_off_cmd_id_t cmd) {
    switch (cmd) {
    case ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
        return "TOGGLE";
        break;

    case ESP_ZB_ZCL_CMD_ON_OFF_ON_ID:
        return "ON";
        break;

    case ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID:
        return "OFF";
        break;

    default:
        return "UNKNOWN";
        break;
    }
}

void send_zb_command(uint8_t endpoint, esp_zb_zcl_on_off_cmd_id_t cmd_id) {
    esp_zb_zcl_on_off_cmd_t cmd = {
        .zcl_basic_cmd =
            {
                .dst_addr_u.addr_short = 0x0000,
                .dst_endpoint = 1,
                .src_endpoint = endpoint,
            },
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .on_off_cmd_id = cmd_id,
    };

    uint8_t tsn = esp_zb_zcl_on_off_cmd_req(&cmd);

    ESP_LOGI(TAG, "Sent %s (src_ep=%u -> 0x%04x/%u), TSN=%u", cmd_str(cmd_id), cmd.zcl_basic_cmd.src_endpoint,
             cmd.zcl_basic_cmd.dst_addr_u.addr_short, cmd.zcl_basic_cmd.dst_endpoint, tsn);
}

esp_err_t create_zb_endpoint(uint8_t endpoint) {
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = endpoint,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID,
        .app_device_version = 0,
    };
    // clusters in endpoint
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    };
    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
    };

    esp_zb_cluster_list_t* cl = esp_zb_zcl_cluster_list_create();

    // --- Server clusters (for device info / commissioning) ---
    esp_zb_cluster_list_add_basic_cluster(cl, esp_zb_basic_cluster_create(&basic_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cl, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Set BASIC attributes (manufacturer & model) if you have macros
    esp_zb_attribute_list_t* basic_srv = esp_zb_cluster_list_get_cluster(cl, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (basic_srv) {
#ifdef ESP_MANUFACTURER_NAME
        (void)esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, ESP_MANUFACTURER_NAME);
#endif
#ifdef ESP_MODEL_IDENTIFIER
        (void)esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, ESP_MODEL_IDENTIFIER);
#endif
    }

    esp_zb_cluster_list_add_on_off_cluster(cl, esp_zb_on_off_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);

    return zbc_register_endpoint(&ep_cfg, cl, zbc_endpoint_attribute_handler);
}

/* --------- Keys callback --------- */

static void on_key(uint8_t id, key_evt_t evt, void* ctx) {
    const uint8_t ep = MACROPAD_MAP[id].zb_endpoint;

    switch (evt) {
    case KEY_EVT_PRESS_DOWN:
        ESP_LOGI(TAG, "key %u: pressed", id);
        gpio_set_level(GPIO_NUM_15, 1);
        break;
    case KEY_EVT_SINGLE:
        ESP_LOGI(TAG, "key %u: single", id);
        send_zb_command(ep, ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID);
        break;
    case KEY_EVT_DOUBLE:
        ESP_LOGI(TAG, "key %u: double", id);
        send_zb_command(ep, ESP_ZB_ZCL_CMD_ON_OFF_ON_ID);
        break;
    case KEY_EVT_LONG:
        ESP_LOGI(TAG, "key %u: long", id);
        send_zb_command(ep, ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID);
        break;
    case KEY_EVT_PRESS_UP:
        ESP_LOGI(TAG, "key %u: released", id);
        gpio_set_level(GPIO_NUM_15, 0);
        break;
    }
}

/* --------- Public init --------- */

esp_err_t macropad_init() {
    // init LED
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 0);

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
            ESP_RETURN_ON_ERROR(create_zb_endpoint(MACROPAD_MAP[i].zb_endpoint), TAG, "create_endpoint failed");
        }
    }

    // Build Keys GPIO array
    static gpio_num_t keys_gpio[MACROPAD_KEY_COUNT];
    for (uint8_t i = 0; i < MACROPAD_KEY_COUNT; ++i) {
        keys_gpio[i] = MACROPAD_MAP[i].key_gpio;
    }

    ESP_RETURN_ON_ERROR(keys_init(keys_gpio, MACROPAD_KEY_COUNT, true, on_key, NULL), TAG, "keys_init failed");
    ESP_LOGI(TAG, "Macropad ready (%u keys)", MACROPAD_KEY_COUNT);
    return ESP_OK;
}