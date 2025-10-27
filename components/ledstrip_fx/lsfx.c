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

typedef enum { LSFX_CMD_SET_FX, LSFX_CMD_SET_ENABLED, LSFX_CMD_SET_BRIGHTNESS } lsfx_cmd_type_t;

typedef struct {
    const lsfx_fx_t* fx;
    const void* params;
} fx_with_params_t;

typedef struct {
    lsfx_cmd_type_t type;
    union {
        fx_with_params_t fx;
        uint8_t brightness;
        bool enabled;
    } data;
} lsfx_cmd_t;

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

    lsfx_cmd_t cmd;
    fx_with_params_t active_fx;
    active_fx.fx = NULL;

    uint32_t time = 0;
    TickType_t wait_ticks = portMAX_DELAY;

    while (true) {

        // Decide how long to wait in this iteration
        if (!self->enabled) {
            // DISABLED: Sleep indefinitely, waiting for any command (e.g., to enable)
            wait_ticks = portMAX_DELAY;
        } else if (!active_fx.fx || active_fx.fx->is_one_time) {
            // ENABLED, but static effect or no effect: Sleep indefinitely
            wait_ticks = portMAX_DELAY;
        } else {
            // ENABLED and animated: Wait for the next frame's time
            wait_ticks = pdMS_TO_TICKS(LSFX_FRAME_TIME_MS);
        }

        // Wait for a command or a timeout
        if (xQueueReceive(self->queue, &cmd, wait_ticks) == pdTRUE) {

            bool needs_refresh = false;

            if (cmd.type == LSFX_CMD_SET_FX) {
                // New effect
                active_fx = cmd.data.fx;
                time = 0;
                ESP_LOGI(TAG, "Current FX: %s", active_fx.fx->name);
                needs_refresh = true;

            } else if (cmd.type == LSFX_CMD_SET_BRIGHTNESS) {
                self->brightness = cmd.data.brightness;
                ESP_LOGI(TAG, "Brightness set to: %u", self->brightness);
                needs_refresh = true;

            } else if (cmd.type == LSFX_CMD_SET_ENABLED) {
                self->enabled = cmd.data.enabled;
                ESP_LOGI(TAG, "Enabled set to: %d", self->enabled);
                needs_refresh = true;
            }

            // Refresh logic
            if (needs_refresh) {
                if (self->enabled && active_fx.fx) {
                    // Enabled and an effect is active -> render it
                    active_fx.fx->gen_frame(time, self->strip_config.max_leds, self->brightness, active_fx.params, lsfx_set_pixel_trampoline);
                    led_strip_refresh(self->led_strip);
                } else {
                    // Disabled or no effect
                    led_strip_clear(self->led_strip);
                    led_strip_refresh(self->led_strip);
                }
            }

        } else {
            // This block will ONLY be reached if:
            // 1. self->enabled == true
            // 2. active_fx.fx != NULL
            // 3. active_fx.fx->is_one_time == false

            time += LSFX_FRAME_TIME_MS;
            active_fx.fx->gen_frame(time, self->strip_config.max_leds, self->brightness, active_fx.params, lsfx_set_pixel_trampoline);
            led_strip_refresh(self->led_strip);
        }
    }
}

// public

void lsfx_init(lsfx_t* self, led_strip_config_t strip_config, led_strip_rmt_config_t rmt_config) {
    self->strip_config = strip_config;
    self->rmt_config = rmt_config;
    self->brightness = LSFX_BRIGHTNESS_MAX;
    self->enabled = true;

    self->queue = xQueueCreate(8, sizeof(lsfx_cmd_t));
    xTaskCreate(lsfx_task, "lsfx", 2048, self, 1, &self->task);
}

void lsfx_set_fx(lsfx_t* self, const lsfx_fx_t* fx, const void* fx_params) {
    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_FX, .data.fx = {.fx = fx, .params = fx_params}};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add FX to queue");
    }
}

void lsfx_set_brightness(lsfx_t* self, uint8_t brightness) {
    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_BRIGHTNESS, .data.brightness = brightness};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add brightness cmd to queue");
    }
}

void lsfx_set_enabled(lsfx_t* self, bool enabled) {
    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_ENABLED, .data.enabled = enabled};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add enabled cmd to queue");
    }
}