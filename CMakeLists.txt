cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rp2350_game_console C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(rp2350_game_console
    main.c
    display.c
    input.c
    audio.c
    eeprom.c
    tetris_game.c
    tetris_render.c
    flappy_game.c
    flappy_render.c
)

target_include_directories(rp2350_game_console PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(rp2350_game_console
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_adc
    hardware_pwm
    hardware_i2c
    hardware_clocks
)

pico_enable_stdio_usb(rp2350_game_console 1)
pico_enable_stdio_uart(rp2350_game_console 0)
pico_add_extra_outputs(rp2350_game_console)
