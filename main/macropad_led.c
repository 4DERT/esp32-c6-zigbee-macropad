#include "macropad_led.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "lsfx.h"
#include "lsfx_fx_bicolor.h"
#include "lsfx_fx_police.h"
#include "lsfx_fx_rainbow.h"
#include "lsfx_fx_static.h"

static const char* TAG = "LED_CTRL";

// 2D DATA STRUCTURE (EFFECTS -> VARIANTS)
typedef struct {
    const lsfx_fx_t* fx_func; // Pointer to the effect function (e.g., &lsfx_fx_rainbow_t)
    const void* fx_params;    // Pointer to the parameters (e.g., &g_rainbow_10s)
    const char* variant_name; // Name for logging
} effect_variant_t;

typedef struct {
    const char* effect_name;          // Name for logging (e.g., "Rainbow")
    const effect_variant_t* variants; // Pointer to the array of variants
    uint8_t num_variants;             // Size of the variants array
} app_effect_t;

// --- List of Variants for STATIC Effect ---
static const effect_variant_t static_variants[] = {
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 0, .blue = 0}, "Red"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 0, .green = 255, .blue = 0}, "Green"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 0, .green = 0, .blue = 255}, "Blue"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 255, .blue = 255}, "White"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 255, .blue = 0}, "Yellow"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 128, .blue = 0}, "Orange"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 128, .green = 0, .blue = 255}, "Purple"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 0, .green = 255, .blue = 255}, "Cyan"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 0, .blue = 255}, "Magenta"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 255, .green = 64, .blue = 128}, "Pink"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 64, .green = 255, .blue = 128}, "Mint"},
    {&lsfx_fx_static_t, &(const lsfx_fx_static_params_t){.red = 0, .green = 128, .blue = 255}, "Sky Blue"},
};
#define STATIC_VARIANTS_SIZE (sizeof(static_variants) / sizeof(static_variants[0]))

// --- List of Variants for RAINBOW Effect ---
static const effect_variant_t rainbow_variants[] = {
    {&lsfx_fx_rainbow_t, &(const lsfx_fx_rainbow_params_t){.period_ms = 2000}, "2s Period"},
    {&lsfx_fx_rainbow_t, &(const lsfx_fx_rainbow_params_t){.period_ms = 30000}, "30s Period"},
    {&lsfx_fx_rainbow_t, &(const lsfx_fx_rainbow_params_t){.period_ms = 60000}, "60s Period"},
};
#define RAINBOW_VARIANTS_SIZE (sizeof(rainbow_variants) / sizeof(rainbow_variants[0]))

// --- List of Variants for POLICE Effect ---
static const effect_variant_t police_variants[] = {
    {&lsfx_fx_police_t, &(const lsfx_fx_police_params_t){.period_ms = 100}, "100ms Period"},
    {&lsfx_fx_police_t, &(const lsfx_fx_police_params_t){.period_ms = 200}, "200ms Period"},
    {&lsfx_fx_police_t, &(const lsfx_fx_police_params_t){.period_ms = 400}, "400ms Period"},
};
#define POLICE_VARIANTS_SIZE (sizeof(police_variants) / sizeof(police_variants[0]))

// --- List of Variants for BICOLOR Effect ---
// clang-format off
static const effect_variant_t bicolor_variants[] = {
    // 1. Warm sunset gradient (red → orange)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 255, .g1 = 64,  .b1 = 0,      // top: deep orange-red
        .r2 = 255, .g2 = 180, .b2 = 64 },   // bottom: warm amber
      "sunset glow" },

    // 2. Ocean tones (turquoise → deep blue)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 0,   .g1 = 255, .b1 = 180,    // top: aqua turquoise
        .r2 = 0,   .g2 = 64,  .b2 = 255 },  // bottom: deep ocean blue
      "ocean" },

    // 3. Aurora colors (green → purple)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 0,   .g1 = 255, .b1 = 128,    // top: vivid green-blue
        .r2 = 128, .g2 = 0,   .b2 = 255 },  // bottom: magenta-purple
      "aurora" },

    // 4. Cyberpunk neon (pink → blue)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 255, .g1 = 0,   .b1 = 128,    // top: magenta-pink
        .r2 = 0,   .g2 = 128, .b2 = 255 },  // bottom: neon blue
      "synthwave" },

    // 5. Ice & fire contrast (blue → orange)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 0,   .g1 = 64,  .b1 = 255,    // top: cold blue
        .r2 = 255, .g2 = 128, .b2 = 0 },    // bottom: warm orange
      "ice & fire" },

    // 6. Forest shades (dark green → light green)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 0,   .g1 = 80,  .b1 = 0,      // top: deep green
        .r2 = 64,  .g2 = 255, .b2 = 64 },   // bottom: bright green
      "forest" },

    // 7. Twilight mood (navy → pink)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 32,  .g1 = 0,   .b1 = 128,    // top: dark navy
        .r2 = 255, .g2 = 64,  .b2 = 128 },  // bottom: warm pink
      "twilight" },

    // 8. Cool neutral (white → blue)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 255, .g1 = 255, .b1 = 255,    // top: pure white
        .r2 = 64,  .g2 = 128, .b2 = 255 },  // bottom: cool blue
      "white-blue" },

    // 9. Royal contrast (gold → violet)
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 255, .g1 = 200, .b1 = 64,     // top: gold
        .r2 = 160, .g2 = 0,   .b2 = 255 },  // bottom: violet
      "royal" },

    // 10. Classic red-blue look
    { &lsfx_fx_bicolor_t, &(const lsfx_fx_bicolor_params_t){
        .r1 = 255, .g1 = 0,   .b1 = 0,      // top: red
        .r2 = 0,   .g2 = 0,   .b2 = 255 },  // bottom: blue
      "red-blue" },
};
// clang-format on
#define BICOLOR_VARIANTS_SIZE (sizeof(bicolor_variants) / sizeof(bicolor_variants[0]))

