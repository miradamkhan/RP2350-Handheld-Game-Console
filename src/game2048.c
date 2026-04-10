/*
 * game2048.c — Playable 2048 game for RP2350 console.
 *
 * Notes:
 * - Full redraw on move/state change (2048 is low update-rate).
 * - One joystick move per tilt: stick must return to center first.
 * - Merge rule: each tile merges at most once per move.
 */

#include "game2048.h"

#include "display.h"
#include "audio.h"
#include "eeprom.h"

#include "pico/stdlib.h"

#include <stdlib.h>
#include <string.h>

#define GAME_DT_MS              16
#define BOARD_N                  4

#define BOARD_X                 16
#define BOARD_Y                 56
#define CELL_SZ                 50
#define CELL_GAP                 4

typedef enum {
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} move_dir_t;

static int rand_range(int min_v, int max_v)
{
    return min_v + (rand() % (max_v - min_v + 1));
}

static int count_empty(const game2048_t *g)
{
    int n = 0;
    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            if (g->board[r][c] == 0) n++;
        }
    }
    return n;
}

/* Spawn one new tile after a valid move. */
static bool spawn_tile(game2048_t *g)
{
    int empty = count_empty(g);
    if (empty == 0) return false;

    int pick = rand_range(0, empty - 1);
    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            if (g->board[r][c] != 0) continue;
            if (pick == 0) {
                /* 90% spawn 2, 10% spawn 4. */
                g->board[r][c] = (rand_range(0, 9) == 0) ? 4 : 2;
                return true;
            }
            pick--;
        }
    }
    return false;
}

static bool has_2048(const game2048_t *g)
{
    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            if (g->board[r][c] >= 2048) return true;
        }
    }
    return false;
}

/* True if at least one valid move remains. */
static bool can_move(const game2048_t *g)
{
    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            int v = g->board[r][c];
            if (v == 0) return true;
            if (c + 1 < BOARD_N && g->board[r][c + 1] == v) return true;
            if (r + 1 < BOARD_N && g->board[r + 1][c] == v) return true;
        }
    }
    return false;
}

/*
 * Process one line to the left:
 * 1) compress non-zeros
 * 2) merge equal adjacent once
 * 3) compress again
 */
static bool slide_line_left(int line[BOARD_N], uint32_t *score_add, int *max_merge)
{
    int tmp[BOARD_N] = {0};
    int out = 0;
    bool moved = false;

    for (int i = 0; i < BOARD_N; i++) {
        if (line[i] != 0) tmp[out++] = line[i];
    }

    for (int i = 0; i < BOARD_N - 1; i++) {
        if (tmp[i] != 0 && tmp[i] == tmp[i + 1]) {
            tmp[i] *= 2;
            *score_add += (uint32_t)tmp[i];
            if (tmp[i] > *max_merge) *max_merge = tmp[i];
            tmp[i + 1] = 0;
            i++; /* merged tile cannot merge again this move */
        }
    }

    int out2[BOARD_N] = {0};
    out = 0;
    for (int i = 0; i < BOARD_N; i++) {
        if (tmp[i] != 0) out2[out++] = tmp[i];
    }

    for (int i = 0; i < BOARD_N; i++) {
        if (line[i] != out2[i]) moved = true;
        line[i] = out2[i];
    }
    return moved;
}

static void get_line(const game2048_t *g, move_dir_t dir, int idx, int line[BOARD_N])
{
    for (int i = 0; i < BOARD_N; i++) {
        switch (dir) {
        case DIR_LEFT:  line[i] = g->board[idx][i]; break;
        case DIR_RIGHT: line[i] = g->board[idx][BOARD_N - 1 - i]; break;
        case DIR_UP:    line[i] = g->board[i][idx]; break;
        case DIR_DOWN:  line[i] = g->board[BOARD_N - 1 - i][idx]; break;
        }
    }
}

static void set_line(game2048_t *g, move_dir_t dir, int idx, const int line[BOARD_N])
{
    for (int i = 0; i < BOARD_N; i++) {
        switch (dir) {
        case DIR_LEFT:  g->board[idx][i] = line[i]; break;
        case DIR_RIGHT: g->board[idx][BOARD_N - 1 - i] = line[i]; break;
        case DIR_UP:    g->board[i][idx] = line[i]; break;
        case DIR_DOWN:  g->board[BOARD_N - 1 - i][idx] = line[i]; break;
        }
    }
}

