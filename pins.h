#ifndef PINS_H
#define PINS_H

// RP2350 / Proton pin map used by this project.
// Assumption: standard RP2-style alternate functions are available on Proton.
// Verify against your exact board schematic before final wiring.

// SPI0 pins for MSP2202 (ILI9341) TFT
#define TFT_SPI_PORT        spi0
#define TFT_PIN_SCK         18
#define TFT_PIN_MOSI        19
#define TFT_PIN_CS          17
#define TFT_PIN_DC          20
#define TFT_PIN_RST         21
#define TFT_PIN_BL          22

// ADC pins for Adafruit analog joystick (X/Y)
#define JOY_ADC_X_PIN       26
#define JOY_ADC_Y_PIN       27
#define JOY_ADC_X_CH        0
#define JOY_ADC_Y_CH        1

// Joystick select button (active low)
#define JOY_BTN_PIN         15

// PWM output for speaker/buzzer
#define AUDIO_PWM_PIN       10

// I2C1 pins for 24AA32AF EEPROM
#define EEPROM_I2C_PORT     i2c1
#define EEPROM_PIN_SDA      6
#define EEPROM_PIN_SCL      7
// Assumption: A2/A1/A0 strapped low on lab kit board
#define EEPROM_I2C_ADDR     0x50

#endif // PINS_H
