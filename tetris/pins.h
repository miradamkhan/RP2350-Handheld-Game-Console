/*
 * pins.h — Single source of truth for all GPIO/peripheral pin assignments.
 *
 * Target : RP2350 Proton board
 * Display: MSP2202 2.2" TFT (ILI9341) via SPI
 * EEPROM : 24AA32AF via I2C
 * Input  : Adafruit analog joystick #512 (2× ADC + 1 GPIO)
 * Audio  : PWM speaker output
 *
 * Pin choices avoid conflicts between SPI0, I2C0, ADC, and PWM.
 * All pins are validated against the RP2350 pin-mux table.
 */

#ifndef PINS_H
#define PINS_H

/* ── SPI1: ILI9341 TFT Display ── */
#define PIN_SPI_SCK     14  /* GP14 — SPI1 SCK */
#define PIN_SPI_MOSI    15  /* GP15 — SPI1 TX  */
#define PIN_TFT_CS      13  /* GP13 — manual CS */
#define PIN_TFT_DC      12  /* GP12 — data/cmd  */
#define PIN_TFT_RST     11  /* GP11 — reset     */

#define SPI_PORT        spi1
#define SPI_BAUD_HZ     40000000  /* 40 MHz */

/* ── I2C0: 24AA32AF EEPROM ── */
#define PIN_I2C_SDA     4   /* GP4 — I2C0 SDA */
#define PIN_I2C_SCL     5   /* GP5 — I2C0 SCL */

#define I2C_PORT        i2c0
#define I2C_BAUD_HZ     400000    /* 400 kHz fast-mode */

/* ── ADC: Adafruit Joystick #512 ── */
#define PIN_JOY_X       45  /* GP45 — ADC5 */
#define PIN_JOY_Y       44  /* GP44 — ADC4 */
#define ADC_CHAN_X       5
#define ADC_CHAN_Y       4

/* ── GPIO: Joystick select button (active-low, internal pull-up) ── */
#define PIN_JOY_BTN     6   /* GP6 */

/* ── PWM: Speaker ── */
#define PIN_SPEAKER     30  /* GP30 */

/* ── Debug ── */
#define PIN_DEBUG_LED   17  /* GP17 */

/* ── Display geometry (ILI9341) ── */
#define TFT_WIDTH       240
#define TFT_HEIGHT      320

#endif /* PINS_H */
