#include "zb_core.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

static const char* TAG = "zbc";

#define CONNECTED_BIT BIT0
#define ZBC_MAX_EP 240

static EventGroupHandle_t connected_event_group;

static esp_zb_ep_list_t* s_ep_list;
static zbc_endpoint_attribute_cb_t s_ep_cb[ZBC_MAX_EP];
static zbc_network_leave_cb_t s_network_leave_cb;

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee bdb commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
    uint32_t* p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Device started up in%s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : " non");
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
                xEventGroupSetBits(connected_event_group, CONNECTED_BIT);
            }
        } else {
            ESP_LOGW(TAG, "%s failed with status: %s, retrying", esp_zb_zdo_signal_to_string(sig_type), esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG,
                     "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short "
                     "Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4], extended_pan_id[3], extended_pan_id[2],
                     extended_pan_id[1], extended_pan_id[0], esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
            xEventGroupSetBits(connected_event_group, CONNECTED_BIT);
        } else {
            ESP_LOGI(TAG, "Network steering was not successful (status: %d)", err_status);
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_LEAVE: // End Device + Router
        ESP_LOGI(TAG, "Factory resetting Zigbee stack, device will reboot!");
        if (s_network_leave_cb)
            s_network_leave_cb();
        esp_zb_factory_reset();
        break;
#if USE_DEEP_SLEEP
    case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
        ESP_LOGV(TAG, "Zigbee can sleep");
        break;
#endif
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
        break;
    }
}

esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t* message) {
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);

    // searching for callback
    uint8_t ep = message->info.dst_endpoint;
    if (ep > 0 && ep <= ZBC_MAX_EP) {
        zbc_endpoint_attribute_cb_t cb = s_ep_cb[ep];
        if (cb) {
            cb(message);
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "No user cb for ep=%u (cluster=0x%04X attr=0x%04X)", ep, message->info.cluster, message->attribute.id);
    return ESP_OK;
}

esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void* message) {
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((const esp_zb_zcl_set_attr_value_message_t*)message);
        break;

    default:
        ESP_LOGW(TAG, "(zb_action_handler) -> Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

esp_err_t create_range_extender_endpoint(uint8_t endpoint) {
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = endpoint,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_RANGE_EXTENDER_DEVICE_ID,
        .app_device_version = 0,
    };

    // clusters
    esp_zb_cluster_list_t* cl = esp_zb_zcl_cluster_list_create();

    // minimal metadata for the range extender endpoint
    esp_zb_cluster_list_add_basic_cluster(cl, esp_zb_basic_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // allow identifying the range extender endpoint
    esp_zb_cluster_list_add_identify_cluster(cl, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // can send Identify commands if needed
    esp_zb_cluster_list_add_identify_cluster(cl, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);

    // register endpoint
    return zbc_register_endpoint(&ep_cfg, cl, NULL);
}

static void esp_zb_task(void* pvParameters) {
/* initialize Zigbee stack with Zigbee end-device config */
#if CONFIG_ZB_ZED
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
#elif CONFIG_ZB_ZCZR
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ROUTER_CONFIG();
#else
#error "Select Zigbee device role in menuconfig (ZED or ZC/ZR)."
#endif

#if USE_DEEP_SLEEP && CONFIG_ZB_ZED
    esp_zb_sleep_enable(true);
#endif

    esp_zb_init(&zb_nwk_cfg);

// Create range extender endpoint if necessary
#if CONFIG_ZB_ZCZR
    create_range_extender_endpoint(RANGE_EXTENDER_ENDPOINT);
#endif

    // register all endpoints
    esp_zb_device_register(s_ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

// public
esp_err_t zbc_register_endpoint(esp_zb_endpoint_config_t* endpoint_cfg, esp_zb_cluster_list_t* cluster_list,
                                zbc_endpoint_attribute_cb_t attribute_cb) {
    uint8_t ep = endpoint_cfg->endpoint;

    ESP_RETURN_ON_FALSE(endpoint_cfg && cluster_list, ESP_ERR_INVALID_ARG, TAG, "NULL args");
    ESP_RETURN_ON_FALSE(ep > 0 && ep <= ZBC_MAX_EP, ESP_ERR_INVALID_ARG, TAG, "Invalid endpoint %u", ep);
    ESP_RETURN_ON_FALSE(s_ep_cb[ep] == NULL, ESP_ERR_INVALID_STATE, TAG, "Endpoint %u already registered", ep);

    // Add enpoint to list
    ESP_RETURN_ON_ERROR(esp_zb_ep_list_add_ep(s_ep_list, cluster_list, *endpoint_cfg), TAG, "esp_zb_ep_list_add_ep failed");

    // save attribute_cb with coresponding endpoint number
    s_ep_cb[ep] = attribute_cb;

    return ESP_OK;
}

void zbc_init(zbc_network_leave_cb_t network_leave_cb) {
    connected_event_group = xEventGroupCreate();

    s_ep_list = esp_zb_ep_list_create();
    s_network_leave_cb = network_leave_cb;

    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    /* load Zigbee platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
}

void zbc_start() { xTaskCreate(esp_zb_task, "Zigbee_main", 8192, NULL, 5, NULL); }

bool zbc_is_connected() {
    EventBits_t bits = xEventGroupGetBits(connected_event_group);
    return (bits & CONNECTED_BIT) != 0;
}

void zbc_wait_until_connected() { xEventGroupWaitBits(connected_event_group, CONNECTED_BIT, false, true, portMAX_DELAY); }

void zbc_factory_reset() {
    ESP_LOGI(TAG, "Factory resetting Zigbee stack, device will reboot!");
    if (s_network_leave_cb)
        s_network_leave_cb();
    esp_zb_factory_reset();
}