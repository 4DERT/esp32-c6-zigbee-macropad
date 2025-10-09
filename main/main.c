#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "main";

void app_main(void) {

    while (true) {
        ESP_LOGI(TAG, "Hello world!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}