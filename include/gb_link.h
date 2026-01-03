/**
 * @file gb_link.h
 * @brief Game Boy Link Cable interface driver
 * 
 * This module provides the interface for communicating with a Game Boy
 * via the link cable. It uses PIO for precise timing of the serial protocol.
 * 
 * The Game Boy link protocol is a synchronous serial interface:
 * - SC (Serial Clock): Directly to Game Boy
 * - SI (Serial In): Data to Game Boy  
 * - SO (Serial Out): Data from Game Boy
 * 
 * For mGB mode, we only need TX (sending MIDI to Game Boy).
 * Future modes (LSDJ MI.OUT) will add RX capability.
 */

#ifndef GB_LINK_H
#define GB_LINK_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize the Game Boy link interface
 * 
 * Sets up PIO for transmitting to the Game Boy.
 * Must be called before any other gb_link functions.
 * 
 * @return true if initialization successful
 */
bool gb_link_init(void);

/**
 * @brief Deinitialize the Game Boy link interface
 * 
 * Releases PIO resources. Call when switching modes that don't use GB link.
 */
void gb_link_deinit(void);

// =============================================================================
// Transmission (Master → Game Boy)
// =============================================================================

/**
 * @brief Send a byte to the Game Boy (non-blocking)
 * 
 * Queues a byte for transmission via PIO. Returns immediately.
 * 
 * @param data Byte to send
 * @return true if byte was queued, false if TX queue is full
 */
bool gb_link_send_byte(uint8_t data);

/**
 * @brief Send a byte to the Game Boy (blocking)
 * 
 * Blocks until the byte can be queued for transmission.
 * 
 * @param data Byte to send
 */
void gb_link_send_byte_blocking(uint8_t data);

/**
 * @brief Check if the TX queue has space
 * 
 * @return true if at least one byte can be queued
 */
bool gb_link_tx_ready(void);

/**
 * @brief Get number of bytes waiting in TX queue
 * 
 * @return Number of pending bytes
 */
uint8_t gb_link_tx_pending(void);

/**
 * @brief Flush the TX queue
 * 
 * Waits until all queued bytes have been transmitted.
 */
void gb_link_tx_flush(void);

// =============================================================================
// Statistics (for debugging)
// =============================================================================

/**
 * @brief Get total bytes transmitted
 * 
 * @return Count of bytes sent since initialization
 */
uint32_t gb_link_get_tx_count(void);

/**
 * @brief Reset transmission statistics
 */
void gb_link_reset_stats(void);

#endif // GB_LINK_H
