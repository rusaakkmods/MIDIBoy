/**
 * @file config.h
 * @brief MIDIBoy hardware configuration and pin definitions
 * 
 * This file contains all hardware-specific configuration for the MIDIBoy
 * firmware running on RP2040 Zero. Pin assignments match the POC v2 hardware.
 */

#ifndef MIDIBOY_CONFIG_H
#define MIDIBOY_CONFIG_H

#include "pico/stdlib.h"

// =============================================================================
// Version Information
// =============================================================================
#define MIDIBOY_VERSION_MAJOR   0
#define MIDIBOY_VERSION_MINOR   1
#define MIDIBOY_VERSION_PATCH   0

// =============================================================================
// Game Boy Link Interface Pins (directly from POC v2 BOM)
// =============================================================================
// GB_SO (Serial Out from Game Boy) - directly to GP2 via 10k series resistor
#define PIN_GB_SO           2

// GB_SI (Serial In to Game Boy) - from GP3 via 100Ω series resistor  
#define PIN_GB_SI           3

// GB_SC (Serial Clock) - directly to GP4 via 10k series resistor
#define PIN_GB_SC           4

// =============================================================================
// MIDI Interface Pins (UART1)
// =============================================================================
// MIDI OUT TX - GP8 via transistor driver circuit
#define PIN_MIDI_TX         8

// MIDI IN RX - GP9 via 6N137 optocoupler
#define PIN_MIDI_RX         9

// UART instance for MIDI
#define MIDI_UART_ID        uart1
#define MIDI_BAUD_RATE      31250

// =============================================================================
// LED Indicator
// =============================================================================
#define PIN_LED_ACTIVITY    15

// Onboard LED (RP2040 Zero has WS2812 on GPIO16, but we use simple GPIO)
#define PIN_LED_ONBOARD     25  // Standard Pico LED pin (if available)

// =============================================================================
// Timing Configuration
// =============================================================================
// mGB expects ~500µs delay between bytes (from Arduinoboy reference)
#define MGB_INTER_BYTE_DELAY_US     500

// Game Boy link clock period (approx 122µs for ~8kHz clock)
// The PIO will handle precise timing
#define GB_LINK_BIT_PERIOD_US       8

// LED blink duration for activity indication
#define LED_BLINK_DURATION_MS       50

// =============================================================================
// Buffer Sizes
// =============================================================================
// MIDI receive ring buffer size (must be power of 2)
#define MIDI_RX_BUFFER_SIZE         256

// GB link transmit queue size (must be power of 2)
#define GB_TX_QUEUE_SIZE            64

// =============================================================================
// Operating Modes
// =============================================================================
typedef enum {
    MODE_MGB_MIDI_IN = 0,       // Stage 1: MIDI → mGB
    MODE_LSDJ_SYNC_IN,          // Stage 2: External clock → LSDJ
    MODE_LSDJ_SYNC_OUT,         // Stage 3: LSDJ → External clock
    MODE_NANOLOOP_SYNC_IN,      // Stage 4: Nanoloop sync
    MODE_NANOLOOP_SYNC_OUT,
    MODE_STEPPER_SYNC_IN,
    MODE_STEPPER_SYNC_OUT,
    MODE_LSDJ_MIDI_IN,          // Stage 5: MIDI → LSDJ (MIDI-GB)
    MODE_LSDJ_MIDI_OUT,         // Stage 6: LSDJ MI.OUT → MIDI
    MODE_COUNT
} midiboy_mode_t;

// =============================================================================
// Core Assignment
// =============================================================================
// Core 0: Real-time tasks (PIO, MIDI UART, mode handlers)
// Core 1: Housekeeping (USB-MIDI in future, LED updates, mode switching)
#define CORE_REALTIME       0
#define CORE_HOUSEKEEPING   1

// =============================================================================
// Debug Configuration
// =============================================================================
#ifndef NDEBUG
    #define DEBUG_ENABLED       1
    #define DEBUG_PRINT(...)    printf(__VA_ARGS__)
#else
    #define DEBUG_ENABLED       0
    #define DEBUG_PRINT(...)    ((void)0)
#endif

#endif // MIDIBOY_CONFIG_H
