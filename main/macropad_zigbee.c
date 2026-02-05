#include "macropad_zigbee.h"

#include <string.h>

#include "macropad_led.h"
#include "macropad_map.h"
#include "zb_core.h"

static const char* TAG = "Macropad_Zigbee";

#define MANUF_LEN (sizeof(CONFIG_MACROPAD_MANUFACTURER_NAME) - 1)
#define MODEL_LEN (sizeof(CONFIG_MACROPAD_MODEL_IDENTIFIER) - 1)

_Static_assert(MANUF_LEN <= 255, "Manufacturer name too long for ZCL char string");
_Static_assert(MODEL_LEN <= 255, "Model identifier too long for ZCL char string");

static uint8_t s_manufacturer[1 + MANUF_LEN];
static uint8_t s_model[1 + MODEL_LEN];

static inline void zcl_make_char(uint8_t* dst, const char* src, size_t len) {
    dst[0] = (uint8_t)len;
    if (len)
        memcpy(&dst[1], src, len);
}

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

void on_zbc_endpoint_attribute_set(const esp_zb_zcl_set_attr_value_message_t* message) {
    uint8_t endpoint = message->info.dst_endpoint;
    uint16_t cluster = message->info.cluster;
    uint16_t attribute = message->attribute.id;

    if (endpoint == MACROPAD_LIGHT_EP && cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF && attribute == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) {
        bool light_state = *(bool*)message->attribute.data.value;
        macropad_led_set_enabled(light_state);
        ESP_LOGI(TAG, "Light set to: %s", light_state ? "ON" : "OFF");
    }

    else if (cluster == ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY && attribute == ESP_ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID) {
        uint16_t time = *(uint16_t*)message->attribute.data.value;
        if (time > 0) {
            macropad_led_start_loading();
            ESP_LOGI(TAG, "Identify... (time=%u)", time);
        } else {
            macropad_led_stop_loading();
            ESP_LOGI(TAG, "Identify stopped");
        }
    }

    ESP_LOGI(TAG, "Attribute set: ep: %u, cl: %u", endpoint, cluster);
}

// Public

void macropad_zb_init() {
    zcl_make_char(s_manufacturer, CONFIG_MACROPAD_MANUFACTURER_NAME, MANUF_LEN);
    zcl_make_char(s_model, CONFIG_MACROPAD_MODEL_IDENTIFIER, MODEL_LEN);

    zbc_init(NULL);
}

void macropad_zb_start() { zbc_start(); }

void macropad_zb_factory_reset() {
    zbc_factory_reset();
}

bool macropad_zb_is_connected() { return zbc_is_connected(); }

void macropad_zb_send(uint8_t endpoint, esp_zb_zcl_on_off_cmd_id_t cmd_id) {
    if (!macropad_zb_is_connected()) {
        ESP_LOGW(TAG, "Device is currently not connected to zigbee network, action will be not send.");
        return;
    }

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

esp_err_t macropad_zb_create_endpoint(uint8_t endpoint) {
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

    // Server clusters (for device info / commissioning)
    esp_zb_cluster_list_add_basic_cluster(cl, esp_zb_basic_cluster_create(&basic_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cl, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Set BASIC attributes (manufacturer & model)
    esp_zb_attribute_list_t* basic_srv = esp_zb_cluster_list_get_cluster(cl, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (basic_srv) {
        esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void*)s_manufacturer);
        esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void*)s_model);
    }

    esp_zb_cluster_list_add_on_off_cluster(cl, esp_zb_on_off_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);

    return zbc_register_endpoint(&ep_cfg, cl, on_zbc_endpoint_attribute_set);
}

esp_err_t macropad_zb_create_light_endpoint(uint8_t endpoint) {
    // Endpoint configuration for an On/Off Light
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = endpoint,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_LIGHT_DEVICE_ID, // Device ID for Light (0x0100)
        .app_device_version = 0,
    };

    // Configuration for Basic and Identify clusters
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    };

    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
    };

    // Configuration for On/Off cluster (default state)
    esp_zb_on_off_cluster_cfg_t on_off_cfg = {
        .on_off = 0, // Default off
    };

    esp_zb_cluster_list_t* cl = esp_zb_zcl_cluster_list_create();

    // Server clusters (Device acts as the executor)

    // Basic and Identify are mandatory for all devices
    esp_zb_cluster_list_add_basic_cluster(cl, esp_zb_basic_cluster_create(&basic_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cl, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Crucial for Light: On/Off cluster must be SERVER role to receive commands
    esp_zb_cluster_list_add_on_off_cluster(cl, esp_zb_on_off_cluster_create(&on_off_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Set BASIC attributes (Manufacturer & Model)
    esp_zb_attribute_list_t* basic_srv = esp_zb_cluster_list_get_cluster(cl, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (basic_srv) {
        esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void*)s_manufacturer);
        esp_zb_basic_cluster_add_attr(basic_srv, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void*)s_model);
    }

    // Register the endpoint with the cluster list and callback
    return zbc_register_endpoint(&ep_cfg, cl, on_zbc_endpoint_attribute_set);
}

void macropad_zb_update_light_status(bool is_enabled) {
    esp_zb_lock_acquire(portMAX_DELAY);

    esp_zb_zcl_status_t status = esp_zb_zcl_set_attribute_val(MACROPAD_LIGHT_EP, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                              ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &is_enabled, false);

    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Failed to set OnOff attribute: 0x%x", status);
        esp_zb_lock_release();
        return;
    }

    // Send report
    esp_zb_zcl_report_attr_cmd_t report = {
        .zcl_basic_cmd = {
            .src_endpoint = MACROPAD_LIGHT_EP,
            .dst_endpoint = 1,                     // ZHA coordinator endpoint
            .dst_addr_u.addr_short = 0x0000,       // coordinator
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
        .attributeID = ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .dis_default_resp = 1,
    };

    esp_zb_zcl_report_attr_cmd_req(&report);

    esp_zb_lock_release();
}