/**
 * @file usb_midi.c
 * @brief USB-MIDI device interface implementation
 * 
 * Uses TinyUSB to provide MIDI device functionality.
 * Handles bidirectional MIDI routing between USB and DIN.
 */

#include "usb_midi.h"
#include "config.h"

#include "tusb.h"
#include "pico/stdlib.h"

#include <string.h>

// =============================================================================
// Private State
// =============================================================================

static midi_message_callback_t s_rx_callback = NULL;

// Statistics
static volatile uint32_t s_rx_count = 0;
static volatile uint32_t s_tx_count = 0;

static bool s_initialized = false;

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Get message type from status byte
 */
static midi_message_type_t get_message_type(uint8_t status) {
    switch (status & 0xF0) {
        case 0x80: return MIDI_MSG_NOTE_OFF;
        case 0x90: return MIDI_MSG_NOTE_ON;
        case 0xA0: return MIDI_MSG_POLY_PRESSURE;
        case 0xB0: return MIDI_MSG_CONTROL_CHANGE;
        case 0xC0: return MIDI_MSG_PROGRAM_CHANGE;
        case 0xD0: return MIDI_MSG_CHANNEL_PRESSURE;
        case 0xE0: return MIDI_MSG_PITCH_BEND;
        case 0xF0:
            switch (status) {
                case 0xF0: return MIDI_MSG_SYSEX_START;
                case 0xF1: return MIDI_MSG_MTC_QUARTER;
                case 0xF2: return MIDI_MSG_SONG_POSITION;
                case 0xF3: return MIDI_MSG_SONG_SELECT;
                case 0xF6: return MIDI_MSG_TUNE_REQUEST;
                case 0xF7: return MIDI_MSG_SYSEX_END;
                case 0xF8: return MIDI_MSG_CLOCK;
                case 0xFA: return MIDI_MSG_START;
                case 0xFB: return MIDI_MSG_CONTINUE;
                case 0xFC: return MIDI_MSG_STOP;
                case 0xFE: return MIDI_MSG_ACTIVE_SENSING;
                case 0xFF: return MIDI_MSG_SYSTEM_RESET;
                default:   return MIDI_MSG_NONE;
            }
        default:
            return MIDI_MSG_NONE;
    }
}

/**
 * @brief Parse USB-MIDI packet into midi_message_t
 */
static void parse_usb_midi_packet(uint8_t const packet[4], midi_message_t *msg) {
    uint8_t cable_number = (packet[0] >> 4) & 0x0F;
    uint8_t code_index = packet[0] & 0x0F;
    
    (void)cable_number; // We only use cable 0
    
    // Extract the MIDI bytes
    msg->raw[0] = packet[1];
    msg->raw[1] = packet[2];
    msg->raw[2] = packet[3];
    
    // Determine message type and length based on code index
    switch (code_index) {
        case 0x02: // Two-byte System Common
        case 0x06: // System Common (1 byte)
        case 0x0C: // Program Change
        case 0x0D: // Channel Pressure
            msg->length = 2;
            msg->type = get_message_type(msg->raw[0]);
            msg->channel = msg->raw[0] & 0x0F;
            msg->data1 = msg->raw[1];
            msg->data2 = 0;
            break;
            
        case 0x03: // Three-byte System Common
        case 0x08: // Note Off
        case 0x09: // Note On
        case 0x0A: // Poly Pressure
        case 0x0B: // Control Change
        case 0x0E: // Pitch Bend
            msg->length = 3;
            msg->type = get_message_type(msg->raw[0]);
            msg->channel = msg->raw[0] & 0x0F;
            msg->data1 = msg->raw[1];
            msg->data2 = msg->raw[2];
            break;
            
        case 0x05: // Single-byte System Common
        case 0x0F: // Single byte (Real-time)
            msg->length = 1;
            msg->type = get_message_type(msg->raw[0]);
            msg->channel = 0;
            msg->data1 = 0;
            msg->data2 = 0;
            break;
            
        default:
            msg->length = 0;
            msg->type = MIDI_MSG_NONE;
            break;
    }
}

/**
 * @brief Get USB-MIDI code index for a MIDI message
 */
