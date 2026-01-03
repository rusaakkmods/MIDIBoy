# MIDIBoy

**A USB-MIDI to Game Boy interface for the Raspberry Pi Pico**

MIDIBoy is a firmware for the Raspberry Pi Pico that bridges MIDI controllers and DAWs to the Nintendo Game Boy, enabling you to use the Game Boy's iconic sound chip as a synthesizer.

![License](https://img.shields.io/badge/license-GPL%20v2-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-green.svg)
![Status](https://img.shields.io/badge/status-Stage%201%20Complete-yellow.svg)

## Features

### Current (Stage 1 - mGB Mode)
- ✅ **USB-MIDI Device** - Enumerates as "rMODS MIDIBoy" on any MIDI host
- ✅ **DIN MIDI Input** - Standard 5-pin MIDI IN support (31250 baud)
- ✅ **Bidirectional Routing** - DIN ↔ USB ↔ Game Boy message forwarding
- ✅ **mGB Protocol Support** - Compatible with [trash80's mGB](https://github.com/trash80/mGB)
- ✅ **Real-time Performance** - Dual-core architecture for reliable timing
- ✅ **Visual Feedback** - LED activity indicator

### Planned Features
- 🔲 **LSDJ Sync Modes** - MIDI sync, keyboard, and Arduinoboy modes
- 🔲 **Game Boy MIDI OUT** - Receive MIDI from Game Boy (for mGB CC feedback)
- 🔲 **Configuration Menu** - On-device mode selection
- 🔲 **SysEx Configuration** - Remote configuration via MIDI

## Hardware

### Supported Microcontroller
- **Raspberry Pi Pico** (or compatible RP2040 boards)

### Pin Assignments

| Function | GPIO | Pico Pin | Description |
|----------|------|----------|-------------|
| GB_SI | GP2 | Pin 4 | Game Boy Serial In (data to GB) |
| GB_SC | GP3 | Pin 5 | Game Boy Serial Clock |
| GB_SO | GP4 | Pin 6 | Game Boy Serial Out (data from GB) |
| MIDI_TX | GP8 | Pin 11 | MIDI UART TX (optional MIDI OUT) |
| MIDI_RX | GP9 | Pin 12 | MIDI UART RX (DIN MIDI IN) |
| LED | GP25 | Onboard | Activity LED (built-in) |

### Wiring Diagram

**Raspberry Pi Pico Pinout:**

```
                        ┌──────────────┐
                        │     USB      │
                        └──────────────┘
               ┌────────────────────────────────┐
         GP0  ─┤ 1                           40 ├─ VBUS
         GP1  ─┤ 2                           39 ├─ VSYS
         GND  ─┤ 3                           38 ├─ GND
  GB_SI  GP2  ─┤ 4  ◄── To GB Link Pin 3     37 ├─ 3V3_EN
  GB_SC  GP3  ─┤ 5  ◄── To GB Link Pin 5     36 ├─ 3V3
  GB_SO  GP4  ─┤ 6  ◄── From GB Link Pin 2   35 ├─ ADC_VREF
         GP5  ─┤ 7                           34 ├─ GP28
         GND  ─┤ 8                           33 ├─ GND
         GP6  ─┤ 9                           32 ├─ GP27
         GP7  ─┤10                           31 ├─ GP26
MIDI TX  GP8  ─┤11  ◄── To MIDI OUT          30 ├─ RUN
MIDI RX  GP9  ─┤12  ◄── From MIDI IN         29 ├─ GP22
         GND  ─┤13                           28 ├─ GND
        GP10  ─┤14                           27 ├─ GP21
        GP11  ─┤15                           26 ├─ GP20
        GP12  ─┤16                           25 ├─ GP19
        GP13  ─┤17                           24 ├─ GP18
         GND  ─┤18                           23 ├─ GND
        GP14  ─┤19                           22 ├─ GP17
        GP15  ─┤20                           21 ├─ GP16
               └────────────────────────────────┘
                                               
                     Onboard LED = GP25
```

**Game Boy Link Cable Pinout (looking at cable plug):**

```
    ┌─────────┐
    │ 6  4  2 │
    │  5  3  1│
    └─────────┘
    
    Pin 1: VCC (5V) - Not connected
    Pin 2: SO (Serial Out from GB) → Pico GP4 (Pin 6)
    Pin 3: SI (Serial In to GB) ← Pico GP2 (Pin 4)
    Pin 4: SD (Serial Data) - Not used
    Pin 5: SC (Serial Clock) ← Pico GP3 (Pin 5)
    Pin 6: GND → Pico GND (Pin 3 or 8)
```

**Connection Summary:**
| Pico GPIO | Pico Pin | GB Link Pin | Function |
|-----------|----------|-------------|----------|
| GP2 | Pin 4 | Pin 3 (SI) | Data to Game Boy |
| GP3 | Pin 5 | Pin 5 (SC) | Serial Clock |
| GP4 | Pin 6 | Pin 2 (SO) | Data from Game Boy |
| GND | Pin 3/8 | Pin 6 (GND) | Ground |
| GP8 | Pin 11 | - | MIDI OUT (optional) |
| GP9 | Pin 12 | - | MIDI IN |
| GP25 | Onboard | - | Activity LED (built-in) |

## Building

### Prerequisites

1. **Pico SDK 2.2.0+** - Install via [Raspberry Pi Pico VS Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)
2. **CMake 3.13+**
3. **Ninja** build system
4. **ARM GCC Toolchain** (arm-none-eabi-gcc)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/MIDIBoy.git
cd MIDIBoy

# Create build directory and configure
mkdir build && cd build
cmake ..

# Build
ninja
```

Or use VS Code with the Pico extension:
1. Open the project folder
2. Press `Ctrl+Shift+B` and select "Compile Project"

### Output Files
- `build/MIDIBoy.uf2` - Drag-and-drop firmware for BOOTSEL mode
- `build/MIDIBoy.elf` - For debugging with OpenOCD/SWD

## Installation

### Method 1: UF2 (Recommended)
1. Hold the **BOOTSEL** button on your RP2040 Zero
2. Connect to USB while holding BOOTSEL
3. Release BOOTSEL - the device mounts as a USB drive
4. Drag `MIDIBoy.uf2` to the drive
5. Device will reboot automatically

### Method 2: Picotool
```bash
picotool load build/MIDIBoy.uf2 -fx
```

### Method 3: SWD Debug Probe
Use the "Flash" task in VS Code with a Picoprobe or other CMSIS-DAP debugger.

## Usage

### With mGB
1. Flash [mGB](https://github.com/trash80/mGB) to a Game Boy cartridge
2. Connect MIDIBoy to the Game Boy link port
3. Connect MIDIBoy to your computer via USB
4. Power on the Game Boy and start mGB
5. MIDIBoy appears as "rMODS MIDIBoy" in your DAW/MIDI software

### MIDI Channel Mapping (mGB Mode)

| MIDI Channel | mGB Instrument | Sound Type |
|--------------|----------------|------------|
| 1 | PU1 | Pulse Wave 1 |
| 2 | PU2 | Pulse Wave 2 |
| 3 | WAV | Wavetable |
| 4 | NOI | Noise |
| 5 | POLY | Polyphonic (all channels) |

### LED Indicators

| Pattern | Meaning |
|---------|---------|
| Solid ON | USB connected, waiting for enumeration |
| 2 slow blinks | Startup complete |
| Brief flash | MIDI activity |
| Fast blinking | Error condition |

## Architecture

MIDIBoy uses a dual-core architecture for reliable real-time performance:

```
┌─────────────────────────────────────────────────────────────┐
│                         RP2040                              │
│  ┌─────────────────────┐    ┌─────────────────────────────┐ │
│  │      Core 0         │    │          Core 1             │ │
│  │   (Real-time)       │    │      (Housekeeping)         │ │
│  │                     │    │                             │ │
│  │  ┌───────────────┐  │    │  ┌───────────────────────┐  │ │
│  │  │  MIDI Parser  │  │    │  │    TinyUSB Stack      │  │ │
│  │  │  (UART IRQ)   │  │    │  │    (tud_task)         │  │ │
│  │  └───────┬───────┘  │    │  └───────────────────────┘  │ │
│  │          │          │    │                             │ │
│  │  ┌───────▼───────┐  │    │  ┌───────────────────────┐  │ │
│  │  │  mGB Router   │  │    │  │   LED Update Loop     │  │ │
│  │  │ (CH mapping)  │  │    │  │   (blink patterns)    │  │ │
│  │  └───────┬───────┘  │    │  └───────────────────────┘  │ │
│  │          │          │    │                             │ │
│  │  ┌───────▼───────┐  │    └─────────────────────────────┘ │
│  │  │   GB Link     │  │                                    │
│  │  │  (PIO TX)     │  │                                    │
│  │  └───────────────┘  │                                    │
│  └─────────────────────┘                                    │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

| Module | File | Purpose |
|--------|------|---------|
| GB Link | `gb_link.c` | PIO-based Game Boy serial transmission |
| MIDI UART | `midi_uart.c` | Interrupt-driven MIDI parser with running status |
| USB MIDI | `usb_midi.c` | TinyUSB MIDI device wrapper |
| Mode mGB | `mode_mgb.c` | mGB protocol handler and message router |
| LED | `led.c` | Activity indicator with blink patterns |

## Technical Details

### Game Boy Link Protocol
- **Clock Speed**: ~8 kHz (externally clocked by MIDIBoy)
- **Data Format**: 8-bit, MSB first
- **Inter-byte Delay**: 500µs minimum for mGB compatibility

### MIDI Implementation
- **DIN MIDI**: 31250 baud, 8N1
- **USB MIDI**: USB 2.0 Full Speed, MIDI 1.0 class compliant
- **Supported Messages**: Note On/Off, CC, Program Change, Pitch Bend, Aftertouch

### Memory Usage
- Flash: ~64 KB (of 2 MB)
- RAM: ~16 KB (of 264 KB)

## Troubleshooting

### Device not recognized as MIDI
- Check USB cable (must be data-capable, not charge-only)
- Try a different USB port
- Re-flash the firmware

### No sound from Game Boy
- Verify mGB is running on the Game Boy
- Check link cable connections (SI, SC, GND)
- Ensure you're sending on MIDI channels 1-5

### LED not blinking on MIDI input
- Verify MIDI connection (DIN or USB)
- Check MIDI channel matches (1-5 for mGB)
- Use a MIDI monitor to verify messages are being sent

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [trash80](https://github.com/trash80) - For mGB and the original Arduinoboy
- [Raspberry Pi Foundation](https://www.raspberrypi.org/) - For the RP2040 and Pico SDK
- [TinyUSB](https://github.com/hathach/tinyusb) - USB stack
- The chiptune community for keeping Game Boy music alive!

## Links

- [mGB by trash80](https://github.com/trash80/mGB)
- [LSDJ](https://www.littlesounddj.com/)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [Pico SDK Documentation](https://raspberrypi.github.io/pico-sdk-doxygen/)

---

*Made with ♪ for the Game Boy music community*
