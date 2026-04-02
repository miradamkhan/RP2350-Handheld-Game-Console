# 🎮 RP2350 Flappy Bird  
### ECE 362 – Microprocessor Systems and Interfacing Final Project

Embedded Flappy Bird implementation for an RP2350-based Proton board using Pico SDK style C. This project integrates real-time graphics, analog input, audio output, and persistent storage into a handheld gaming system.

---

## 🚀 Features

- 🖥️ **SPI TFT Graphics** (ILI9341 display)
- 🎮 **Analog Joystick Input** (ADC + GPIO button)
- 🔊 **PWM Audio Output** (non-blocking sound effects)
- 💾 **EEPROM High Score Storage** (I2C)
- ⏱️ **Fixed Timestep Game Loop**
- 🧠 Game states:
  - MENU
  - PLAYING
  - GAME OVER

---

## 📁 Project Structure
main.c # system init, game loop, state handling
game.c / game.h # Flappy Bird physics, pipes, scoring, collisions
display.c / display.h# ILI9341 SPI driver + rendering helpers
input.c / input.h # joystick ADC + button logic
audio.c / audio.h # PWM tone generation (non-blocking)
eeprom.c / eeprom.h # 24AA32AF I2C read/write logic
pins.h # complete GPIO mapping
CMakeLists.txt # Pico SDK build configuration


---

## 🔌 Pin Map (Final Configuration)

### 🟦 SPI Display (ILI9341 TFT)
- SCK → GPIO14  
- MOSI → GPIO15  
- CS → GPIO13  
- DC → GPIO12  
- RST → GPIO11  

---

### 🟩 I2C EEPROM (24AA32AF)
- SDA → GPIO4  
- SCL → GPIO5  
- Address: `0x50` (A0–A2 tied low)

---

### 🟨 Joystick (Analog + Button)
- X → GPIO45 (ADC)  
- Y → GPIO44 (ADC)  
- BTN → GPIO6 (active LOW)

---

### 🟧 Audio (PWM Speaker)
- PWM → GPIO30  

---

### 🟥 Debug
- LED → GPIO17  

---

### 🟪 Free GPIO (Available)
- GPIO7, GPIO8, GPIO9, GPIO10, GPIO18, GPIO19  

---

## ⚙️ Hardware Notes

- TFT module: **MSP2202 (ILI9341)**  
- Display mode: **320x240 landscape**
- EEPROM:
  - Size: 4KB (32 Kbit)
  - Page size: 32 bytes
  - Page-safe write handling implemented

---

## 🛠 Build Instructions

Ensure Pico SDK is installed and environment variable is set:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
mkdir build
cd build
cmake ..
cmake --build .

🧪 Hardware Bring-Up Tests

Available test functions:

display_test() → verifies TFT rendering
input_test() → prints joystick ADC + button values (UART)
audio_test() → plays PWM tones
eeprom_test() → verifies EEPROM read/write

Enable in main.c:
#define RUN_HW_TESTS 1
🎮 Gameplay
Press joystick button to:
Start game
Jump
Restart after game over
Mechanics:
Gravity + jump velocity physics
Pipes scroll horizontally
Score increases when passing pipes
Collision ends the game
High Score:
Loaded at startup
Saved only when exceeded
Stored in EEPROM (persists across resets)
✅ ECE 362 Demo Checklist
✔ SPI → Display renders graphics and UI (display_test)
✔ ADC + GPIO → Joystick input works (input_test)
✔ PWM → Audio output functional (audio_test)
✔ I2C → EEPROM read/write verified (eeprom_test)
✔ Full system demo:
Game runs smoothly
Input is responsive
Audio works
High score persists
🧠 Design Highlights
Modular embedded C architecture
Non-blocking gameplay loop
Separation of hardware drivers and game logic
Centralized pin configuration via pins.h
🚀 Future Improvements
Additional games (e.g., Tetris)
Improved graphics and animations
Battery-powered handheld version
Custom PCB enclosure refinement
👥 Team Members
Enio Hysa
Mir Adam Khan
Abdul Malik
Dixon Wagner
