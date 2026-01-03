/**
 * @file usb_descriptors.c
 * @brief USB device and MIDI descriptors for MIDIBoy
 * 
 * Configures TinyUSB to present as a MIDI device with the name "rMODS MIDIBoy"
 */

#include "tusb.h"
#include "pico/unique_id.h"

// =============================================================================
// Device Descriptor
// =============================================================================

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,           // USB 2.0
    .bDeviceClass       = 0x00,             // Defined in Interface Descriptor
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    
    .idVendor           = 0x2E8A,           // Raspberry Pi
    .idProduct          = 0x000A,           // Raspberry Pi Pico SDK MIDI
    .bcdDevice          = 0x0100,           // Device version 1.0
    
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    
    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// =============================================================================
// Configuration Descriptor
// =============================================================================

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

#define EPNUM_MIDI_OUT      0x01
#define EPNUM_MIDI_IN       0x81

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(0, 0, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64)
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index; // Only one configuration
    return desc_configuration;
}

// =============================================================================
// String Descriptors
// =============================================================================

// Array of pointer to string descriptors
char const *string_desc_arr[] = {
    (const char[]){ 0x09, 0x04 },   // 0: Language (English)
    "rMODS",                         // 1: Manufacturer
    "rMODS MIDIBoy",                // 2: Product
    NULL,                            // 3: Serial (generated from flash ID)
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // Convert ASCII string into UTF-16
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }

        const char *str = string_desc_arr[index];

        // Use board unique ID for serial number
        if (index == 3) {
            // Generate serial from RP2040 flash unique ID
            extern void pico_get_unique_board_id_string(char *id_out, uint len);
            static char serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
            pico_get_unique_board_id_string(serial_str, sizeof(serial_str));
            str = serial_str;
        }

        if (!str) return NULL;

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        // Convert ASCII to UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // First byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
