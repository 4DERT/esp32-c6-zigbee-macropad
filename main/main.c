#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "keys.h"
#include "macropad.h"
#include "zb_core.h"

static const char* TAG = "main";

static void init_nvs(void);

void app_main(void) {
    init_nvs();
    macropad_init();
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