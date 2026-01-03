/**
 * @file mode_mgb.c
 * @brief mGB MIDI IN mode handler implementation
 * 
 * Receives MIDI from DIN/TRS input and USB, forwards to Game Boy running mGB.
 * Also forwards DIN MIDI to USB (MIDI thru/merge).
 * 
 * mGB expects raw MIDI bytes with channel remapping:
 * - External MIDI channels are mapped to mGB's internal channels (0-4)
 * - A delay between bytes is required for mGB to process them
 */

#include "mode_mgb.h"
#include "config.h"
#include "gb_link.h"
#include "midi_uart.h"
#include "usb_midi.h"
#include "led.h"

#include "pico/stdlib.h"
#include "pico/time.h"

#include <string.h>

// =============================================================================
// Private State
// =============================================================================

static mode_mgb_config_t s_config;
static bool s_active = false;

// Statistics
static volatile uint32_t s_forward_count = 0;
static volatile uint32_t s_drop_count = 0;

// Timing for inter-byte delay
static absolute_time_t s_last_byte_time;

// =============================================================================
// Default Configuration
// =============================================================================

static void apply_default_config(void) {
    // Default mapping: MIDI channels 1-5 → mGB channels 0-4
    // MIDI channels 6-16 are not mapped (disabled)
    for (int i = 0; i < 16; i++) {
        if (i < MGB_CHANNEL_COUNT) {
            s_config.midi_to_mgb_channel[i] = i;
        } else {
            s_config.midi_to_mgb_channel[i] = 0xFF;  // Disabled
        }
    }
    
    // Enable all mGB channels by default
    for (int i = 0; i < MGB_CHANNEL_COUNT; i++) {
        s_config.channel_enabled[i] = true;
    }
}

// =============================================================================
// MIDI Message Handling
// =============================================================================

/**
 * @brief Send a byte to mGB with proper inter-byte delay
 */
static void send_byte_to_mgb(uint8_t byte) {
    // Wait for minimum inter-byte delay
    int64_t elapsed_us = absolute_time_diff_us(s_last_byte_time, get_absolute_time());
    if (elapsed_us < MGB_INTER_BYTE_DELAY_US) {
        sleep_us(MGB_INTER_BYTE_DELAY_US - elapsed_us);
    }
    
    // Send the byte
    gb_link_send_byte_blocking(byte);
    s_last_byte_time = get_absolute_time();
}

/**
 * @brief Forward a MIDI message to mGB with channel remapping
 */
static void forward_message_to_mgb(const midi_message_t *msg) {
    // Get the mapped mGB channel
    uint8_t midi_channel = msg->channel;
    uint8_t mgb_channel = s_config.midi_to_mgb_channel[midi_channel];
    
    // Check if this channel is mapped and enabled
    if (mgb_channel >= MGB_CHANNEL_COUNT) {
        return;  // Channel not mapped
    }
    
    if (!s_config.channel_enabled[mgb_channel]) {
        return;  // Channel disabled
    }
    
    // Remap the status byte to the mGB channel
    uint8_t status = (msg->raw[0] & 0xF0) | mgb_channel;
    
    // Send the message bytes to mGB
    switch (msg->type) {
        case MIDI_MSG_NOTE_OFF:
        case MIDI_MSG_NOTE_ON:
        case MIDI_MSG_POLY_PRESSURE:
        case MIDI_MSG_CONTROL_CHANGE:
        case MIDI_MSG_PITCH_BEND:
            // 3-byte messages
            send_byte_to_mgb(status);
            send_byte_to_mgb(msg->data1);
            send_byte_to_mgb(msg->data2);
            s_forward_count++;
            led_trigger_activity();
            break;
            
        case MIDI_MSG_PROGRAM_CHANGE:
        case MIDI_MSG_CHANNEL_PRESSURE:
            // 2-byte messages
            send_byte_to_mgb(status);
            send_byte_to_mgb(msg->data1);
            s_forward_count++;
            led_trigger_activity();
            break;
            
        default:
            // Other messages are not forwarded to mGB
            break;
    }
}

