#pragma once

#include <stdbool.h>

void macropad_led_init();
void macropad_led_toggle_enabled(); // key 1
void macropad_led_set_enabled(bool state);
bool macropad_led_get_enabled();
void macropad_led_cycle_effects(); // key 2
void macropad_led_cycle_effect_variants(); // key 3
void macropad_led_cycle_brightness(); // key 4
// void macropad_led_toggle_timer(); // key 5
void macropad_led_start_loading();
void macropad_led_stop_loading();