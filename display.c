/*
 * display.c — MSP2202 (ILI9341) TFT over SPI for the RP2350 Proton board.
 *
 * Target panel: LCD wiki SKU MSP2202 only — same electrical/SPI rules as vendor manual
 * (D/C low = command, high = data; SPI mode 0).
 * https://www.lcdwiki.com/2.2inch_SPI_Module_ILI9341_SKU:MSP2202
 *
 * Init, window set, fill, and 5x7 font for menu/games.
 */

#include "display.h"
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include <string.h>

/* ──────────────────────── low-level helpers ──────────────────────── */

static inline void cs_select(void)   { gpio_put(PIN_TFT_CS, 0); }
static inline void cs_deselect(void) { gpio_put(PIN_TFT_CS, 1); }

/* MSP2202 / ILI9341: D/C low = command, high = data. TFT_DC_INVERT flips if PCB inverts. */
static inline void dc_command(void)
{
#if TFT_DC_INVERT
    gpio_put(PIN_TFT_DC, 1);
#else
    gpio_put(PIN_TFT_DC, 0);
#endif
}

static inline void dc_data(void)
{
#if TFT_DC_INVERT
    gpio_put(PIN_TFT_DC, 0);
#else
    gpio_put(PIN_TFT_DC, 1);
#endif
}

static void send_command_params(uint8_t cmd, const uint8_t *params, uint16_t len);

static void spi_write_byte(uint8_t b)
{
    spi_write_blocking(SPI_PORT, &b, 1);
}

static void send_command(uint8_t cmd)
{
    send_command_params(cmd, NULL, 0);
}

static void send_command_params(uint8_t cmd, const uint8_t *params, uint16_t len)
{
    /*
     * Keep CS asserted across command + params.
     * Some TFT controllers ignore params if CS toggles between bytes.
     */
    cs_select();
    dc_command();
    spi_write_byte(cmd);
    if (len > 0 && params != NULL) {
        dc_data();
        spi_write_blocking(SPI_PORT, params, len);
    }
    cs_deselect();
}

static void apply_spi_format(void)
{
#if TFT_SPI_MODE == 3
    spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
#else
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
#endif
}

/* ──────────────────────── ILI9341 init sequence ──────────────────── */

