# RP2350 Flappy Bird (ECE 362 Final Project)

Embedded Flappy Bird implementation for an RP2350-based Proton board using Pico SDK style C.

## Features

- `SPI` TFT graphics on MSP2202 module (`ILI9341` controller only)
- `ADC` joystick X/Y sampling with normalization and deadzone
- `GPIO` joystick select button with debounce + edge detect
- `PWM` non-blocking audio tones (jump / score / collision)
- `I2C` high-score persistence on `24AA32AF` EEPROM
- Fixed timestep game loop with states: `MENU`, `PLAYING`, `GAME_OVER`

## Project Structure

- `main.c` - system init, fixed timestep loop, state-driven render/audio/storage
- `game.c` / `game.h` - Flappy Bird physics, pipes, scoring, collisions, game state
- `display.c` / `display.h` - ILI9341 SPI driver + drawing/render helpers
- `input.c` / `input.h` - joystick ADC + button debounce/edge logic
- `audio.c` / `audio.h` - PWM tone generation (non-blocking during gameplay)
- `eeprom.c` / `eeprom.h` - 24AA32AF page-aware read/write helpers
- `pins.h` - complete peripheral pin map
- `CMakeLists.txt` - Pico SDK target configuration

## Pin Map (Current Configuration)

Assumption: Proton board supports RP2-style alternate functions on these GPIOs.
Verify with your exact board schematic before wiring.

- **TFT SPI (ILI9341)**
  - `SCK` -> GPIO18
  - `MOSI` -> GPIO19
  - `CS` -> GPIO17
  - `DC` -> GPIO20
  - `RST` -> GPIO21
  - `BL` -> GPIO22
- **Joystick (Adafruit analog 2-axis + select)**
  - `X` -> GPIO26 (`ADC0`)
  - `Y` -> GPIO27 (`ADC1`)
  - `SEL` -> GPIO15 (active-low digital input)
- **Audio (PWM speaker/buzzer)**
  - `AUDIO_PWM` -> GPIO10
- **EEPROM (24AA32AF, I2C)**
  - `SDA` -> GPIO6 (`I2C1`)
  - `SCL` -> GPIO7 (`I2C1`)
  - I2C address assumed `0x50` (`A2/A1/A0` strapped low)

## Hardware Notes

- The MSP2202 module includes an SD slot, but this project **uses TFT only**.
- Display is configured for ILI9341 landscape mode (`320x240` logical area).
- EEPROM driver supports:
  - total size `4096 bytes` (24AA32AF / 32 Kbit)
  - page size `32 bytes`
  - page-split writes
  - write-cycle completion wait

## Build

1. Ensure Pico SDK is installed and environment is set (e.g. `PICO_SDK_PATH`).
2. From project root:
   - `mkdir build`
   - `cd build`
   - `cmake ..`
   - `cmake --build .`
3. Flash generated UF2 to your board (or use your standard debug/flash flow).

## Hardware Bring-Up Tests

Test helpers are included:

- `display_test()` in `display.c`
- `input_test()` in `input.c` (continuous UART print loop)
- `eeprom_test()` in `eeprom.c`
- `audio_test()` in `audio.c`

`main.c` includes:

- `#define RUN_HW_TESTS 0`

Set to `1` to run display/audio/EEPROM test calls at startup.
`input_test()` is intentionally left commented there because it is continuous and blocks.

## Gameplay Behavior

- Press joystick select button to start, jump, and return from game-over to menu.
- Gravity + jump velocity model controls the bird.
- Pipes scroll left; score increments when bird passes a pipe.
- Collision with pipes or floor/ceiling ends run.
- High score is read on startup and only written when exceeded.

## ECE 362 Demo Checklist

- Verify TFT draws color bars and score text (`display_test`).
- Verify joystick ADC and button transitions over UART (`input_test`).
- Verify EEPROM write/read match (`eeprom_test`).
- Verify audible PWM tones (`audio_test`).
- Run full game and confirm high score persists across reset.
