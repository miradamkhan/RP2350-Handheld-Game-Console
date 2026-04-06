/*
 * flappy_game.c — Flappy Bird game logic adapted for portrait mode (240x320).
 */

#include "flappy_game.h"
#include "pins.h"

#include <stdlib.h>

#define BIRD_W             8
#define BIRD_H             8
#define BIRD_X             40.0f
#define GRAVITY            520.0f
#define JUMP_VELOCITY     -180.0f
#define PIPE_SPEED         55.0f
#define PIPE_GAP_H         52
#define PIPE_W             16
#define GROUND_Y           (FLAPPY_SCREEN_H - 2)

static int rand_range(int min_v, int max_v) {
    int span = max_v - min_v + 1;
    return min_v + (rand() % span);
}

static void reset_round(flappy_game_t* g) {
    g->score = 0;
    g->prev_score = -1;
    g->bird_x = BIRD_X;
    g->bird_y = FLAPPY_SCREEN_H / 2.0f;
    g->bird_vy = 0.0f;
    g->bird_rect_prev = (flappy_rect_t){(int16_t)g->bird_x, (int16_t)g->bird_y, BIRD_W, BIRD_H};
    g->bird_rect_now = g->bird_rect_prev;

    float start_x = (float)(FLAPPY_SCREEN_W + 20);
    for (int i = 0; i < FLAPPY_PIPE_COUNT; i++) {
        g->pipes[i].x = start_x + (float)(i * 80);
        g->pipes[i].gap_h = PIPE_GAP_H;
        g->pipes[i].gap_y = rand_range(40, FLAPPY_SCREEN_H - 40 - PIPE_GAP_H);
        g->pipes[i].w = PIPE_W;
        g->pipes[i].scored = false;
    }

    flappy_game_build_render_data(g);
    g->redraw_all = true;
}

void flappy_game_init(flappy_game_t* g, int loaded_high_score) {
    *g = (flappy_game_t){0};
    g->state = FLAPPY_PLAYING;
    g->high_score = loaded_high_score;
    reset_round(g);
}

void flappy_game_start(flappy_game_t* g) {
    reset_round(g);
    g->state = FLAPPY_PLAYING;
}

static bool rect_overlap(flappy_rect_t a, flappy_rect_t b) {
    if (a.x + a.w <= b.x) return false;
    if (b.x + b.w <= a.x) return false;
    if (a.y + a.h <= b.y) return false;
    if (b.y + b.h <= a.y) return false;
    return true;
}

void flappy_game_build_render_data(flappy_game_t* g) {
    g->pipe_rect_count = 0;
    for (int i = 0; i < FLAPPY_PIPE_COUNT; i++) {
        flappy_pipe_t* p = &g->pipes[i];
        int16_t px = (int16_t)(p->x + 0.5f);
        flappy_rect_t top = {px, 0, (int16_t)p->w, (int16_t)p->gap_y};
        flappy_rect_t bot = {px, (int16_t)(p->gap_y + p->gap_h), (int16_t)p->w,
                             (int16_t)(FLAPPY_SCREEN_H - (p->gap_y + p->gap_h))};
        g->pipe_draw_rects[g->pipe_rect_count++] = top;
        g->pipe_draw_rects[g->pipe_rect_count++] = bot;
    }
}

void flappy_game_update(flappy_game_t* g, bool flap, float dt_s) {
    g->bird_rect_prev = g->bird_rect_now;

    if (g->state == FLAPPY_OVER) {
        return;
    }

    /* PLAYING */
    if (flap) {
        g->bird_vy = JUMP_VELOCITY;
    }

    g->bird_vy += GRAVITY * dt_s;
    g->bird_y += g->bird_vy * dt_s;

    for (int i = 0; i < FLAPPY_PIPE_COUNT; i++) {
        flappy_pipe_t* p = &g->pipes[i];
        p->x -= PIPE_SPEED * dt_s;

        if (p->x + (float)p->w < 0.0f) {
            float max_x = 0.0f;
            for (int j = 0; j < FLAPPY_PIPE_COUNT; j++) {
                if (g->pipes[j].x > max_x) max_x = g->pipes[j].x;
            }
            p->x = max_x + 80.0f;
            p->gap_y = rand_range(36, FLAPPY_SCREEN_H - 36 - p->gap_h);
            p->scored = false;
        }

        if (!p->scored && p->x + (float)p->w < g->bird_x) {
            p->scored = true;
            g->score++;
        }
    }

    g->bird_rect_now = (flappy_rect_t){(int16_t)g->bird_x, (int16_t)g->bird_y, BIRD_W, BIRD_H};
    flappy_game_build_render_data(g);

    /* Ground and ceiling collision */
    if (g->bird_y < 0 || (g->bird_y + BIRD_H) >= GROUND_Y) {
        g->state = FLAPPY_OVER;
        g->redraw_all = true;
        return;
    }

    /* Pipe collision */
    for (int i = 0; i < g->pipe_rect_count; i++) {
        if (rect_overlap(g->bird_rect_now, g->pipe_draw_rects[i])) {
            g->state = FLAPPY_OVER;
            g->redraw_all = true;
            return;
        }
    }
}
