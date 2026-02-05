#include "macropad_led.h"

#include "esp_log.h"
#include "driver/gpio.h"

#define MACROPAD_LED_GPIO CONFIG_MACROPAD_LED_GPIO

static const char* TAG = "LED_CTRL_GPIO";
static bool is_enabled = false;

void macropad_led_init() {
    ESP_LOGI(TAG, "Initializing LED on GPIO %d", MACROPAD_LED_GPIO);

    gpio_set_direction(MACROPAD_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(MACROPAD_LED_GPIO, 0);
    is_enabled = false;
}

void macropad_led_toggle_enabled() {
    is_enabled = !is_enabled;
    gpio_set_level(MACROPAD_LED_GPIO, is_enabled);
    ESP_LOGI(TAG, "LED state: %s", is_enabled ? "ON" : "OFF");
}

void macropad_led_set_enabled(bool state) {
    is_enabled = state;
    gpio_set_level(MACROPAD_LED_GPIO, is_enabled);
    ESP_LOGI(TAG, "LED state: %s", is_enabled ? "ON" : "OFF");
}

inline bool macropad_led_get_enabled() {
    return is_enabled;
}

// --- Empty functions (stubs) ---
void macropad_led_cycle_effects() {
    ESP_LOGD(TAG, "CycleEffect: No-op (single LED mode)");
}

void macropad_led_cycle_effect_variants() {
    ESP_LOGD(TAG, "CycleVariant: No-op (single LED mode)");
}

void macropad_led_cycle_brightness() {
    ESP_LOGD(TAG, "CycleBrightness: No-op (single LED mode)");
}