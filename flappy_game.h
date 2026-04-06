/*
 * flappy_game.h — Flappy Bird game logic for portrait display (240x320).
 */

#ifndef FLAPPY_GAME_H
#define FLAPPY_GAME_H

#include <stdbool.h>
#include <stdint.h>

/* Screen dimensions (portrait mode) */
#define FLAPPY_SCREEN_W   TFT_WIDTH    /* 240 */
#define FLAPPY_SCREEN_H   TFT_HEIGHT   /* 320 */

#define FLAPPY_PIPE_COUNT 3

typedef enum {
    FLAPPY_PLAYING = 0,
    FLAPPY_OVER
} flappy_state_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} flappy_rect_t;

typedef struct {
    float x;
    int gap_y;
    int gap_h;
    int w;
    bool scored;
} flappy_pipe_t;

typedef struct {
    flappy_state_t state;
    int score;
    int high_score;

    float bird_x;
    float bird_y;
    float bird_vy;

    flappy_rect_t bird_rect_now;
    flappy_rect_t bird_rect_prev;

    flappy_pipe_t pipes[FLAPPY_PIPE_COUNT];
    flappy_rect_t pipe_draw_rects[FLAPPY_PIPE_COUNT * 2]; /* top/bottom rect per pipe */
    int pipe_rect_count;

    bool redraw_all;
    int prev_score;
} flappy_game_t;

void flappy_game_init(flappy_game_t* g, int loaded_high_score);
void flappy_game_update(flappy_game_t* g, bool flap, float dt_s);
void flappy_game_build_render_data(flappy_game_t* g);
void flappy_game_start(flappy_game_t* g);

#endif /* FLAPPY_GAME_H */
