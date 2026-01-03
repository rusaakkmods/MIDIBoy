/**
 * @file tusb_config.h
 * @brief TinyUSB configuration for MIDIBoy
 * 
 * Configures TinyUSB stack for MIDI device on RP2040.
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU    OPT_MCU_RP2040
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS     OPT_OS_NONE
#endif

// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG  0
#endif

// Enable Device stack
#define CFG_TUD_ENABLED 1

// Disable Host stack
#define CFG_TUH_ENABLED 0

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif

// Root hub port configuration
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)

//------------- CLASS -------------//
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            1   // MIDI enabled
#define CFG_TUD_VENDOR          0

// MIDI FIFO size of TX and RX
#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 64

#ifdef __cplusplus
}
#endif

#endif // TUSB_CONFIG_H
