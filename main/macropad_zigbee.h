#pragma once

#include "zb_core.h"
#include <stdint.h>

/* Basic manufacturer information */
#define ESP_MANUFACTURER_NAME                                                                                                                        \
    "\x09"                                                                                                                                           \
    "ESPRESSIF"                                       /* Customized manufacturer name */
#define ESP_MODEL_IDENTIFIER "\x07" CONFIG_IDF_TARGET /* Customized model identifier */

void macropad_zb_init();
void macropad_zb_start();
bool macropad_zb_is_connected();
void macropad_zb_send(uint8_t endpoint, esp_zb_zcl_on_off_cmd_id_t cmd_id);
esp_err_t macropad_zb_create_endpoint(uint8_t endpoint);