void display_init(void)
{
    /* Match lab order: CS/DC/RST as GPIO, then mux SCK/MOSI to SPI, then spi_init. */
    gpio_init(PIN_TFT_CS);
    gpio_set_dir(PIN_TFT_CS, GPIO_OUT);
    gpio_put(PIN_TFT_CS, 1);

    gpio_init(PIN_TFT_DC);
    gpio_set_dir(PIN_TFT_DC, GPIO_OUT);
    dc_command();

    gpio_init(PIN_TFT_RST);
    gpio_set_dir(PIN_TFT_RST, GPIO_OUT);

    gpio_set_function(PIN_SPI_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);

    spi_init(SPI_PORT, SPI_BAUD_HZ);
    apply_spi_format();

#if PIN_TFT_BL >= 0
    /* Some TFT modules require explicit backlight enable. */
    gpio_init(PIN_TFT_BL);
    gpio_set_dir(PIN_TFT_BL, GPIO_OUT);
    gpio_put(PIN_TFT_BL, 1);
#endif

    /* Hardware reset. */
    gpio_put(PIN_TFT_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_TFT_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_TFT_RST, 1);
    sleep_ms(120);

    send_command(0x01);  /* SWRESET */
    sleep_ms(120);

    /*
     * MSP2202 init sequence matched to known-working LCDWIKI sample code.
     * Keep register order/values aligned with reference.
     */
    send_command_params(0xCF, (uint8_t[]){0x00, 0xD9, 0x30}, 3);
    send_command_params(0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);
    send_command_params(0xE8, (uint8_t[]){0x85, 0x10, 0x7A}, 3);
    send_command_params(0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);
    send_command_params(0xF7, (uint8_t[]){0x20}, 1);
    send_command_params(0xEA, (uint8_t[]){0x00, 0x00}, 2);
    send_command_params(0xC0, (uint8_t[]){0x21}, 1);
    send_command_params(0xC1, (uint8_t[]){0x12}, 1);
    send_command_params(0xC5, (uint8_t[]){0x39, 0x37}, 2);
    send_command_params(0xC7, (uint8_t[]){0xAB}, 1);
    send_command_params(0x36, (uint8_t[]){0x48}, 1);
    send_command_params(0x3A, (uint8_t[]){0x55}, 1);
    send_command_params(0xB1, (uint8_t[]){0x00, 0x1B}, 2);
    send_command_params(0xB6, (uint8_t[]){0x0A, 0xA2}, 2);
    send_command_params(0xF2, (uint8_t[]){0x00}, 1);
    send_command_params(0x26, (uint8_t[]){0x01}, 1);
    send_command_params(0xE0, (uint8_t[]){
        0x0F, 0x23, 0x1F, 0x0B, 0x0E, 0x08, 0x4B, 0xA8,
        0x3B, 0x0A, 0x14, 0x06, 0x10, 0x09, 0x00
    }, 15);
    send_command_params(0xE1, (uint8_t[]){
        0x00, 0x1C, 0x20, 0x04, 0x10, 0x08, 0x34, 0x47,
        0x44, 0x05, 0x0B, 0x09, 0x2F, 0x36, 0x0F
    }, 15);

    /* Full frame window as in sample init (240x320). */
    send_command_params(0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0x3F}, 4);
    send_command_params(0x2A, (uint8_t[]){0x00, 0x00, 0x00, 0xEF}, 4);

    send_command(0x11);  /* SLPOUT */
    sleep_ms(120);
    send_command(0x29);  /* DISPON */
    sleep_ms(20);
    /* Keep MADCTL from init (0x48). Do not overwrite with 0x08 here — that clears
     * MX and can mis-address GRAM on some builds; RP2350 SPI16 + 0x08 also hid bugs. */
}

/* ──────────────────────── drawing primitives ─────────────────────── */

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* Column Address Set */
    {
        uint8_t p[] = {
            (uint8_t)(x0 >> 8), (uint8_t)(x0),
            (uint8_t)(x1 >> 8), (uint8_t)(x1)
        };
        send_command_params(0x2A, p, sizeof(p));
    }
    /* Row Address Set */
    {
        uint8_t p[] = {
            (uint8_t)(y0 >> 8), (uint8_t)(y0),
            (uint8_t)(y1 >> 8), (uint8_t)(y1)
        };
        send_command_params(0x2B, p, sizeof(p));
    }
    /* Memory Write */
    send_command(0x2C);
}

static void begin_memory_write(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t p_col[] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0),
        (uint8_t)(x1 >> 8), (uint8_t)(x1)
    };
    uint8_t p_row[] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0),
        (uint8_t)(y1 >> 8), (uint8_t)(y1)
    };

    /*
     * Keep CS low from CASET/RASET/RAMWR straight into pixel bytes.
     * Some modules require this contiguous transaction framing.
     */
    cs_select();
    dc_command();
    spi_write_byte(0x2A);
    dc_data();
    spi_write_blocking(SPI_PORT, p_col, sizeof(p_col));

    dc_command();
    spi_write_byte(0x2B);
    dc_data();
    spi_write_blocking(SPI_PORT, p_row, sizeof(p_row));

    dc_command();
    spi_write_byte(0x2C);
    dc_data();
}

/* ILI9341 expects two 8-bit writes per RGB565 pixel (MSB first). Stay in SPI 8-bit mode. */
static void write_pixels_rgb565_8(uint16_t color, uint32_t num_pixels)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;
    uint8_t buf[128];
    while (num_pixels > 0) {
        uint32_t chunk = num_pixels > 64 ? 64 : num_pixels;
        for (uint32_t i = 0; i < chunk; i++) {
            buf[i * 2]     = hi;
            buf[i * 2 + 1] = lo;
        }
        spi_write_blocking(SPI_PORT, buf, (size_t)(chunk * 2));
        num_pixels -= chunk;
    }
}

