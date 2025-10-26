#pragma once

#include "led_strip.h"
#include "lsfx_fx.h"


struct lsfx {
    led_strip_handle_t led_strip;
    TaskHandle_t task;
    QueueHandle_t queue;
    led_strip_config_t strip_config;
    led_strip_rmt_config_t rmt_config;
};

typedef struct lsfx lsfx_t;

/*
    // LED strip general initialization
    led_strip_config_t strip_config = {
        .strip_gpio_num = self->gpio,             // The GPIO that connected to the LED strip's data line
        .max_leds = self->led_num,                // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = self->invert_out,     // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
    };

*/
void lsfx_init(lsfx_t* self, led_strip_config_t strip_config, led_strip_rmt_config_t rmt_config);

void lsfx_set_fx(lsfx_t* self, const lsfx_fx_t* fx);