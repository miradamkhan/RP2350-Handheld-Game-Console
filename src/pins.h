/*
 * pins.h — Single source of truth for all GPIO/peripheral pin assignments.
 *
 * Target : RP2350 Proton board
 *
 * Display (only): MSP2202 — ILI9341, 4-wire SPI, 240×320 (see LCD wiki SKU).
 * https://www.lcdwiki.com/2.2inch_SPI_Module_ILI9341_SKU:MSP2202
 *
 * Per MSP2202 user manual (SPI section): D/C (RS) low = command, high = data;
 * SPI0-style timing CPOL=0, CPHA=0. (Do not invert D/C unless hardware inverts it.)
 *
 * EEPROM : 24AA32AF via I2C
 * Input  : Adafruit analog joystick #512 (2x ADC + 1 GPIO)
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
/* Set to the GPIO wired to TFT backlight (LED/BL/BLK). Use -1 if tied to 3V3. */
#define PIN_TFT_BL      -1

#define SPI_PORT        spi1
/* If the panel glitches, try 4_000_000 or 1_000_000. */
#define SPI_BAUD_HZ     4000000
/* Bring-up fallback: 1 = software SPI bit-bang on SCK/MOSI pins. */
#define TFT_USE_SOFT_SPI 0

/*
 * SPI mode: MSP2202 manual — SPI0, CPOL=0, CPHA=0 (RP2040/2350 “mode 0”).
 */
#ifndef TFT_SPI_MODE
#define TFT_SPI_MODE    0
#endif

/*
 * D/C invert: 0 = MSP2202 manual / ILI9341 (low=command, high=data).
 * Set to 1 only if your PCB inverts the D/C line to the panel.
 */
#ifndef TFT_DC_INVERT
#define TFT_DC_INVERT   0
#endif

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