void display_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    begin_memory_write(x, y, x, y);
    write_pixels_rgb565_8(color, 1);
    cs_deselect();
}

void display_fill(uint16_t color)
{
    display_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH  - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    begin_memory_write(x, y, x + w - 1, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;
    uint8_t row_buf[TFT_WIDTH * 2];
    uint32_t row_bytes = (uint32_t)w * 2;
    for (uint32_t i = 0; i < row_bytes; i += 2) {
        row_buf[i]     = hi;
        row_buf[i + 1] = lo;
    }
    for (uint16_t r = 0; r < h; r++) {
        spi_write_blocking(SPI_PORT, row_buf, row_bytes);
    }
    cs_deselect();
}

/* ──────────────────────── 5x7 bitmap font ────────────────────────── */

/* Covers ASCII 32 (' ') through 126 ('~'). Each glyph is 5 columns of
 * 7-bit rows, packed into uint8_t[5].  Row bit 0 is the top pixel.      */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32 ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33 '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* 34 '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35 '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36 '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* 37 '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* 38 '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* 39 ''' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40 '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41 ')' */
    {0x14,0x08,0x3E,0x08,0x14}, /* 42 '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43 '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* 44 ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* 45 '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* 46 '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* 47 '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48 '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49 '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* 50 '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51 '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52 '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* 53 '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54 '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* 55 '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* 56 '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57 '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* 58 ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* 59 ';' */
    {0x08,0x14,0x22,0x41,0x00}, /* 60 '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* 61 '=' */
    {0x00,0x41,0x22,0x14,0x08}, /* 62 '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* 63 '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64 '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 70 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 71 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 77 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 83 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 87 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 88 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 89 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 90 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* 91 '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* 92 '\\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* 93 ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* 94 '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* 95 '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* 96 '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 97 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 98 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 99 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /*100 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /*101 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /*102 'f' */
    {0x0C,0x52,0x52,0x52,0x3E}, /*103 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /*104 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /*105 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /*106 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /*107 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /*108 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /*109 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /*110 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /*111 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /*112 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /*113 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /*114 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /*115 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /*116 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /*117 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /*118 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /*119 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /*120 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /*121 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /*122 'z' */
    {0x00,0x08,0x36,0x41,0x00}, /*123 '{' */
    {0x00,0x00,0x7F,0x00,0x00}, /*124 '|' */
    {0x00,0x41,0x36,0x08,0x00}, /*125 '}' */
    {0x10,0x08,0x08,0x10,0x08}, /*126 '~' */
};

uint16_t display_char(uint16_t x, uint16_t y, char c, uint16_t color,
                      uint16_t bg, uint8_t scale)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font5x7[c - 32];

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 7; row++) {
            uint16_t clr = (line & (1 << row)) ? color : bg;
            if (scale == 1) {
                display_pixel(x + col, y + row, clr);
            } else {
                display_fill_rect(x + col * scale, y + row * scale,
                                  scale, scale, clr);
            }
        }
    }
    /* 1-pixel gap between characters */
    if (scale == 1) {
        for (uint8_t row = 0; row < 7; row++)
            display_pixel(x + 5, y + row, bg);
    } else {
        display_fill_rect(x + 5 * scale, y, scale, 7 * scale, bg);
    }

    return x + 6 * scale;
}

uint16_t display_string(uint16_t x, uint16_t y, const char *s,
                        uint16_t color, uint16_t bg, uint8_t scale)
{
    while (*s) {
        x = display_char(x, y, *s++, color, bg, scale);
    }
    return x;
}

/* ──────────────────────── hardware test ──────────────────────────── */

void display_test(void)
{
    display_fill(COLOR_RED);
    sleep_ms(500);
    display_fill(COLOR_GREEN);
    sleep_ms(500);
    display_fill(COLOR_BLUE);
    sleep_ms(500);
    display_fill(COLOR_WHITE);
    sleep_ms(300);
    display_fill(COLOR_BLACK);
    display_string(20, 140, "Display OK", COLOR_WHITE, COLOR_BLACK, 2);
    sleep_ms(1000);
}
