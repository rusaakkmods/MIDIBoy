/**
 * @file midi_uart.c
 * @brief MIDI UART receiver with parsing implementation
 * 
 * Implements interrupt-driven MIDI reception with a proper MIDI parser
 * that handles running status and all standard message types.
 */

#include "midi_uart.h"
#include "config.h"

#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

#include <string.h>

// =============================================================================
// Private Types
// =============================================================================

// Parser state machine
typedef enum {
    PARSER_IDLE,
    PARSER_STATUS,
    PARSER_DATA1,
    PARSER_DATA2,
    PARSER_SYSEX,
} parser_state_t;

// =============================================================================
// Private State
// =============================================================================

// Ring buffer for received bytes
static volatile uint8_t s_rx_buffer[MIDI_RX_BUFFER_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

// Parser state
static parser_state_t s_parser_state = PARSER_IDLE;
static uint8_t s_running_status = 0;
static midi_message_t s_current_msg;
static uint8_t s_expected_data_bytes = 0;

// Message queue (simple single-message buffer for polling)
static volatile bool s_message_ready = false;
static midi_message_t s_message_queue;

// Callbacks
static midi_message_callback_t s_message_callback = NULL;
static midi_byte_callback_t s_byte_callback = NULL;

// Statistics
static volatile uint32_t s_rx_count = 0;
static volatile uint32_t s_message_count = 0;
static volatile uint32_t s_error_count = 0;

static bool s_initialized = false;

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Get the number of data bytes expected for a status byte
 */
static uint8_t get_data_byte_count(uint8_t status) {
    switch (status & 0xF0) {
        case 0x80:  // Note Off
        case 0x90:  // Note On
        case 0xA0:  // Poly Pressure
        case 0xB0:  // Control Change
        case 0xE0:  // Pitch Bend
            return 2;
            
        case 0xC0:  // Program Change
        case 0xD0:  // Channel Pressure
            return 1;
            
        case 0xF0:  // System messages
            switch (status) {
                case 0xF0:  // SysEx Start
                    return 255;  // Variable length
                case 0xF1:  // MTC Quarter Frame
                case 0xF3:  // Song Select
                    return 1;
                case 0xF2:  // Song Position
                    return 2;
                default:    // Real-time and others
                    return 0;
            }
            
        default:
            return 0;
    }
}

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
 * @brief Check if a status byte is a real-time message
 */
static inline bool is_realtime_message(uint8_t byte) {
    return byte >= 0xF8;
}

/**
 * @brief Complete and dispatch the current message
 */
static void dispatch_message(void) {
    s_message_count++;
    
    // Handle Note On with velocity 0 as Note Off
    if (s_current_msg.type == MIDI_MSG_NOTE_ON && s_current_msg.data2 == 0) {
        s_current_msg.type = MIDI_MSG_NOTE_OFF;
        s_current_msg.raw[0] = 0x80 | s_current_msg.channel;
    }
    
    // Call message callback if registered
    if (s_message_callback != NULL) {
        s_message_callback(&s_current_msg);
    }
    
    // Also store in queue for polling
    if (!s_message_ready) {
        memcpy((void*)&s_message_queue, &s_current_msg, sizeof(midi_message_t));
        s_message_ready = true;
    }
}

/**
 * @brief Process a single MIDI byte through the parser
 */
static void parse_byte(uint8_t byte) {
    // Real-time messages can occur anywhere and don't affect running status
    if (is_realtime_message(byte)) {
        midi_message_t rt_msg = {
            .type = get_message_type(byte),
            .channel = 0,
            .data1 = 0,
            .data2 = 0,
            .raw = {byte, 0, 0},
            .length = 1
        };
        
        if (s_message_callback != NULL) {
            s_message_callback(&rt_msg);
        }
        s_message_count++;
        return;
    }
    
    // Status byte (bit 7 set)
    if (byte & 0x80) {
        // System common messages clear running status
        if ((byte & 0xF0) == 0xF0) {
            s_running_status = 0;
        } else {
            // Channel message - set running status
            s_running_status = byte;
        }
        
        s_expected_data_bytes = get_data_byte_count(byte);
        
        // Handle SysEx
        if (byte == 0xF0) {
            s_parser_state = PARSER_SYSEX;
            return;
        }
        
        // Handle SysEx End
        if (byte == 0xF7) {
            s_parser_state = PARSER_IDLE;
            return;
        }
        
        // Messages with no data bytes are complete immediately
        if (s_expected_data_bytes == 0) {
            s_current_msg.type = get_message_type(byte);
            s_current_msg.channel = byte & 0x0F;
            s_current_msg.data1 = 0;
            s_current_msg.data2 = 0;
            s_current_msg.raw[0] = byte;
            s_current_msg.length = 1;
            dispatch_message();
            s_parser_state = PARSER_IDLE;
            return;
        }
        
        // Start collecting data bytes
        s_current_msg.type = get_message_type(byte);
        s_current_msg.channel = byte & 0x0F;
        s_current_msg.raw[0] = byte;
        s_current_msg.length = 1;
        s_parser_state = PARSER_DATA1;
        return;
    }
    
    // Data byte (bit 7 clear)
    // Handle running status
    if (s_parser_state == PARSER_IDLE && s_running_status != 0) {
        // Re-use running status
        s_expected_data_bytes = get_data_byte_count(s_running_status);
        s_current_msg.type = get_message_type(s_running_status);
        s_current_msg.channel = s_running_status & 0x0F;
        s_current_msg.raw[0] = s_running_status;
        s_current_msg.length = 1;
        s_parser_state = PARSER_DATA1;
    }
    
    // Skip if we're not expecting data
    if (s_parser_state == PARSER_IDLE || s_parser_state == PARSER_SYSEX) {
        return;
    }
    
    // Collect data bytes
    if (s_parser_state == PARSER_DATA1) {
        s_current_msg.data1 = byte;
        s_current_msg.raw[1] = byte;
        s_current_msg.length = 2;
        
        if (s_expected_data_bytes == 1) {
            // Message complete
            s_current_msg.data2 = 0;
            dispatch_message();
            s_parser_state = PARSER_IDLE;
        } else {
            s_parser_state = PARSER_DATA2;
        }
        return;
    }
    
    if (s_parser_state == PARSER_DATA2) {
        s_current_msg.data2 = byte;
        s_current_msg.raw[2] = byte;
        s_current_msg.length = 3;
        dispatch_message();
        s_parser_state = PARSER_IDLE;
        return;
    }
}

// =============================================================================
// UART Interrupt Handler
// =============================================================================

static void on_uart_rx(void) {
    while (uart_is_readable(MIDI_UART_ID)) {
        uint8_t byte = uart_getc(MIDI_UART_ID);
        s_rx_count++;
        
        // Call raw byte callback if registered
        if (s_byte_callback != NULL) {
            s_byte_callback(byte);
        }
        
        // Add to ring buffer
        uint16_t next_head = (s_rx_head + 1) & (MIDI_RX_BUFFER_SIZE - 1);
        if (next_head != s_rx_tail) {
            s_rx_buffer[s_rx_head] = byte;
            s_rx_head = next_head;
        } else {
            // Buffer overrun
            s_error_count++;
        }
    }
}

// =============================================================================
// Public Functions
// =============================================================================

bool midi_uart_init(void) {
    if (s_initialized) {
        return true;
    }
    
    // Initialize UART
    uart_init(MIDI_UART_ID, MIDI_BAUD_RATE);
    
    // Set up pins
    gpio_set_function(PIN_MIDI_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_MIDI_RX, GPIO_FUNC_UART);
    
    // Set UART format: 8 data bits, 1 stop bit, no parity
    uart_set_format(MIDI_UART_ID, 8, 1, UART_PARITY_NONE);
    
    // Disable hardware flow control
    uart_set_hw_flow(MIDI_UART_ID, false, false);
    
    // Enable FIFO
    uart_set_fifo_enabled(MIDI_UART_ID, true);
    
    // Set up interrupt handler
    int uart_irq = (MIDI_UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, on_uart_rx);
    irq_set_enabled(uart_irq, true);
    
    // Enable RX interrupt
    uart_set_irq_enables(MIDI_UART_ID, true, false);
    
    // Reset state
    s_rx_head = 0;
    s_rx_tail = 0;
    s_parser_state = PARSER_IDLE;
    s_running_status = 0;
    s_message_ready = false;
    s_rx_count = 0;
    s_message_count = 0;
    s_error_count = 0;
    
    s_initialized = true;
    
    DEBUG_PRINT("MIDI UART: Initialized at %d baud\n", MIDI_BAUD_RATE);
    
    return true;
}

void midi_uart_deinit(void) {
    if (!s_initialized) {
        return;
    }
    
    // Disable interrupt
    int uart_irq = (MIDI_UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(uart_irq, false);
    
    // Deinitialize UART
    uart_deinit(MIDI_UART_ID);
    
    s_initialized = false;
    
    DEBUG_PRINT("MIDI UART: Deinitialized\n");
}

void midi_uart_set_message_callback(midi_message_callback_t callback) {
    s_message_callback = callback;
}

void midi_uart_set_byte_callback(midi_byte_callback_t callback) {
    s_byte_callback = callback;
}

bool midi_uart_message_available(void) {
    return s_message_ready;
}

bool midi_uart_get_message(midi_message_t *msg) {
    if (!s_message_ready || msg == NULL) {
        return false;
    }
    
    // Copy message and clear ready flag
    memcpy(msg, (void*)&s_message_queue, sizeof(midi_message_t));
    s_message_ready = false;
    
    return true;
}

void midi_uart_process(void) {
    // Process all bytes in the ring buffer
    while (s_rx_tail != s_rx_head) {
        uint8_t byte = s_rx_buffer[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) & (MIDI_RX_BUFFER_SIZE - 1);
        
        parse_byte(byte);
    }
}

uint32_t midi_uart_get_rx_count(void) {
    return s_rx_count;
}

uint32_t midi_uart_get_message_count(void) {
    return s_message_count;
}

uint32_t midi_uart_get_error_count(void) {
    return s_error_count;
}

void midi_uart_reset_stats(void) {
    s_rx_count = 0;
    s_message_count = 0;
    s_error_count = 0;
}
