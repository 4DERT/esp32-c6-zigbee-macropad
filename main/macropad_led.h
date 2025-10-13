#pragma once

#define MACROPAD_LED_GPIO GPIO_NUM_15
#define MACROPAD_LED_ACTIVE 1
#define MACROPAD_LED_BLINK_MS 100

void macropad_led_init();
void macropad_led_on();
void macropad_led_off();
void macropad_led_blink();
