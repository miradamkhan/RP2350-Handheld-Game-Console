/*
 * display.h — ILI9341 TFT driver (SPI) public interface.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

/* Colour helpers — RGB565 big-endian as ILI9341 expects */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFD20
#define COLOR_GRAY      0x8410
#define COLOR_DARK_GRAY 0x4208

/* Convert 8-bit R, G, B to RGB565 */
#define RGB565(r, g, b) \
    ((uint16_t)(((r) & 0xF8) << 8 | ((g) & 0xFC) << 3 | ((b) >> 3)))

/* Initialise SPI and ILI9341; must be called once at boot. */
void display_init(void);

/* Set the drawing window (column/row address). */
void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/* Draw a single pixel. */
void display_pixel(uint16_t x, uint16_t y, uint16_t color);

/* Fill the entire screen with one colour. */
void display_fill(uint16_t color);

/* Draw a filled rectangle. */
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);

/* Draw a single character (5×7 font, scaled). Returns advance width. */
uint16_t display_char(uint16_t x, uint16_t y, char c, uint16_t color,
                      uint16_t bg, uint8_t scale);

/* Draw a null-terminated string. Returns final X position. */
uint16_t display_string(uint16_t x, uint16_t y, const char *s,
                        uint16_t color, uint16_t bg, uint8_t scale);

/* Hardware test: fill screen red → green → blue, then white. */
void display_test(void);

#endif /* DISPLAY_H */
