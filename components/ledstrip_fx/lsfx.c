#include "lsfx.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "led_strip.h"

#include "lsfx_fx.h"

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

static const char* TAG = "LSFX";

// static _Thread_local struct lsfx* tls_self = NULL;
static __thread lsfx_t* tls_self = NULL;

// set pixel
void lsfx_set_pixel_trampoline(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (!tls_self)
        return;
    led_strip_set_pixel(tls_self->led_strip, index, red, green, blue);
}

static void lsfx_task(void* params) {
    lsfx_t* self = (lsfx_t*)params;
    tls_self = self;

    // LED Strip object handle
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&self->strip_config, &self->rmt_config, &self->led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");

    const lsfx_fx_t* fx = NULL;
    const lsfx_fx_t* fx_tmp = NULL;
    uint32_t time = 0;
    while (true) {
        if(xQueueReceive(self->queue, &fx_tmp, 0) == pdTRUE){
            fx = fx_tmp;

            ESP_LOGI(TAG, "Current FX: %s", fx->name);
        }
        
        fx->gen_frame(time, self->strip_config.max_leds, 255, NULL, lsfx_set_pixel_trampoline);
        led_strip_refresh(self->led_strip);
        
        vTaskDelay(pdMS_TO_TICKS(20));
        time += 20;
    }
}

// public

void lsfx_init(lsfx_t* self, led_strip_config_t strip_config, led_strip_rmt_config_t rmt_config) {
    self->strip_config = strip_config;
    self->rmt_config = rmt_config;

    self->queue = xQueueCreate(8, sizeof(lsfx_fx_t*));
    xTaskCreate(lsfx_task, "lsfx", 2048, self, 1, &self->task);
}

void lsfx_set_fx(lsfx_t* self, const lsfx_fx_t* fx) {
    if(xQueueSend(self->queue, &fx, 0) != pdTRUE){
        ESP_LOGE(TAG, "Unable to add FX to queue");
    }
}