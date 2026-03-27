# RP2350-Handheld-Game-Console
ECE 362 Final Project - Microprocessors
Overview

This project implements a handheld game console using the RP2350 microcontroller on the Proton development board. The system runs Tetris and Flappy Bird, rendered on a TFT LCD display and controlled using an analog joystick.

The project integrates multiple hardware peripherals to handle graphics, input, audio, and persistent storage in a real-time embedded system.

Features
🎮 Two Games
Tetris (grid-based logic, rotation, line clearing)
Flappy Bird (physics-based movement and collision)
🖥️ TFT LCD Display (SPI)
Real-time graphics rendering
Game UI and score display
🎯 Analog Joystick Input (ADC + GPIO)
X-Y movement using ADC
Push button for actions and menu selection
🔊 Audio Output (PWM + Speaker)
Sound effects for gameplay events
💾 Persistent High Scores (I2C EEPROM)
Stores high scores across power cycles
Hardware Components
RP2350 Proton Board
TFT LCD Display (SPI)
Analog Joystick Module (X-Y + push button)
Speaker
External EEPROM (I2C)
Breadboard and supporting components
System Architecture

The system is built around a main game loop that continuously:

Reads joystick input (ADC + GPIO)
Updates game state
Renders graphics to the display (SPI)
Outputs sound when needed (PWM)
Handles score storage (I2C EEPROM)
