#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include "display.h"
#include "input.h"

#define PIPE_COUNT 3

typedef enum {
    GAME_MENU = 0,
    GAME_PLAYING,
    GAME_OVER
} GameState;

typedef struct {
    float x;
    int gap_y;
    int gap_h;
    int w;
    bool scored;
} Pipe;

typedef struct {
    GameState state;
    int score;
    int high_score;

    float bird_x;
    float bird_y;
    float bird_vy;

    Rect bird_rect_now;
    Rect bird_rect_prev;

    Pipe pipes[PIPE_COUNT];
    Rect pipe_draw_rects[PIPE_COUNT * 2]; // top/bottom rect per pipe
    int pipe_rect_count;

    bool redraw_all;
    int prev_score;
} Game;

void game_init(Game* g, int loaded_high_score);
void game_update(Game* g, const InputState* in, float dt_s);
void game_build_render_data(Game* g);
void game_start(Game* g);

#endif // GAME_H
