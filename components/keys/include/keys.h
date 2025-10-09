#pragma once
#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYS_MAX_NUM 32
#define KEYS_LONG_PRESS_MS 700

typedef enum {
    KEY_EVT_SINGLE = 0,
    KEY_EVT_DOUBLE,
    KEY_EVT_LONG,
} key_evt_t;

/**
 * @brief User callback for key events.
 * @param key_id 0..N-1 (index in provided GPIO list)
 * @param evt single/double/long
 * @param user_ctx opaque pointer provided at init
 */
typedef void (*keys_event_cb_t)(uint8_t key_id, key_evt_t evt, void* user_ctx);

/**
 * @brief Initialize N GPIO keys using espressif/button.
 *        Registers SINGLE/DOUBLE/LONG callbacks for each key.
 * @param gpio_list         array of GPIO numbers (length == num_keys)
 * @param num_keys          number of keys (1..KEYS_MAX_NUM)
 * @param is_active_on_low  true if button is active low (typical with pull-up)
 * @param cb                user callback for events (must not be NULL)
 * @param user_ctx          passed back as-is on each event (may be NULL)
 */
esp_err_t keys_init(const gpio_num_t* gpio_list, uint8_t num_keys, bool is_active_on_low, keys_event_cb_t cb, void* user_ctx);

/** Optional: free resources; events will stop after this call. */
void keys_deinit(void);

/**
 * @brief Get the numver of active keys
 * @return number of active keys
 */
uint8_t keys_count(void);

#ifdef __cplusplus
}
#endif