// --- THE MASTER EFFECT LIST  ---
static const app_effect_t app_effects[] = {
    {"Static", static_variants, STATIC_VARIANTS_SIZE},
    {"Rainbow", rainbow_variants, RAINBOW_VARIANTS_SIZE},
    {"Police", police_variants, POLICE_VARIANTS_SIZE},
    {"BiColor", bicolor_variants, BICOLOR_VARIANTS_SIZE},
};
#define NUM_EFFECTS (sizeof(app_effects) / sizeof(app_effects[0]))

// -- Brightness --
static const uint8_t brightness_levels[] = {9, 21, 49, 84, 135, 199, 255};
#define BRIGHTNESS_LEVELS_NUM (sizeof(brightness_levels) / sizeof(brightness_levels[0]))

// Module state
static lsfx_handle_t mlsfx;
static uint8_t current_effect_index = 0;  // Index for app_effects
static uint8_t current_variant_index = 0; // Index for ..._variants
static uint8_t current_brightness_index = BRIGHTNESS_LEVELS_NUM - 1;
static bool is_enabled = false;

static inline void apply_current_fx(void) {
    const app_effect_t* effect = &app_effects[current_effect_index];
    const effect_variant_t* var = &effect->variants[current_variant_index];
    lsfx_set_fx(mlsfx, var->fx_func, var->fx_params);
}

static inline void log_state(const char* prefix) {
    const app_effect_t* effect = &app_effects[current_effect_index];
    const effect_variant_t* var = &effect->variants[current_variant_index];
    ESP_LOGI(TAG, "%sEffect=%s, Variant=%s, Brightness=%u", prefix ? prefix : "", effect->effect_name, var->variant_name,
             brightness_levels[current_brightness_index]);
}

// PUBLIC API

void macropad_led_init() {
    // LED strip general initialization
    led_strip_config_t strip_config = {
        .strip_gpio_num = MACROPAD_LED_GPIO,      // The GPIO that connected to the LED strip's data line
        .max_leds = MACROPAD_LED_NUM,             // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,      // different clock source can lead to different power consumption
        .resolution_hz = (10 * 1000 * 1000), // RMT counter clock frequency
        .flags.with_dma = false,             // DMA feature is available on ESP target like ESP32-S3
    };

    mlsfx = lsfx_init(strip_config, rmt_config);
    if (!mlsfx) {
        ESP_LOGE(TAG, "lsfx_init failed");
        return;
    }

    lsfx_set_brightness(mlsfx, brightness_levels[current_brightness_index]);
    lsfx_set_enabled(mlsfx, false);

    apply_current_fx();
    log_state("Init: ");
}

void macropad_led_toggle_enabled() {
    is_enabled = !is_enabled;
    lsfx_set_enabled(mlsfx, is_enabled);
    log_state("Toggle: ");
}

void macropad_led_cycle_effects() {
    current_effect_index = (current_effect_index + 1) % NUM_EFFECTS;
    current_variant_index = 0;

    apply_current_fx();
    log_state("CycleEffect: ");
}

void macropad_led_cycle_effect_variants() {
    const uint8_t variants_num = app_effects[current_effect_index].num_variants;
    current_variant_index = (current_variant_index + 1) % variants_num;

    apply_current_fx();
    log_state("CycleVariant: ");
}

void macropad_led_cycle_brightness() {
    current_brightness_index = (current_brightness_index + 1) % BRIGHTNESS_LEVELS_NUM;
    lsfx_set_brightness(mlsfx, brightness_levels[current_brightness_index]);
    log_state("CycleBrightness: ");
}