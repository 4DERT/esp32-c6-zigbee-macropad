#pragma once

#include "esp_zigbee_core.h"

/* Zigbee configuration */
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define INSTALLCODE_POLICY_ENABLE false                                  /* enable the install code policy for security */
#define ED_KEEP_ALIVE 3000                                               /* 3000 millisecond */
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK /* Zigbee primary channel mask */

#define ESP_ZB_ZED_CONFIG()                                                                                                                          \
    {                                                                                                                                                \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                                                                                                        \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,                                                                                            \
        .nwk_cfg.zed_cfg =                                                                                                                           \
            {                                                                                                                                        \
                .ed_timeout = ED_AGING_TIMEOUT,                                                                                                      \
                .keep_alive = ED_KEEP_ALIVE,                                                                                                         \
            },                                                                                                                                       \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                                                                                                                \
    {                                                                                                                                                \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                                                                                                          \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                                                                                                                 \
    {                                                                                                                                                \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,                                                                                        \
    }

typedef void (*zbc_endpoint_attribute_cb_t)(const esp_zb_zcl_set_attr_value_message_t* message);
typedef void (*zbc_network_leave_cb_t)(void);

esp_err_t zbc_register_endpoint(esp_zb_endpoint_config_t* endpoint_cfg, esp_zb_cluster_list_t* cluster_list,
                                zbc_endpoint_attribute_cb_t attribute_cb);
void zbc_init(zbc_network_leave_cb_t network_leave_cb);
void zbc_start();
bool zbc_is_connected();
void zbc_wait_until_connected();
void zbc_factory_reset();