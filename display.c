#include "display.h"

#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "pins.h"

// ILI9341 command set
#define ILI9341_SWRESET   0x01
#define ILI9341_SLPOUT    0x11
#define ILI9341_GAMSET    0x26
#define ILI9341_DISPOFF   0x28
#define ILI9341_DISPON    0x29
#define ILI9341_CASET     0x2A
#define ILI9341_PASET     0x2B
#define ILI9341_RAMWR     0x2C
#define ILI9341_MADCTL    0x36
#define ILI9341_PIXFMT    0x3A
#define ILI9341_FRMCTR1   0xB1
#define ILI9341_DFUNCTR   0xB6
#define ILI9341_PWCTR1    0xC0
#define ILI9341_PWCTR2    0xC1
#define ILI9341_VMCTR1    0xC5
#define ILI9341_VMCTR2    0xC7
#define ILI9341_GMCTRP1   0xE0
#define ILI9341_GMCTRN1   0xE1

static inline void tft_select(void) { gpio_put(TFT_PIN_CS, 0); }
static inline void tft_deselect(void) { gpio_put(TFT_PIN_CS, 1); }
static inline void tft_dc_cmd(void) { gpio_put(TFT_PIN_DC, 0); }
static inline void tft_dc_data(void) { gpio_put(TFT_PIN_DC, 1); }

static void tft_write_cmd(uint8_t cmd) {
    tft_select();
    tft_dc_cmd();
    spi_write_blocking(TFT_SPI_PORT, &cmd, 1);
    tft_deselect();
}

static void tft_write_data(const uint8_t* data, size_t len) {
    tft_select();
    tft_dc_data();
    spi_write_blocking(TFT_SPI_PORT, data, len);
    tft_deselect();
}

static void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];
    tft_write_cmd(ILI9341_CASET);
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFF);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFF);
    tft_write_data(data, 4);

    tft_write_cmd(ILI9341_PASET);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFF);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFF);
    tft_write_data(data, 4);

    tft_write_cmd(ILI9341_RAMWR);
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    uint8_t px[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    tft_set_addr_window(x, y, x, y);
    tft_write_data(px, sizeof(px));
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || w == 0 || h == 0) return;
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;

    tft_set_addr_window(x, y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

    uint8_t line[DISPLAY_WIDTH * 2];
    for (uint16_t i = 0; i < w; i++) {
        line[2 * i] = (uint8_t)(color >> 8);
        line[2 * i + 1] = (uint8_t)(color & 0xFF);
    }

    tft_select();
    tft_dc_data();
    for (uint16_t row = 0; row < h; row++) {
        spi_write_blocking(TFT_SPI_PORT, line, w * 2);
    }
    tft_deselect();
}

void display_fill_screen(uint16_t color) {
    display_fill_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}

void display_draw_sprite_8x8(uint16_t x, uint16_t y, const uint8_t sprite[8], uint16_t fg, uint16_t bg) {
    for (uint16_t row = 0; row < 8; row++) {
        for (uint16_t col = 0; col < 8; col++) {
            bool bit = (sprite[row] >> (7 - col)) & 0x1;
            display_draw_pixel((uint16_t)(x + col), (uint16_t)(y + row), bit ? fg : bg);
        }
    }
}

static const uint8_t digit_3x5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
};

static void draw_digit_3x5(uint16_t x, uint16_t y, int d, uint16_t fg, uint16_t bg) {
    if (d < 0 || d > 9) return;
    for (uint16_t r = 0; r < 5; r++) {
        for (uint16_t c = 0; c < 3; c++) {
            bool on = (digit_3x5[d][r] >> (2 - c)) & 1;
            display_draw_pixel((uint16_t)(x + c), (uint16_t)(y + r), on ? fg : bg);
        }
    }
}

void display_draw_score(uint16_t x, uint16_t y, int score, uint16_t color, uint16_t bg) {
    if (score < 0) score = 0;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", score);
    uint16_t cursor = x;
    for (size_t i = 0; i < strlen(buf); i++) {
        int d = buf[i] - '0';
        draw_digit_3x5(cursor, y, d, color, bg);
        cursor += 4;
    }
}