static uint8_t get_usb_midi_code_index(const midi_message_t *msg) {
    switch (msg->type) {
        case MIDI_MSG_NOTE_OFF:         return 0x08;
        case MIDI_MSG_NOTE_ON:          return 0x09;
        case MIDI_MSG_POLY_PRESSURE:    return 0x0A;
        case MIDI_MSG_CONTROL_CHANGE:   return 0x0B;
        case MIDI_MSG_PROGRAM_CHANGE:   return 0x0C;
        case MIDI_MSG_CHANNEL_PRESSURE: return 0x0D;
        case MIDI_MSG_PITCH_BEND:       return 0x0E;
        
        // System messages
        case MIDI_MSG_SYSEX_START:      return 0x04;
        case MIDI_MSG_SYSEX_END:        return 0x05;
        case MIDI_MSG_MTC_QUARTER:      return 0x02;
        case MIDI_MSG_SONG_POSITION:    return 0x03;
        case MIDI_MSG_SONG_SELECT:      return 0x02;
        case MIDI_MSG_TUNE_REQUEST:     return 0x05;
        
        // Real-time messages
        case MIDI_MSG_CLOCK:            return 0x0F;
        case MIDI_MSG_START:            return 0x0F;
        case MIDI_MSG_CONTINUE:         return 0x0F;
        case MIDI_MSG_STOP:             return 0x0F;
        case MIDI_MSG_ACTIVE_SENSING:   return 0x0F;
        case MIDI_MSG_SYSTEM_RESET:     return 0x0F;
        
        default:                        return 0x00;
    }
}

// =============================================================================
// Public Functions
// =============================================================================

bool usb_midi_init(void) {
    if (s_initialized) {
        return true;
    }
    
    // TinyUSB initialization is handled by tusb_init() in main
    // Just reset our state here
    s_rx_callback = NULL;
    s_rx_count = 0;
    s_tx_count = 0;
    s_initialized = true;
    
    DEBUG_PRINT("USB-MIDI: Initialized (waiting for host)\n");
    
    return true;
}

bool usb_midi_is_mounted(void) {
    return tud_mounted();
}

void usb_midi_process_rx(void) {
    if (!s_initialized) {
        return;
    }
    
    // Read all available MIDI packets
    uint8_t packet[4];
    while (tud_midi_available()) {
        if (tud_midi_packet_read(packet)) {
            s_rx_count++;
            
            // Parse and dispatch the message
            if (s_rx_callback != NULL) {
                midi_message_t msg;
                parse_usb_midi_packet(packet, &msg);
                
                if (msg.length > 0) {
                    s_rx_callback(&msg);
                }
            }
        }
    }
}

void usb_midi_set_rx_callback(midi_message_callback_t callback) {
    s_rx_callback = callback;
}

bool usb_midi_send_message(const midi_message_t *msg) {
    if (!s_initialized || !tud_mounted() || msg == NULL || msg->length == 0) {
        return false;
    }
    
    // Build USB-MIDI packet
    uint8_t packet[4];
    packet[0] = get_usb_midi_code_index(msg); // Cable 0 + Code Index
    packet[1] = msg->raw[0];
    packet[2] = (msg->length > 1) ? msg->raw[1] : 0;
    packet[3] = (msg->length > 2) ? msg->raw[2] : 0;
    
    if (tud_midi_packet_write(packet)) {
        s_tx_count++;
        return true;
    }
    
    return false;
}

bool usb_midi_send_raw(const uint8_t *bytes, uint8_t length) {
    if (!s_initialized || !tud_mounted() || bytes == NULL || length == 0 || length > 3) {
        return false;
    }
    
    // Create a message structure from raw bytes
    midi_message_t msg;
    msg.raw[0] = bytes[0];
    msg.raw[1] = (length > 1) ? bytes[1] : 0;
    msg.raw[2] = (length > 2) ? bytes[2] : 0;
    msg.length = length;
    msg.type = get_message_type(bytes[0]);
    msg.channel = bytes[0] & 0x0F;
    msg.data1 = msg.raw[1];
    msg.data2 = msg.raw[2];
    
    return usb_midi_send_message(&msg);
}

uint32_t usb_midi_get_rx_count(void) {
    return s_rx_count;
}

uint32_t usb_midi_get_tx_count(void) {
    return s_tx_count;
}

void usb_midi_reset_stats(void) {
    s_rx_count = 0;
    s_tx_count = 0;
}
