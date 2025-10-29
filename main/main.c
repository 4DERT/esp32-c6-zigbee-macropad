#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "keys.h"
#include "macropad.h"
#include "zb_core.h"
// #include "macropad_led_strip.h"
#include "lsfx.h"
#include "lsfx_fx_police.h"
#include "lsfx_fx_rainbow.h"
#include "lsfx_fx_static.h"

static void init_nvs(void);

// lsfx_t mlsfx;
lsfx_handle_t mlsfx;
lsfx_fx_rainbow_params_t rainbow_params = {.period_ms = 1000};
lsfx_fx_static_params_t static_params = {.red = 0, .green = 127, .blue = 255};

void app_main(void) {
    init_nvs();
    macropad_init();
    // mls_init();

    // LED strip general initialization
    led_strip_config_t strip_config = {
        .strip_gpio_num = 17,                     // The GPIO that connected to the LED strip's data line
        .max_leds = 10,                           // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,      // different clock source can lead to different power consumption
        .resolution_hz = (10 * 1000 * 1000), // RMT counter clock frequency
        .flags.with_dma = false,             // DMA feature is available on ESP target like ESP32-S3
    };

    mlsfx = lsfx_init(strip_config, rmt_config);
    lsfx_set_fx(mlsfx, &lsfx_fx_rainbow_t, &rainbow_params);
    vTaskDelay(pdMS_TO_TICKS(4000));
    lsfx_set_fx(mlsfx, &lsfx_fx_police_t, NULL);
    vTaskDelay(pdMS_TO_TICKS(4000));
    rainbow_params.period_ms = 120000;
    lsfx_set_fx(mlsfx, &lsfx_fx_rainbow_t, &rainbow_params);
    vTaskDelay(pdMS_TO_TICKS(10000));
    lsfx_set_brightness(mlsfx, 63);
    uint8_t brightness = lsfx_get_brightness(mlsfx);
    bool enabled = lsfx_get_enabled(mlsfx);
    lsfx_effect_binding_t fx_bind = lsfx_get_fx(mlsfx);
    ESP_LOGI("MAIN", "brightness: %u, enalbed: %d, fx_name: %s\n", brightness, enabled, fx_bind.fx->name);
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