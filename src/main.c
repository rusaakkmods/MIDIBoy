/**
 * @file main.c
 * @brief MIDIBoy firmware entry point
 * 
 * MIDIBoy - RP2040 Zero based Game Boy MIDI interface
 * 
 * Architecture:
 * - Core 0: Real-time processing (MIDI parsing, GB link, mode handlers)
 * - Core 1: Housekeeping (LED updates, future USB-MIDI, mode switching)
 * 
 * Current implementation: Stage 1 - mGB MIDI IN mode
 * 
 * Hardware connections (POC v2):
 * - GP2: GB_SO (Serial Out from GB) - input
 * - GP3: GB_SI (Serial In to GB) - output  
 * - GP4: GB_SC (Serial Clock) - output (master mode)
 * - GP8: MIDI TX (UART1) - output
 * - GP9: MIDI RX (UART1) - input
 * - GP15: Activity LED - output
 */

#include "config.h"
#include "led.h"
#include "gb_link.h"
#include "usb_midi.h"
#include "mode_mgb.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "tusb.h"

#include <stdio.h>

// =============================================================================
// Core 1 Entry Point (Housekeeping)
// =============================================================================

/**
 * @brief Core 1 main function
 * 
 * Handles non-real-time tasks:
 * - LED updates
 * - USB device stack processing (TinyUSB)
 * - Future: Mode switching via button
 */
static void core1_main(void) {
    DEBUG_PRINT("Core 1: Started (housekeeping + USB)\n");
    
    while (true) {
        // Update LED state (handles auto-off and blink patterns)
        led_update();
        
        // Process USB device stack (TinyUSB)
        tud_task();
        
        // Small delay to prevent busy-looping
        // USB needs regular servicing but doesn't need ultra-high frequency
        sleep_us(100);
    }
}

// =============================================================================
// Startup Animation
// =============================================================================

static void startup_animation(void) {
    // Simple startup indication: 3 quick blinks
    led_blink_pattern(3, 100, 100);
    
    // Wait for pattern to complete
    while (led_is_blinking()) {
        led_update();
        sleep_ms(10);
    }
    
    sleep_ms(200);
}

// =============================================================================
// Debug Status Print
// =============================================================================

#if DEBUG_ENABLED
static absolute_time_t s_last_status_time;
static const uint32_t STATUS_INTERVAL_MS = 5000;

static void print_status(void) {
    if (absolute_time_diff_us(s_last_status_time, get_absolute_time()) < STATUS_INTERVAL_MS * 1000) {
        return;
    }
    s_last_status_time = get_absolute_time();
    
    printf("\n--- MIDIBoy Status ---\n");
    printf("Mode: mGB MIDI IN\n");
    printf("MIDI msgs forwarded: %lu\n", mode_mgb_get_forward_count());
    printf("GB bytes sent: %lu\n", gb_link_get_tx_count());
    printf("----------------------\n");
}
#else
#define print_status() ((void)0)
#endif

// =============================================================================
// Main Entry Point
// =============================================================================

int main(void) {
    // Initialize clock and basic peripherals
    // Note: stdio_init_all() is NOT called because we use USB for MIDI, not CDC
    
    // Wait a moment for hardware to stabilize
    sleep_ms(100);
    
    // Initialize LED first for visual feedback
    led_init();
    startup_animation();
    
    // Initialize TinyUSB
    tusb_init();
    
    // Wait for USB enumeration (show we're waiting)
    led_set(true);
    uint32_t wait_count = 0;
    while (!tud_mounted() && wait_count < 50) {  // Wait up to 5 seconds
        tud_task();
        sleep_ms(100);
        wait_count++;
    }
    led_set(false);
    
    // Print banner (only if USB CDC was enabled, but we disabled it)
    // In production, this won't print anywhere since stdio is disabled
    
    sleep_ms(500);
    
    // Initialize mGB mode (this sets up GB link and MIDI UART)
    if (!mode_mgb_init()) {
        // Error indication: fast blinking
        while (true) {
            led_toggle();
            sleep_ms(100);
        }
    }
    
    // Success indication: 2 quick blinks
    led_blink_pattern(2, 150, 150);
    while (led_is_blinking()) {
        led_update();
        sleep_ms(10);
    }
    
    // Start Core 1 for housekeeping tasks (LED + USB)
    multicore_launch_core1(core1_main);
    
    // ==========================================================================
    // Main Loop (Core 0 - Real-time)
    // ==========================================================================
    while (true) {
        // Process mGB mode (MIDI → GB link)
        // This also handles USB ↔ DIN MIDI routing
        mode_mgb_process();
        
        // Minimal delay - MIDI timing is critical
        // Minimal delay - MIDI timing is important
        // The MIDI parser uses interrupts, so we just need to 
        // process messages and feed them to GB link
        tight_loop_contents();
    }
    
    return 0;
}
