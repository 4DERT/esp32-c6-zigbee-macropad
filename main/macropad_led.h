#pragma once

#define MACROPAD_LED_GPIO 17
#define MACROPAD_LED_NUM 10

void macropad_led_init();
void macropad_led_toggle_enabled(); // key 1
void macropad_led_cycle_effects(); // key 2
void macropad_led_cycle_effect_variants(); // key 3
void macropad_led_cycle_brightness(); // key 4
// void macropad_led_toggle_timer(); // key 5