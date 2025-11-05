#include "lsfx.h"

#include <stdatomic.h>

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

struct lsfx_t {
    led_strip_handle_t led_strip;
    TaskHandle_t task;
    QueueHandle_t queue;
    led_strip_config_t strip_config;
    led_strip_rmt_config_t rmt_config;
    _Atomic uint8_t brightness;
    _Atomic bool enabled;
    _Atomic lsfx_effect_binding_t active_fx;
};

typedef struct {
    lsfx_cmd_type_t type;
    union {
        lsfx_effect_binding_t fx;
        uint8_t brightness;
        bool enabled;
    } data;
} lsfx_cmd_t;

// static _Thread_local struct lsfx* tls_self = NULL;
static __thread lsfx_handle_t tls_self = NULL;

// set pixel
void lsfx_set_pixel_trampoline(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (!tls_self)
        return;
    led_strip_set_pixel(tls_self->led_strip, index, red, green, blue);
}

static void lsfx_task(void* params) {
    lsfx_handle_t self = (lsfx_handle_t)params;
    tls_self = self;

    lsfx_cmd_t cmd;
    uint32_t time = 0;
    TickType_t wait_ticks = portMAX_DELAY;

    while (true) {
        bool current_enabled = atomic_load(&self->enabled);
        lsfx_effect_binding_t current_fx = atomic_load(&self->active_fx);
        uint8_t current_brightness = atomic_load(&self->brightness);

        // Decide how long to wait in this iteration
        if (!current_enabled) {
            // DISABLED: Sleep indefinitely, waiting for any command (e.g., to enable)
            wait_ticks = portMAX_DELAY;
        } else if (!current_fx.fx || current_fx.fx->is_one_time) {
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
                atomic_store(&self->active_fx, cmd.data.fx);
                time = 0;
                ESP_LOGI(TAG, "Current FX: %s", cmd.data.fx.fx->name);
                needs_refresh = true;

            } else if (cmd.type == LSFX_CMD_SET_BRIGHTNESS) {
                atomic_store(&self->brightness, cmd.data.brightness);
                ESP_LOGI(TAG, "Brightness set to: %u", cmd.data.brightness);
                needs_refresh = true;

            } else if (cmd.type == LSFX_CMD_SET_ENABLED) {
                atomic_store(&self->enabled, cmd.data.enabled);
                ESP_LOGI(TAG, "Enabled set to: %d", cmd.data.enabled);
                needs_refresh = true;
            }

            // Refresh logic
            if (needs_refresh) {
                current_enabled = atomic_load(&self->enabled);
                current_fx = atomic_load(&self->active_fx);
                current_brightness = atomic_load(&self->brightness);

                if (current_enabled && current_fx.fx) {
                    // Enabled and an effect is active -> render it
                    current_fx.fx->gen_frame(time, self->strip_config.max_leds, current_brightness, current_fx.params, lsfx_set_pixel_trampoline);
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
            current_fx.fx->gen_frame(time, self->strip_config.max_leds, current_brightness, current_fx.params, lsfx_set_pixel_trampoline);
            led_strip_refresh(self->led_strip);
        }
    }
}

// public

lsfx_handle_t lsfx_init(led_strip_config_t strip_config, led_strip_rmt_config_t rmt_config) {
    lsfx_handle_t self = (lsfx_handle_t)calloc(1, sizeof(struct lsfx_t));
    if (self == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for lsfx_t");
        return NULL;
    }

    self->strip_config = strip_config;
    self->rmt_config = rmt_config;

    if((led_strip_new_rmt_device(&self->strip_config, &self->rmt_config, &self->led_strip)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create led_strip object");
        free(self);
        return NULL;
    }

    atomic_init(&self->brightness, LSFX_BRIGHTNESS_MAX);
    atomic_init(&self->enabled, true);
    lsfx_effect_binding_t initial_fx = {.fx = NULL, .params = NULL};
    atomic_init(&self->active_fx, initial_fx);

    self->queue = xQueueCreate(8, sizeof(lsfx_cmd_t));
    if (self->queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        led_strip_del(self->led_strip);
        free(self);
        return NULL;
    }

    BaseType_t task_created = xTaskCreate(lsfx_task, "lsfx", 2048, self, 1, &self->task);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create lsfx task");

        // clean
        vQueueDelete(self->queue);
        led_strip_del(self->led_strip);
        free(self);
        return NULL;
    }

    ESP_LOGI(TAG, "LSFX instance created successfully");
    return self;
}

void lsfx_deinit(lsfx_handle_t self) {
    if (self == NULL)
        return;

    vTaskDelete(self->task);
    vQueueDelete(self->queue);
    led_strip_del(self->led_strip);
    free(self);
}

void lsfx_set_fx(lsfx_handle_t self, const lsfx_fx_t* fx, const void* fx_params) {
    if (self == NULL)
        return;

    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_FX, .data.fx = {.fx = fx, .params = fx_params}};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add FX to queue");
    }
}

lsfx_effect_binding_t lsfx_get_fx(lsfx_handle_t self) {
    if (self == NULL) {
        return (lsfx_effect_binding_t){.fx = NULL, .params = NULL};
    }

    return atomic_load(&self->active_fx);
}

void lsfx_set_brightness(lsfx_handle_t self, uint8_t brightness) {
    if (self == NULL)
        return;

    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_BRIGHTNESS, .data.brightness = brightness};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add brightness cmd to queue");
    }
}

uint8_t lsfx_get_brightness(lsfx_handle_t self) {
    if (self == NULL)
        return 0;

    return atomic_load(&self->brightness);
}

void lsfx_set_enabled(lsfx_handle_t self, bool enabled) {
    if (self == NULL)
        return;

    lsfx_cmd_t cmd = {.type = LSFX_CMD_SET_ENABLED, .data.enabled = enabled};

    if (xQueueSend(self->queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Unable to add enabled cmd to queue");
    }
}

bool lsfx_get_enabled(lsfx_handle_t self) {
    if (self == NULL)
        return false;

    return atomic_load(&self->enabled);
}