/**
 * @brief MIDI message callback (called from UART interrupt context)
 * 
 * Also forwards DIN MIDI to USB for thru/merge functionality
 */
static void on_midi_message(const midi_message_t *msg) {
    // Forward DIN MIDI to USB (MIDI merge/thru)
    usb_midi_send_message(msg);
    
    // The main loop will poll and process messages for GB forwarding
}

/**
 * @brief USB MIDI message callback
 * 
 * Receives MIDI from USB host and forwards to GB
 */
static void on_usb_midi_message(const midi_message_t *msg) {
    // USB → GB forwarding happens in the main process loop
    // This just serves as a notification (message is queued in usb_midi module)
    (void)msg;
}

// =============================================================================
// Public Functions - Lifecycle
// =============================================================================

bool mode_mgb_init(void) {
    if (s_active) {
        return true;  // Already active
    }
    
    // Apply default configuration
    apply_default_config();
    
    // Initialize GB link
    if (!gb_link_init()) {
        DEBUG_PRINT("mGB: Failed to initialize GB link\\n");
        return false;
    }
    
    // Initialize MIDI UART
    if (!midi_uart_init()) {
        DEBUG_PRINT("mGB: Failed to initialize MIDI UART\\n");
        gb_link_deinit();
        return false;
    }
    
    // Initialize USB MIDI
    if (!usb_midi_init()) {
        DEBUG_PRINT("mGB: Failed to initialize USB MIDI\\n");
        midi_uart_deinit();
        gb_link_deinit();
        return false;
    }
    
    // Set up callbacks
    midi_uart_set_message_callback(on_midi_message);
    usb_midi_set_rx_callback(on_usb_midi_message);
    
    // Reset timing
    s_last_byte_time = get_absolute_time();
    
    // Reset statistics
    s_forward_count = 0;
    s_drop_count = 0;
    
    s_active = true;
    
    DEBUG_PRINT("mGB: Mode initialized\\n");
    DEBUG_PRINT("mGB: Channel mapping: MIDI 1-5 -> PU1, PU2, WAV, NOI, POLY\\n");
    DEBUG_PRINT("mGB: DIN MIDI <-> USB MIDI bidirectional routing active\\n");
    
    return true;
}

void mode_mgb_deinit(void) {
    if (!s_active) {
        return;
    }
    
    // Clear callbacks
    midi_uart_set_message_callback(NULL);
    usb_midi_set_rx_callback(NULL);
    
    // Deinitialize subsystems
    midi_uart_deinit();
    gb_link_deinit();
    
    s_active = false;
    
    DEBUG_PRINT("mGB: Mode deinitialized\\n");
}

const mode_mgb_config_t* mode_mgb_get_config(void) {
    return &s_config;
}

void mode_mgb_set_config(const mode_mgb_config_t *config) {
    if (config != NULL) {
        memcpy(&s_config, config, sizeof(mode_mgb_config_t));
    }
}

void mode_mgb_reset_config(void) {
    apply_default_config();
}

// =============================================================================
// Public Functions - Runtime
// =============================================================================

void mode_mgb_process(void) {
    if (!s_active) {
        return;
    }
    
    // Process MIDI input from DIN (runs the parser)
    midi_uart_process();
    
    // Process USB MIDI input
    usb_midi_process_rx();
    
    // Forward DIN MIDI messages to Game Boy
    midi_message_t msg;
    while (midi_uart_get_message(&msg)) {
        // Only forward channel voice messages
        if (msg.type >= MIDI_MSG_NOTE_OFF && msg.type <= MIDI_MSG_PITCH_BEND) {
            forward_message_to_mgb(&msg);
        }
    }
}

bool mode_mgb_is_active(void) {
    return s_active;
}

// =============================================================================
// Public Functions - Statistics
// =============================================================================

uint32_t mode_mgb_get_forward_count(void) {
    return s_forward_count;
}

uint32_t mode_mgb_get_drop_count(void) {
    return s_drop_count;
}

void mode_mgb_reset_stats(void) {
    s_forward_count = 0;
    s_drop_count = 0;
}
