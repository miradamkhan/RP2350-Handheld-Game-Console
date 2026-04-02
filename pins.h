#ifndef PINS_H
#define PINS_H

// RP2350 / Proton pin map used by this project.
// Assumption: standard RP2-style alternate functions are available on Proton.
// Verify against your exact board schematic before final wiring.

// SPI1 pins for MSP2202 (ILI9341) TFT
// (GPIO14/15 map to SPI1 on RP2-style muxing)
#define TFT_SPI_PORT        spi1
#define TFT_PIN_SCK         14
#define TFT_PIN_MOSI        15
#define TFT_PIN_CS          13
#define TFT_PIN_DC          12
#define TFT_PIN_RST         11

// ADC pins for Adafruit analog joystick (X/Y)
#define JOY_ADC_X_PIN       45
#define JOY_ADC_Y_PIN       44
#define JOY_ADC_X_CH        5
#define JOY_ADC_Y_CH        4

// Joystick select button (active low)
#define JOY_BTN_PIN         6

// PWM output for speaker/buzzer
#define AUDIO_PWM_PIN       30

// Debug LED (active-high)
#define DEBUG_LED_PIN       17

// I2C0 pins for 24AA32AF EEPROM
#define EEPROM_I2C_PORT     i2c0
#define EEPROM_PIN_SDA      4
#define EEPROM_PIN_SCL      5
// Assumption: A2/A1/A0 strapped low on lab kit board
#define EEPROM_I2C_ADDR     0x50

#endif // PINS_H
