#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

// ILI9341 native resolution is 240x320.
// We configure landscape orientation in display.c => 320x240 logical drawing area.
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_BLUE      0x001F
#define COLOR_GREEN     0x07E0
#define COLOR_RED       0xF800
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Rect;

void display_init(void);
void display_fill_screen(uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_sprite_8x8(uint16_t x, uint16_t y, const uint8_t sprite[8], uint16_t fg, uint16_t bg);
void display_draw_score(uint16_t x, uint16_t y, int score, uint16_t color, uint16_t bg);

// Game-specific render helpers
void display_render_menu(int high_score);
void display_render_game_over(int score, int high_score);
void display_render_playfield(
    const Rect* pipes,
    int pipe_count,
    Rect bird_now,
    Rect bird_prev,
    bool redraw_all,
    int score_now,
    int score_prev
);
void display_test(void);

#endif // DISPLAY_H
