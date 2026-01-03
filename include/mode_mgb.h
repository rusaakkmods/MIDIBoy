/**
 * @file mode_mgb.h
 * @brief mGB MIDI IN mode handler
 * 
 * This mode turns the Game Boy running mGB into a MIDI synthesizer.
 * MIDI messages received from DIN/TRS MIDI IN are forwarded to mGB
 * via the Game Boy link cable.
 * 
 * mGB Protocol:
 * - mGB expects raw MIDI bytes over the link cable
 * - Channel mapping: MIDI channels 1-5 → mGB channels PU1, PU2, WAV, NOI, POLY
 * - Supports: Note On/Off, CC, Program Change, Pitch Bend
 * - No sync/clock needed
 * 
 * Reference: https://github.com/trash80/mGB
 */

#ifndef MODE_MGB_H
#define MODE_MGB_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// mGB Channel Mapping
// =============================================================================

/**
 * mGB channels (internal to Game Boy):
 * - Channel 0: PU1 (Pulse 1)
 * - Channel 1: PU2 (Pulse 2)  
 * - Channel 2: WAV (Wave)
 * - Channel 3: NOI (Noise)
 * - Channel 4: POLY (Polyphonic mode using all channels)
 */
#define MGB_CHANNEL_PU1     0
#define MGB_CHANNEL_PU2     1
#define MGB_CHANNEL_WAV     2
#define MGB_CHANNEL_NOI     3
#define MGB_CHANNEL_POLY    4
#define MGB_CHANNEL_COUNT   5

// =============================================================================
// Configuration
// =============================================================================

/**
 * @brief mGB mode configuration
 */
typedef struct {
    // MIDI channel mapping: midi_channel[n] = mGB channel for MIDI ch n+1
    // Default: MIDI 1→PU1, MIDI 2→PU2, MIDI 3→WAV, MIDI 4→NOI, MIDI 5→POLY
    uint8_t midi_to_mgb_channel[16];
    
    // Enable/disable channels
    bool channel_enabled[MGB_CHANNEL_COUNT];
} mode_mgb_config_t;

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Initialize mGB mode
 * 
 * Sets up GB link and MIDI UART for mGB operation.
 * 
 * @return true if initialization successful
 */
bool mode_mgb_init(void);

/**
 * @brief Deinitialize mGB mode
 * 
 * Releases resources. Call when switching to a different mode.
 */
void mode_mgb_deinit(void);

/**
 * @brief Get current mGB mode configuration
 * 
 * @return Pointer to current configuration
 */
const mode_mgb_config_t* mode_mgb_get_config(void);

/**
 * @brief Set mGB mode configuration
 * 
 * @param config New configuration to apply
 */
void mode_mgb_set_config(const mode_mgb_config_t *config);

/**
 * @brief Reset configuration to defaults
 */
void mode_mgb_reset_config(void);

// =============================================================================
// Runtime
// =============================================================================

/**
 * @brief Process mGB mode
 * 
 * Main processing function. Call this regularly from the main loop.
 * Processes MIDI input and forwards to Game Boy.
 */
void mode_mgb_process(void);

/**
 * @brief Check if mGB mode is active
 * 
 * @return true if mode is initialized and running
 */
bool mode_mgb_is_active(void);

// =============================================================================
// Statistics
// =============================================================================

/**
 * @brief Get count of MIDI messages forwarded to mGB
 */
uint32_t mode_mgb_get_forward_count(void);

/**
 * @brief Get count of dropped messages (due to full buffer)
 */
uint32_t mode_mgb_get_drop_count(void);

/**
 * @brief Reset statistics
 */
void mode_mgb_reset_stats(void);

#endif // MODE_MGB_H
