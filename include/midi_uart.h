/**
 * @file midi_uart.h
 * @brief MIDI UART receiver with parsing
 * 
 * This module handles receiving MIDI data from the DIN/TRS MIDI input
 * via UART. It provides a simple MIDI parser that handles:
 * - Channel voice messages (Note On/Off, CC, Program Change, Pitch Bend, etc.)
 * - Running status
 * - Real-time messages (passed through)
 * - System common messages (basic support)
 * 
 * Uses interrupt-driven reception with a ring buffer.
 */

#ifndef MIDI_UART_H
#define MIDI_UART_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// MIDI Message Types
// =============================================================================

typedef enum {
    MIDI_MSG_NONE = 0,
    
    // Channel Voice Messages
    MIDI_MSG_NOTE_OFF,          // 0x8n
    MIDI_MSG_NOTE_ON,           // 0x9n
    MIDI_MSG_POLY_PRESSURE,     // 0xAn
    MIDI_MSG_CONTROL_CHANGE,    // 0xBn
    MIDI_MSG_PROGRAM_CHANGE,    // 0xCn
    MIDI_MSG_CHANNEL_PRESSURE,  // 0xDn
    MIDI_MSG_PITCH_BEND,        // 0xEn
    
    // System Common Messages
    MIDI_MSG_SYSEX_START,       // 0xF0
    MIDI_MSG_MTC_QUARTER,       // 0xF1
    MIDI_MSG_SONG_POSITION,     // 0xF2
    MIDI_MSG_SONG_SELECT,       // 0xF3
    MIDI_MSG_TUNE_REQUEST,      // 0xF6
    MIDI_MSG_SYSEX_END,         // 0xF7
    
    // System Real-Time Messages
    MIDI_MSG_CLOCK,             // 0xF8
    MIDI_MSG_START,             // 0xFA
    MIDI_MSG_CONTINUE,          // 0xFB
    MIDI_MSG_STOP,              // 0xFC
    MIDI_MSG_ACTIVE_SENSING,    // 0xFE
    MIDI_MSG_SYSTEM_RESET,      // 0xFF
} midi_message_type_t;

/**
 * @brief Parsed MIDI message structure
 */
typedef struct {
    midi_message_type_t type;   // Message type
    uint8_t channel;            // MIDI channel (0-15) for channel messages
    uint8_t data1;              // First data byte (note, CC number, etc.)
    uint8_t data2;              // Second data byte (velocity, CC value, etc.)
    uint8_t raw[3];             // Raw bytes for pass-through
    uint8_t length;             // Number of valid bytes in raw[]
} midi_message_t;

// =============================================================================
// Callback Types
// =============================================================================

/**
 * @brief Callback for complete MIDI messages
 * 
 * Called from interrupt context - keep processing minimal!
 * 
 * @param msg Parsed MIDI message
 */
typedef void (*midi_message_callback_t)(const midi_message_t *msg);

/**
 * @brief Callback for raw MIDI bytes (for pass-through modes)
 * 
 * Called from interrupt context for every received byte.
 * 
 * @param byte Raw MIDI byte
 */
typedef void (*midi_byte_callback_t)(uint8_t byte);

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize MIDI UART receiver
 * 
 * Sets up UART1 at 31250 baud with interrupt-driven reception.
 * 
 * @return true if initialization successful
 */
bool midi_uart_init(void);

/**
 * @brief Deinitialize MIDI UART receiver
 */
void midi_uart_deinit(void);

// =============================================================================
// Callbacks
// =============================================================================

/**
 * @brief Set callback for parsed MIDI messages
 * 
 * @param callback Function to call when a complete message is received
 */
void midi_uart_set_message_callback(midi_message_callback_t callback);

/**
 * @brief Set callback for raw MIDI bytes
 * 
 * Useful for modes that need to pass through all bytes.
 * 
 * @param callback Function to call for each received byte
 */
void midi_uart_set_byte_callback(midi_byte_callback_t callback);

// =============================================================================
// Polling Interface (alternative to callbacks)
// =============================================================================

/**
 * @brief Check if a complete MIDI message is available
 * 
 * @return true if a message is ready to be read
 */
bool midi_uart_message_available(void);

/**
 * @brief Get the next complete MIDI message
 * 
 * @param msg Pointer to message structure to fill
 * @return true if a message was available
 */
bool midi_uart_get_message(midi_message_t *msg);

/**
 * @brief Process received MIDI data
 * 
 * Call this regularly from the main loop if using polling mode.
 * Processes any bytes in the receive buffer and calls registered callbacks.
 */
void midi_uart_process(void);

// =============================================================================
// Statistics
// =============================================================================

/**
 * @brief Get count of received MIDI bytes
 */
uint32_t midi_uart_get_rx_count(void);

/**
 * @brief Get count of complete messages received
 */
uint32_t midi_uart_get_message_count(void);

/**
 * @brief Get count of buffer overrun errors
 */
uint32_t midi_uart_get_error_count(void);

/**
 * @brief Reset all statistics
 */
void midi_uart_reset_stats(void);

#endif // MIDI_UART_H
