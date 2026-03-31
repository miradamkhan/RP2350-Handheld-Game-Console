#include "game.h"

#include <stdlib.h>

#include "display.h"

#define BIRD_W             8
#define BIRD_H             8
#define BIRD_X             24.0f
#define GRAVITY            520.0f
#define JUMP_VELOCITY     -180.0f
#define PIPE_SPEED         55.0f
#define PIPE_GAP_H         42
#define PIPE_W             16
#define GROUND_Y           (DISPLAY_HEIGHT - 2)

static int rand_range(int min_v, int max_v) {
    int span = max_v - min_v + 1;
    return min_v + (rand() % span);
}

static void reset_round(Game* g) {
    g->score = 0;
    g->prev_score = -1;
    g->bird_x = BIRD_X;
    g->bird_y = DISPLAY_HEIGHT / 2.0f;
    g->bird_vy = 0.0f;
    g->bird_rect_prev = (Rect){(int16_t)g->bird_x, (int16_t)g->bird_y, BIRD_W, BIRD_H};
    g->bird_rect_now = g->bird_rect_prev;

    float start_x = (float)(DISPLAY_WIDTH + 20);
    for (int i = 0; i < PIPE_COUNT; i++) {
        g->pipes[i].x = start_x + (float)(i * 50);
        g->pipes[i].gap_h = PIPE_GAP_H;
        g->pipes[i].gap_y = rand_range(28, DISPLAY_HEIGHT - 28 - PIPE_GAP_H);
        g->pipes[i].w = PIPE_W;
        g->pipes[i].scored = false;
    }

    game_build_render_data(g);
    g->redraw_all = true;
}

void game_init(Game* g, int loaded_high_score) {
    *g = (Game){0};
    g->state = GAME_MENU;
    g->high_score = loaded_high_score;
    reset_round(g);
}

void game_start(Game* g) {
    reset_round(g);
    g->state = GAME_PLAYING;
}

static bool rect_overlap(Rect a, Rect b) {
    if (a.x + a.w <= b.x) return false;
    if (b.x + b.w <= a.x) return false;
    if (a.y + a.h <= b.y) return false;
    if (b.y + b.h <= a.y) return false;
    return true;
}

void game_build_render_data(Game* g) {
    g->pipe_rect_count = 0;
    for (int i = 0; i < PIPE_COUNT; i++) {
        Pipe* p = &g->pipes[i];
        int16_t px = (int16_t)(p->x + 0.5f);
        Rect top = {px, 0, (int16_t)p->w, (int16_t)p->gap_y};
        Rect bot = {px, (int16_t)(p->gap_y + p->gap_h), (int16_t)p->w, (int16_t)(DISPLAY_HEIGHT - (p->gap_y + p->gap_h))};
        g->pipe_draw_rects[g->pipe_rect_count++] = top;
        g->pipe_draw_rects[g->pipe_rect_count++] = bot;
    }
}

void game_update(Game* g, const InputState* in, float dt_s) {
    g->bird_rect_prev = g->bird_rect_now;

    if (g->state == GAME_MENU) {
        if (in->button_just_pressed) {
            game_start(g);
        }
        return;
    }

    if (g->state == GAME_OVER) {
        if (in->button_just_pressed) {
            g->state = GAME_MENU;
            g->redraw_all = true;
        }
        return;
    }

    // GAME_PLAYING
    if (in->button_just_pressed) {
        g->bird_vy = JUMP_VELOCITY;
    }

    g->bird_vy += GRAVITY * dt_s;
    g->bird_y += g->bird_vy * dt_s;

    for (int i = 0; i < PIPE_COUNT; i++) {
        Pipe* p = &g->pipes[i];
        p->x -= PIPE_SPEED * dt_s;

        if (p->x + (float)p->w < 0.0f) {
            float max_x = 0.0f;
            for (int j = 0; j < PIPE_COUNT; j++) {
                if (g->pipes[j].x > max_x) max_x = g->pipes[j].x;
            }
            p->x = max_x + 50.0f;
            p->gap_y = rand_range(24, DISPLAY_HEIGHT - 24 - p->gap_h);
            p->scored = false;
        }

        if (!p->scored && p->x + (float)p->w < g->bird_x) {
            p->scored = true;
            g->score++;
        }
    }

    g->bird_rect_now = (Rect){(int16_t)g->bird_x, (int16_t)g->bird_y, BIRD_W, BIRD_H};
    game_build_render_data(g);

    // Ground and ceiling collision
    if (g->bird_y < 0 || (g->bird_y + BIRD_H) >= GROUND_Y) {
        g->state = GAME_OVER;
        g->redraw_all = true;
        return;
    }

    // Pipe collision
    for (int i = 0; i < g->pipe_rect_count; i++) {
        if (rect_overlap(g->bird_rect_now, g->pipe_draw_rects[i])) {
            g->state = GAME_OVER;
            g->redraw_all = true;
            return;
        }
    }
}
