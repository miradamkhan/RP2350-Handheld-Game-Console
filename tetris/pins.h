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

/* ── SPI0: ILI9341 TFT Display ── */
#define PIN_SPI_SCK     2   /* GP2  — SPI0 SCK  */
#define PIN_SPI_MOSI    3   /* GP3  — SPI0 TX   */
#define PIN_TFT_CS      5   /* GP5  — manual CS  */
#define PIN_TFT_DC      6   /* GP6  — data/cmd   */
#define PIN_TFT_RST     7   /* GP7  — reset      */

#define SPI_PORT        spi0
#define SPI_BAUD_HZ     40000000  /* 40 MHz — ILI9341 max is ~62 MHz */

/* ── I2C0: 24AA32AF EEPROM ── */
#define PIN_I2C_SDA     16  /* GP16 — I2C0 SDA */
#define PIN_I2C_SCL     17  /* GP17 — I2C0 SCL */

#define I2C_PORT        i2c0
#define I2C_BAUD_HZ     400000    /* 400 kHz fast-mode */

/* ── ADC: Adafruit Joystick #512 ── */
#define PIN_JOY_X       26  /* GP26 — ADC0 */
#define PIN_JOY_Y       27  /* GP27 — ADC1 */
#define ADC_CHAN_X       0
#define ADC_CHAN_Y       1

/* ── GPIO: Joystick select button (active-low, internal pull-up) ── */
#define PIN_JOY_BTN     22  /* GP22 */

/* ── PWM: Speaker ── */
#define PIN_SPEAKER     15  /* GP15 — PWM7 B */

/* ── Display geometry (ILI9341) ── */
#define TFT_WIDTH       240
#define TFT_HEIGHT      320

#endif /* PINS_H */
