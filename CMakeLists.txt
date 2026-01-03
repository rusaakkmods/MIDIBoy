# MIDIBoy Firmware - RP2040 Zero Game Boy MIDI Interface
# Stage 1: mGB MIDI IN Mode

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(MIDIBoy C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------
set(SOURCES
    src/main.c
    src/gb_link.c
    src/midi_uart.c
    src/usb_midi.c
    src/usb_descriptors.c
    src/mode_mgb.c
    src/led.c
)

# PIO files
set(PIO_SOURCES
    src/gb_link_tx.pio
)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

# Generate PIO headers
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gb_link_tx.pio)

pico_set_program_name(${PROJECT_NAME} "MIDIBoy")
pico_set_program_version(${PROJECT_NAME} "0.1.0")

# Disable stdio on UART and USB (we use USB for MIDI)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# -----------------------------------------------------------------------------
# Libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_uart
    hardware_irq
    hardware_dma
    tinyusb_device
    tinyusb_board
)

# -----------------------------------------------------------------------------
# Include Directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# -----------------------------------------------------------------------------
# Compiler Options
# -----------------------------------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Generate additional outputs (UF2, HEX, BIN)
pico_add_extra_outputs(${PROJECT_NAME})