static bool apply_move(game2048_t *g, move_dir_t dir, int *max_merge)
{
    bool moved_any = false;
    uint32_t add = 0;
    *max_merge = 0;

    for (int idx = 0; idx < BOARD_N; idx++) {
        int line[BOARD_N];
        get_line(g, dir, idx, line);
        if (slide_line_left(line, &add, max_merge)) {
            moved_any = true;
        }
        set_line(g, dir, idx, line);
    }

    if (moved_any) {
        g->score += add;
        if (g->score > g->best_score) {
            g->best_score = g->score;
            eeprom_write_high_score(EEPROM_ADDR_2048_HS, g->best_score);
        }
    }
    return moved_any;
}

static void play_merge_sfx(int merged_value)
{
    /*
     * PWM output has fixed duty in this project, so "louder" is emulated by
     * longer/stronger tones as merged tile values increase.
     */
    if (merged_value >= 1024) {
        audio_play_tone(420, 80);
    } else if (merged_value >= 512) {
        audio_play_tone(390, 70);
    } else if (merged_value >= 256) {
        audio_play_tone(360, 60);
    } else if (merged_value >= 128) {
        audio_play_tone(330, 50);
    } else if (merged_value >= 64) {
        audio_play_tone(300, 40);
    } else {
        audio_play_tone(270, 30);
    }
}

static uint16_t tile_color(int v)
{
    switch (v) {
    case 0:    return RGB565(60, 60, 60);
    case 2:    return RGB565(238, 228, 218);
    case 4:    return RGB565(237, 224, 200);
    case 8:    return RGB565(242, 177, 121);
    case 16:   return RGB565(245, 149, 99);
    case 32:   return RGB565(246, 124, 95);
    case 64:   return RGB565(246, 94, 59);
    case 128:  return RGB565(237, 207, 114);
    case 256:  return RGB565(237, 204, 97);
    case 512:  return RGB565(237, 200, 80);
    case 1024: return RGB565(237, 197, 63);
    default:   return RGB565(237, 194, 46); /* 2048+ */
    }
}

static uint16_t tile_text_color(int v)
{
    return (v <= 4) ? RGB565(100, 90, 80) : COLOR_WHITE;
}

static int digits10(int v)
{
    int d = 1;
    while (v >= 10) { v /= 10; d++; }
    return d;
}

static void draw_number(uint16_t x, uint16_t y, uint32_t val,
                        uint16_t color, uint16_t bg, uint8_t scale)
{
    char buf[12];
    int i = 0;
    if (val == 0) {
        buf[i++] = '0';
    } else {
        char t[11];
        int n = 0;
        while (val > 0) { t[n++] = (char)('0' + (val % 10)); val /= 10; }
        while (n > 0) buf[i++] = t[--n];
    }
    buf[i] = '\0';
    display_string(x, y, buf, color, bg, scale);
}

void game2048_init(game2048_t *g, uint32_t loaded_best)
{
    memset(g, 0, sizeof(*g));
    g->best_score = loaded_best;
    g->state = GAME2048_STATE_MENU;
    g->input_armed = true;
    g->board_dirty = true;
}

void game2048_reset(game2048_t *g)
{
    memset(g->board, 0, sizeof(g->board));
    g->score = 0;
    g->won_latched = false;
    g->input_armed = true;
    spawn_tile(g);
    spawn_tile(g);
    g->state = GAME2048_STATE_PLAYING;
    g->board_dirty = true;
}

