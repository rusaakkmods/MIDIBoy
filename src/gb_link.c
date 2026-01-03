/**
 * @file gb_link.c
 * @brief Game Boy Link Cable interface driver implementation
 * 
 * Uses PIO for precise timing of the Game Boy serial protocol.
 * Currently implements TX-only for mGB mode.
 */

#include "gb_link.h"
#include "config.h"
#include "gb_link_tx.pio.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include <string.h>

// =============================================================================
// Private State
// =============================================================================

// PIO configuration
static PIO      s_pio = pio0;
static uint     s_sm = 0;
static uint     s_pio_offset = 0;
static bool     s_initialized = false;

// Statistics
static volatile uint32_t s_tx_count = 0;

// Game Boy link clock frequency (Hz)
// The GB runs at ~8192 Hz internally, but mGB is flexible
// Arduinoboy uses slightly slower timing with delays
#define GB_LINK_CLOCK_HZ    8000

// =============================================================================
// Initialization
// =============================================================================

bool gb_link_init(void) {
    if (s_initialized) {
        return true;  // Already initialized
    }
    
    // Claim a state machine
    int sm = pio_claim_unused_sm(s_pio, false);
    if (sm < 0) {
        // Try the other PIO
        s_pio = pio1;
        sm = pio_claim_unused_sm(s_pio, false);
        if (sm < 0) {
            DEBUG_PRINT("GB Link: Failed to claim PIO state machine\n");
            return false;
        }
    }
    s_sm = (uint)sm;
    
    // Load the PIO program
    if (!pio_can_add_program(s_pio, &gb_link_tx_program)) {
        DEBUG_PRINT("GB Link: Failed to add PIO program\n");
        pio_sm_unclaim(s_pio, s_sm);
        return false;
    }
    
    s_pio_offset = pio_add_program(s_pio, &gb_link_tx_program);
    
    // Initialize the state machine
    gb_link_tx_program_init(
        s_pio, 
        s_sm, 
        s_pio_offset,
        PIN_GB_SI,          // Data to Game Boy
        PIN_GB_SC,          // Clock
        GB_LINK_CLOCK_HZ    // Bit rate
    );
    
    // Reset statistics
    s_tx_count = 0;
    s_initialized = true;
    
    DEBUG_PRINT("GB Link: Initialized on PIO%d SM%d\n", 
                (s_pio == pio0) ? 0 : 1, s_sm);
    
    return true;
}

void gb_link_deinit(void) {
    if (!s_initialized) {
        return;
    }
    
    // Disable and unclaim the state machine
    pio_sm_set_enabled(s_pio, s_sm, false);
    pio_remove_program(s_pio, &gb_link_tx_program, s_pio_offset);
    pio_sm_unclaim(s_pio, s_sm);
    
    s_initialized = false;
    
    DEBUG_PRINT("GB Link: Deinitialized\n");
}

// =============================================================================
// Transmission
// =============================================================================

bool gb_link_send_byte(uint8_t data) {
    if (!s_initialized) {
        return false;
    }
    
    if (gb_link_tx_try_put(s_pio, s_sm, data)) {
        s_tx_count++;
        return true;
    }
    
    return false;
}

void gb_link_send_byte_blocking(uint8_t data) {
    if (!s_initialized) {
        return;
    }
    
    gb_link_tx_put_blocking(s_pio, s_sm, data);
    s_tx_count++;
}

bool gb_link_tx_ready(void) {
    if (!s_initialized) {
        return false;
    }
    
    return gb_link_tx_fifo_has_space(s_pio, s_sm);
}

uint8_t gb_link_tx_pending(void) {
    if (!s_initialized) {
        return 0;
    }
    
    // PIO FIFO depth is 4 for TX
    // We can check the FIFO level
    return pio_sm_get_tx_fifo_level(s_pio, s_sm);
}

void gb_link_tx_flush(void) {
    if (!s_initialized) {
        return;
    }
    
    // Wait for FIFO to drain
    while (!pio_sm_is_tx_fifo_empty(s_pio, s_sm)) {
        tight_loop_contents();
    }
    
    // Also wait for the current byte to finish transmitting
    // Each byte takes approximately 8 * (1/8000) = 1ms plus inter-byte delay
    sleep_us(2000);
}

// =============================================================================
// Statistics
// =============================================================================

uint32_t gb_link_get_tx_count(void) {
    return s_tx_count;
}

void gb_link_reset_stats(void) {
    s_tx_count = 0;
}
