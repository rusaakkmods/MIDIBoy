/**
 * @file led.c
 * @brief LED indicator driver implementation
 * 
 * Provides activity indication via GPIO LED.
 * Uses timer-based auto-off to avoid blocking.
 */

#include "led.h"
#include "config.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

// =============================================================================
// Private State
// =============================================================================

static bool s_led_on = false;
static absolute_time_t s_off_time;
static bool s_auto_off_pending = false;

// Blink pattern state
static bool s_blinking = false;
static uint8_t s_blink_count = 0;
static uint8_t s_blink_remaining = 0;
static uint16_t s_blink_on_ms = 0;
static uint16_t s_blink_off_ms = 0;
static absolute_time_t s_blink_next_time;
static bool s_blink_phase_on = false;

// =============================================================================
// Private Functions
// =============================================================================

static void led_set_raw(bool on) {
    gpio_put(PIN_LED_ACTIVITY, on);
    s_led_on = on;
}

// =============================================================================
// Public Functions
// =============================================================================

void led_init(void) {
    // Configure activity LED pin
    gpio_init(PIN_LED_ACTIVITY);
    gpio_set_dir(PIN_LED_ACTIVITY, GPIO_OUT);
    gpio_put(PIN_LED_ACTIVITY, 0);
    
    s_led_on = false;
    s_auto_off_pending = false;
    s_blinking = false;
    
    DEBUG_PRINT("LED: Initialized on GPIO%d\n", PIN_LED_ACTIVITY);
}

void led_trigger_activity(void) {
    // Don't interrupt a blink pattern
    if (s_blinking) {
        return;
    }
    
    // Turn on LED and schedule auto-off
    led_set_raw(true);
    s_off_time = make_timeout_time_ms(LED_BLINK_DURATION_MS);
    s_auto_off_pending = true;
}

void led_set(bool on) {
    // Cancel any pending auto-off or blink pattern
    s_auto_off_pending = false;
    s_blinking = false;
    led_set_raw(on);
}

void led_toggle(void) {
    led_set(!s_led_on);
}

void led_update(void) {
    // Handle blink pattern
    if (s_blinking) {
        if (time_reached(s_blink_next_time)) {
            if (s_blink_phase_on) {
                // Was on, turn off
                led_set_raw(false);
                s_blink_phase_on = false;
                s_blink_next_time = make_timeout_time_ms(s_blink_off_ms);
            } else {
                // Was off, either turn on for next blink or finish
                s_blink_remaining--;
                if (s_blink_remaining > 0) {
                    led_set_raw(true);
                    s_blink_phase_on = true;
                    s_blink_next_time = make_timeout_time_ms(s_blink_on_ms);
                } else {
                    // Pattern complete
                    s_blinking = false;
                }
            }
        }
        return;
    }
    
    // Handle auto-off
    if (s_auto_off_pending && time_reached(s_off_time)) {
        led_set_raw(false);
        s_auto_off_pending = false;
    }
}

void led_blink_pattern(uint8_t count, uint16_t on_ms, uint16_t off_ms) {
    if (count == 0) {
        s_blinking = false;
        led_set_raw(false);
        return;
    }
    
    s_blink_count = count;
    s_blink_remaining = count;
    s_blink_on_ms = on_ms;
    s_blink_off_ms = off_ms;
    s_blinking = true;
    s_auto_off_pending = false;
    
    // Start first blink
    led_set_raw(true);
    s_blink_phase_on = true;
    s_blink_next_time = make_timeout_time_ms(on_ms);
}

bool led_is_blinking(void) {
    return s_blinking;
}