void game2048_render(const game2048_t *g)
{
    display_fill(COLOR_BLACK);
    display_string(86, 10, "2048", COLOR_YELLOW, COLOR_BLACK, 3);

    display_string(18, 34, "SCORE", COLOR_GRAY, COLOR_BLACK, 1);
    draw_number(58, 34, g->score, COLOR_WHITE, COLOR_BLACK, 1);
    display_string(138, 34, "BEST", COLOR_GRAY, COLOR_BLACK, 1);
    draw_number(170, 34, g->best_score, COLOR_CYAN, COLOR_BLACK, 1);

    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            int v = g->board[r][c];
            int16_t x = (int16_t)(BOARD_X + c * (CELL_SZ + CELL_GAP));
            int16_t y = (int16_t)(BOARD_Y + r * (CELL_SZ + CELL_GAP));
            display_fill_rect((uint16_t)x, (uint16_t)y, CELL_SZ, CELL_SZ, tile_color(v));

            if (v != 0) {
                int d = digits10(v);
                uint8_t scale = (v >= 1024) ? 1 : 2;
                int txt_w = d * (6 * scale);
                int txt_h = 7 * scale;
                int tx = x + (CELL_SZ - txt_w) / 2;
                int ty = y + (CELL_SZ - txt_h) / 2;
                draw_number((uint16_t)tx, (uint16_t)ty, (uint32_t)v,
                            tile_text_color(v), tile_color(v), scale);
            }
        }
    }

    if (g->state == GAME2048_STATE_MENU) {
        display_string(24, 286, "PRESS BUTTON TO START", COLOR_WHITE, COLOR_BLACK, 1);
    } else if (g->state == GAME2048_STATE_WIN) {
        display_fill_rect(26, 280, 188, 26, COLOR_BLACK);
        display_string(56, 286, "YOU WIN! BUTTON", COLOR_GREEN, COLOR_BLACK, 1);
    } else if (g->state == GAME2048_STATE_OVER) {
        display_fill_rect(16, 280, 208, 26, COLOR_BLACK);
        display_string(44, 286, "GAME OVER BUTTON", COLOR_RED, COLOR_BLACK, 1);
    }
}

void game2048_update(game2048_t *g, const input_state_t *in)
{
    if (g->state == GAME2048_STATE_MENU) {
        if (in->button_pressed || in->button_held) {
            game2048_reset(g);
        }
        return;
    }

    if (g->state == GAME2048_STATE_WIN || g->state == GAME2048_STATE_OVER) {
        if (in->button_pressed || in->button_held || in->back_pressed) {
            g->state = GAME2048_STATE_MENU;
            g->board_dirty = true;
        }
        return;
    }

    if (in->back_pressed) {
        g->state = GAME2048_STATE_MENU;
        g->board_dirty = true;
        return;
    }

    if (in->direction == JOY_NONE) {
        g->input_armed = true;
        return;
    }
    if (!g->input_armed) return;
    g->input_armed = false;

    move_dir_t dir;
    switch (in->direction) {
    case JOY_LEFT:  dir = DIR_LEFT;  break;
    case JOY_RIGHT: dir = DIR_RIGHT; break;
    case JOY_UP:    dir = DIR_UP;    break;
    case JOY_DOWN:  dir = DIR_DOWN;  break;
    default:        return;
    }

    int max_merge = 0;
    bool moved = apply_move(g, dir, &max_merge);
    if (!moved) return; /* invalid move: no spawn */

    spawn_tile(g);
    g->board_dirty = true;

    if (max_merge > 0) {
        play_merge_sfx(max_merge);
    }

    if (!g->won_latched && has_2048(g)) {
        g->won_latched = true;
        g->state = GAME2048_STATE_WIN;
        return;
    }

    if (!can_move(g)) {
        g->state = GAME2048_STATE_OVER;
    }
}

void game2048_run(void)
{
    game2048_t g;
    uint32_t best = eeprom_read_high_score(EEPROM_ADDR_2048_HS);
    game2048_init(&g, best);

    absolute_time_t next_tick = make_timeout_time_ms(GAME_DT_MS);

    while (true) {
        input_state_t in = input_read();
        audio_update();

        if (time_reached(next_tick)) {
            next_tick = delayed_by_ms(next_tick, GAME_DT_MS);

            game2048_state_t prev_state = g.state;
            game2048_update(&g, &in);

            if (g.state == GAME2048_STATE_MENU &&
                prev_state != GAME2048_STATE_MENU &&
                (in.button_pressed || in.button_held || in.back_pressed)) {
                return;
            }

            if (g.board_dirty) {
                game2048_render(&g);
                g.board_dirty = false;
            }
        }
        tight_loop_contents();
    }
}
