#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "keys.h"

static const char* TAG = "main";

static const int32_t KEYS_GPIO[] = {7, 6, 21, 20, 22, 19};

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

void app_main(void) {

    keys_init(KEYS_GPIO, sizeof(KEYS_GPIO) / sizeof(KEYS_GPIO[0]), true, app_keys_cb, NULL);

    while (true) {
        // ESP_LOGI(TAG, "Hello world!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}