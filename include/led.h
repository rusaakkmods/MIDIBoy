/**
 * @file led.h
 * @brief LED indicator driver
 * 
 * Simple LED control for activity indication.
 * Uses a non-blocking approach with timed auto-off.
 */

#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize LED driver
 * 
 * Configures GPIO for activity LED.
 */
void led_init(void);

// =============================================================================
// LED Control
// =============================================================================

/**
 * @brief Trigger activity indication
 * 
 * Turns on the LED briefly to indicate activity.
 * Non-blocking - the LED will auto-off after a short duration.
 */
void led_trigger_activity(void);

/**
 * @brief Set LED state directly
 * 
 * @param on true to turn LED on, false to turn off
 */
void led_set(bool on);

/**
 * @brief Toggle LED state
 */
void led_toggle(void);

/**
 * @brief Update LED state
 * 
 * Call this regularly from the main loop to handle auto-off timing.
 */
void led_update(void);

// =============================================================================
// Blink Patterns
// =============================================================================

/**
 * @brief Start a blink pattern
 * 
 * @param count Number of blinks
 * @param on_ms Duration LED is on (ms)
 * @param off_ms Duration LED is off (ms)
 */
void led_blink_pattern(uint8_t count, uint16_t on_ms, uint16_t off_ms);

/**
 * @brief Check if a blink pattern is running
 * 
 * @return true if pattern is active
 */
bool led_is_blinking(void);

#endif // LED_H
