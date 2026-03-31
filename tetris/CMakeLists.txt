cmake_minimum_required(VERSION 3.13)

# Set the board to pico2 (RP2350).
# If the Proton board has its own board definition file, replace "pico2" below.
set(PICO_BOARD pico2)

# Pull in the Pico SDK.  Copy pico_sdk_import.cmake from
# $PICO_SDK_PATH/external/pico_sdk_import.cmake into this directory.
include(pico_sdk_import.cmake)

project(tetris C CXX ASM)
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(tetris
    main.c
    game.c
    display.c
    input.c
    audio.c
    eeprom.c
)

target_link_libraries(tetris
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_adc
    hardware_pwm
    hardware_gpio
    hardware_clocks
)

# USB stdio for debug prints; disable UART stdio.
pico_enable_stdio_usb(tetris 1)
pico_enable_stdio_uart(tetris 0)

# Generate .uf2, .bin, .hex alongside the .elf
pico_add_extra_outputs(tetris)
