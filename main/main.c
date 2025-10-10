#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "keys.h"
#include "zb_core.h"

static const char* TAG = "main";

/* Basic manufacturer information */
#define ESP_MANUFACTURER_NAME                                                                                                                        \
    "\x09"                                                                                                                                           \
    "ESPRESSIF"                                       /* Customized manufacturer name */
#define ESP_MODEL_IDENTIFIER "\x07" CONFIG_IDF_TARGET /* Customized model identifier */

static const gpio_num_t KEYS_GPIO[] = {7, 6, 21, 20, 22, 19};

static void app_keys_cb(uint8_t id, key_evt_t evt, void* ctx) {
    switch (evt) {
    case KEY_EVT_SINGLE:
        ESP_LOGI(TAG, "key %u: single", id);
        break;
    case KEY_EVT_DOUBLE:
        ESP_LOGI(TAG, "key %u: double", id);
        break;
    case KEY_EVT_LONG:
        ESP_LOGI(TAG, "key %u: long", id);
        break;
    }
}

void zbc_endpoint_attribute_handler(const esp_zb_zcl_set_attr_value_message_t* message) { ESP_LOGI(TAG, "zbc_endpoint_attribute_cb_t"); }
void zb_create_endpoint();

static void init_nvs(void);

void app_main(void) {
    init_nvs();

    keys_init(KEYS_GPIO, sizeof(KEYS_GPIO) / sizeof(KEYS_GPIO[0]), true, app_keys_cb, NULL);

    // zigbee
    zbc_init();
    zb_create_endpoint();
    zbc_start();

    zbc_wait_until_connected();

    ESP_LOGI(TAG, "ZIGBEE CONNECTED!");

    while (true) {
        // ESP_LOGI(TAG, "Hello world!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void zb_create_endpoint() {
    // endpoint 7 config
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = 7,
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

    // Optional: set BASIC attributes (manufacturer & model) if you have macros
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

    zbc_register_endpoint(&ep_cfg, cl, zbc_endpoint_attribute_handler);
}

/**
 * @brief Initialize non-volatile storage (NVS)
 */
static void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}