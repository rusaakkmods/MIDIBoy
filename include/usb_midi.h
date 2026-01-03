/**
 * @file usb_midi.h
 * @brief USB-MIDI device interface
 * 
 * Provides USB-MIDI functionality using TinyUSB.
 * Handles bidirectional MIDI communication:
 * - USB Host → MIDIBoy (received MIDI from computer)
 * - MIDIBoy → USB Host (send MIDI to computer)
 */

#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <stdint.h>
#include <stdbool.h>
#include "midi_uart.h"

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize USB-MIDI device
 * 
 * Sets up TinyUSB stack and configures as MIDI device.
 * Device will enumerate as "rMODS MIDIBoy".
 * 
 * @return true if initialization successful
 */
bool usb_midi_init(void);

/**
 * @brief Check if USB is mounted (connected and enumerated)
 * 
 * @return true if USB device is active
 */
bool usb_midi_is_mounted(void);

// =============================================================================
// USB → DIN MIDI (Receive from USB)
// =============================================================================

/**
 * @brief Process received USB-MIDI data
 * 
 * Call this regularly to handle incoming USB-MIDI messages.
 * Calls the registered callback for each received message.
 */
void usb_midi_process_rx(void);

/**
 * @brief Set callback for USB-MIDI messages
 * 
 * @param callback Function to call when MIDI received from USB
 */
void usb_midi_set_rx_callback(midi_message_callback_t callback);

// =============================================================================
// DIN → USB MIDI (Send to USB)
// =============================================================================

/**
 * @brief Send a MIDI message to USB host
 * 
 * @param msg MIDI message to send
 * @return true if message was sent
 */
bool usb_midi_send_message(const midi_message_t *msg);

/**
 * @brief Send raw MIDI bytes to USB host
 * 
 * @param bytes Array of MIDI bytes (1-3 bytes typically)
 * @param length Number of bytes to send
 * @return true if message was sent
 */
bool usb_midi_send_raw(const uint8_t *bytes, uint8_t length);

// =============================================================================
// Statistics
// =============================================================================

/**
 * @brief Get count of MIDI messages received from USB
 */
uint32_t usb_midi_get_rx_count(void);

/**
 * @brief Get count of MIDI messages sent to USB
 */
uint32_t usb_midi_get_tx_count(void);

/**
 * @brief Reset statistics
 */
void usb_midi_reset_stats(void);

#endif // USB_MIDI_H