static void tft_hw_reset(void) {
    gpio_put(TFT_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(TFT_PIN_RST, 1);
    sleep_ms(120);
}

void display_init(void) {
    // Conservative clock for common breakout wiring quality.
    spi_init(TFT_SPI_PORT, 32 * 1000 * 1000);
    gpio_set_function(TFT_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(TFT_PIN_CS);
    gpio_set_dir(TFT_PIN_CS, GPIO_OUT);
    gpio_put(TFT_PIN_CS, 1);

    gpio_init(TFT_PIN_DC);
    gpio_set_dir(TFT_PIN_DC, GPIO_OUT);
    gpio_put(TFT_PIN_DC, 1);

    gpio_init(TFT_PIN_RST);
    gpio_set_dir(TFT_PIN_RST, GPIO_OUT);

    gpio_init(TFT_PIN_BL);
    gpio_set_dir(TFT_PIN_BL, GPIO_OUT);
    gpio_put(TFT_PIN_BL, 1);

    tft_hw_reset();

    tft_write_cmd(ILI9341_SWRESET);
    sleep_ms(150);

    tft_write_cmd(ILI9341_DISPOFF);
    tft_write_cmd(ILI9341_SLPOUT);
    sleep_ms(150);

    // Pixel format = RGB565
    tft_write_cmd(ILI9341_PIXFMT);
    uint8_t pixfmt = 0x55;
    tft_write_data(&pixfmt, 1);

    // Landscape orientation, BGR color order.
    tft_write_cmd(ILI9341_MADCTL);
    uint8_t madctl = 0x28;
    tft_write_data(&madctl, 1);

    uint8_t frmctr1[] = {0x00, 0x18};
    tft_write_cmd(ILI9341_FRMCTR1);
    tft_write_data(frmctr1, sizeof(frmctr1));

    uint8_t dfunc[] = {0x08, 0x82, 0x27};
    tft_write_cmd(ILI9341_DFUNCTR);
    tft_write_data(dfunc, sizeof(dfunc));

    uint8_t pwctr1 = 0x23;
    tft_write_cmd(ILI9341_PWCTR1);
    tft_write_data(&pwctr1, 1);

    uint8_t pwctr2 = 0x10;
    tft_write_cmd(ILI9341_PWCTR2);
    tft_write_data(&pwctr2, 1);

    uint8_t vmctr1[] = {0x3E, 0x28};
    tft_write_cmd(ILI9341_VMCTR1);
    tft_write_data(vmctr1, sizeof(vmctr1));

    uint8_t vmctr2 = 0x86;
    tft_write_cmd(ILI9341_VMCTR2);
    tft_write_data(&vmctr2, 1);

    uint8_t gamma = 0x01;
    tft_write_cmd(ILI9341_GAMSET);
    tft_write_data(&gamma, 1);

    uint8_t gm_pos[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    tft_write_cmd(ILI9341_GMCTRP1);
    tft_write_data(gm_pos, sizeof(gm_pos));

    uint8_t gm_neg[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    tft_write_cmd(ILI9341_GMCTRN1);
    tft_write_data(gm_neg, sizeof(gm_neg));

    tft_write_cmd(ILI9341_DISPON);
    sleep_ms(100);

    display_fill_screen(COLOR_BLACK);
}

static void clear_rect_safe(Rect r) {
    if (r.w <= 0 || r.h <= 0) return;
    display_fill_rect((uint16_t)r.x, (uint16_t)r.y, (uint16_t)r.w, (uint16_t)r.h, COLOR_BLACK);
}

static void draw_pipe(Rect p) {
    if (p.w <= 0 || p.h <= 0) return;
    display_fill_rect((uint16_t)p.x, (uint16_t)p.y, (uint16_t)p.w, (uint16_t)p.h, COLOR_GREEN);
}

void display_render_playfield(
    const Rect* pipes,
    int pipe_count,
    Rect bird_now,
    Rect bird_prev,
    bool redraw_all,
    int score_now,
    int score_prev
) {
    if (redraw_all) {
        display_fill_screen(COLOR_BLACK);
    }

    // Erase old bird and score area; draw new bird.
    if (!redraw_all) {
        clear_rect_safe(bird_prev);
        if (score_now != score_prev) {
            display_fill_rect(2, 2, 40, 8, COLOR_BLACK);
        }
    }

    for (int i = 0; i < pipe_count; i++) {
        // Simple update strategy:
        // clear a 2px strip where the pipe used to be then draw current pipe.
        int clear_x = pipes[i].x + pipes[i].w;
        if (!redraw_all && clear_x >= 0 && clear_x < DISPLAY_WIDTH) {
            display_fill_rect((uint16_t)clear_x, 0, 2, DISPLAY_HEIGHT, COLOR_BLACK);
        }
        draw_pipe(pipes[i]);
    }

    display_fill_rect((uint16_t)bird_now.x, (uint16_t)bird_now.y, (uint16_t)bird_now.w, (uint16_t)bird_now.h, COLOR_YELLOW);
    display_draw_score(2, 2, score_now, COLOR_WHITE, COLOR_BLACK);
}

void display_render_menu(int high_score) {
    display_fill_screen(COLOR_BLUE);
    display_fill_rect(20, 20, 88, 16, COLOR_CYAN);
    display_fill_rect(20, 50, 88, 16, COLOR_CYAN);
    display_draw_score(24, 24, high_score, COLOR_BLACK, COLOR_CYAN);
}

void display_render_game_over(int score, int high_score) {
    display_fill_screen(COLOR_RED);
    display_fill_rect(10, 30, 108, 20, COLOR_BLACK);
    display_draw_score(16, 36, score, COLOR_WHITE, COLOR_BLACK);
    display_fill_rect(10, 60, 108, 20, COLOR_BLACK);
    display_draw_score(16, 66, high_score, COLOR_GREEN, COLOR_BLACK);
}

void display_test(void) {
    display_fill_screen(COLOR_BLACK);
    display_fill_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT / 4, COLOR_RED);
    display_fill_rect(0, DISPLAY_HEIGHT / 4, DISPLAY_WIDTH, DISPLAY_HEIGHT / 4, COLOR_GREEN);
    display_fill_rect(0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT / 4, COLOR_BLUE);
    display_fill_rect(0, (3 * DISPLAY_HEIGHT) / 4, DISPLAY_WIDTH, DISPLAY_HEIGHT / 4, COLOR_WHITE);

    for (uint16_t i = 0; i < 40; i++) {
        display_draw_pixel((uint16_t)(i * 4), (uint16_t)(i * 3), COLOR_YELLOW);
    }
    display_draw_score(8, 8, 9341, COLOR_BLACK, COLOR_RED);
}
