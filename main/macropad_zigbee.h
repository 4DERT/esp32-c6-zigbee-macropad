#pragma once

#include "zb_core.h"
#include <stdint.h>

#define MACROPAD_MANUFACTURER_NAME CONFIG_MACROPAD_MANUFACTURER_NAME
#define MACROPAD_MODEL_IDENTIFIER CONFIG_MACROPAD_MODEL_IDENTIFIER

void macropad_zb_init();
void macropad_zb_start();
bool macropad_zb_is_connected();
void macropad_zb_send(uint8_t endpoint, esp_zb_zcl_on_off_cmd_id_t cmd_id);
esp_err_t macropad_zb_create_endpoint(uint8_t endpoint);
esp_err_t macropad_zb_create_light_endpoint(uint8_t endpoint);
void macropad_zb_update_light_status(bool is_enabled);