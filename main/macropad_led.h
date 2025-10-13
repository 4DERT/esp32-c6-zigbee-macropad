#pragma once

#define MACROPAD_LED_GPIO CONFIG_MACROPAD_LED_GPIO
#define MACROPAD_LED_ACTIVE CONFIG_MACROPAD_LED_ACTIVE_STATE
#define MACROPAD_LED_BLINK_MS CONFIG_MACROPAD_CONNECT_BLINK_MS

void macropad_led_init();
void macropad_led_on();
void macropad_led_off();
void macropad_led_blink();
