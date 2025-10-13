#include "macropad_led.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

void macropad_led_init() {
    gpio_set_direction(MACROPAD_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(MACROPAD_LED_GPIO, !MACROPAD_LED_ACTIVE);
}

inline void macropad_led_on() { gpio_set_level(MACROPAD_LED_GPIO, MACROPAD_LED_ACTIVE); }

inline void macropad_led_off() { gpio_set_level(MACROPAD_LED_GPIO, !MACROPAD_LED_ACTIVE); }

void macropad_led_blink() {
    macropad_led_on();
    vTaskDelay(pdMS_TO_TICKS(MACROPAD_LED_BLINK_MS));
    macropad_led_off();
    vTaskDelay(pdMS_TO_TICKS(MACROPAD_LED_BLINK_MS));
}