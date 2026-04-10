/*
 * invaders.c — Simplified Space Invaders for RP2350 game console.
 *
 * Design goals:
 * - Small state machine: MENU -> PLAYING -> GAME_OVER -> MENU(return)
 * - Fixed-step update loop (16 ms)
 * - Partial redraw: erase previous moving objects only
 * - Minimal allocations and simple integer math for embedded targets
 */

#include "invaders.h"

#include "display.h"
#include "input.h"
#include "audio.h"
#include "eeprom.h"
#include "pins.h"

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>

/* Optional debug tracing to USB serial. */
#ifndef INVADERS_DEBUG
#define INVADERS_DEBUG 0
#endif

#if INVADERS_DEBUG
#define INV_DBG(...) printf(__VA_ARGS__)
#else
#define INV_DBG(...) ((void)0)
#endif

#define GAME_DT_MS            16
/* Layout and entity sizes. */
#define HUD_H                 14
#define PLAYER_W              18
#define PLAYER_H               8
#define PLAYER_Y_MARGIN       10
#define PLAYER_SPEED_PX        3

#define ENEMY_ROWS             4
#define ENEMY_COLS             8
#define ENEMY_W               14
#define ENEMY_H                9
#define ENEMY_GAP_X            8
#define ENEMY_GAP_Y           10
#define ENEMY_START_X         18
#define ENEMY_START_Y         28
#define ENEMY_STEP_X           4
#define ENEMY_STEP_Y           8
#define ENEMY_TICK_MS        220
#define ENEMY_TICK_MIN_MS     90

#define BULLET_MAX             2
#define BULLET_W               2
#define BULLET_H               6
#define BULLET_SPEED_PX        8
#define AUTO_FIRE_MS         180

#define SCORE_PER_ENEMY       10
#define ADC_MID               2048
#define JOY_X_DEADZONE         350

typedef enum {
    INV_STATE_MENU = 0,
    INV_STATE_PLAYING,
    INV_STATE_GAME_OVER
} inv_state_t;

typedef struct {
    int16_t x;
    int16_t y;
    bool active;
} bullet_t;

typedef struct {
    inv_state_t state;

    int16_t player_x;
    int16_t player_prev_x;
    int16_t player_y;

    bool enemies_alive[ENEMY_ROWS][ENEMY_COLS];
    int16_t enemy_offs_x;
    int16_t enemy_offs_y;
    int8_t enemy_dir;
    uint32_t enemy_accum_ms;
    uint16_t enemy_tick_ms;
    int16_t enemy_prev_offs_x;
    int16_t enemy_prev_offs_y;

    bullet_t bullets[BULLET_MAX];
    bullet_t bullets_prev[BULLET_MAX];
    uint32_t shoot_cooldown_ms;

    uint32_t score;
    uint32_t high_score;
    uint32_t prev_score;

    bool full_redraw;
} inv_game_t;

static void draw_number(uint16_t x, uint16_t y, uint32_t val,
                        uint16_t color, uint16_t bg, uint8_t scale)
{
    char buf[12];
    int i = 0;
    if (val == 0) {
        buf[i++] = '0';
    } else {
        char tmp[11];
        int t = 0;
        while (val > 0) { tmp[t++] = '0' + (char)(val % 10); val /= 10; }
        while (t > 0) { buf[i++] = tmp[--t]; }
    }
    buf[i] = '\0';
    display_string(x, y, buf, color, bg, scale);
}

static void init_round(inv_game_t *g)
{
    g->player_x = (int16_t)((TFT_WIDTH - PLAYER_W) / 2);
    g->player_prev_x = g->player_x;
    g->player_y = (int16_t)(TFT_HEIGHT - PLAYER_Y_MARGIN - PLAYER_H);

    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            g->enemies_alive[r][c] = true;
        }
    }

    g->enemy_offs_x = 0;
    g->enemy_offs_y = 0;
    g->enemy_prev_offs_x = 0;
    g->enemy_prev_offs_y = 0;
    g->enemy_dir = 1;
    g->enemy_accum_ms = 0;
    g->enemy_tick_ms = ENEMY_TICK_MS;

    for (int i = 0; i < BULLET_MAX; i++) {
        g->bullets[i].active = false;
        g->bullets_prev[i].active = false;
    }
    g->shoot_cooldown_ms = 0;

    g->score = 0;
    g->prev_score = (uint32_t)-1;
    g->full_redraw = true;
}

static void invaders_game_init(inv_game_t *g)
{
    g->high_score = eeprom_read_high_score(EEPROM_ADDR_INVADERS_HS);
    g->state = INV_STATE_MENU;
    init_round(g);
}

static inline int16_t enemy_x_at(int col, int16_t offs_x)
{
    return (int16_t)(ENEMY_START_X + col * (ENEMY_W + ENEMY_GAP_X) + offs_x);
}

static inline int16_t enemy_y_at(int row, int16_t offs_y)
{
    return (int16_t)(ENEMY_START_Y + row * (ENEMY_H + ENEMY_GAP_Y) + offs_y);
}

static bool enemies_any_alive(const inv_game_t *g)
{
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (g->enemies_alive[r][c]) return true;
        }
    }
    return false;
}

static void draw_hud(const inv_game_t *g, bool force)
{
    if (force) {
        display_fill_rect(0, 0, TFT_WIDTH, HUD_H, COLOR_BLACK);
        display_fill_rect(0, HUD_H - 1, TFT_WIDTH, 1, COLOR_DARK_GRAY);
        display_string(4, 3, "SCORE", COLOR_GRAY, COLOR_BLACK, 1);
        display_string((uint16_t)(TFT_WIDTH / 2), 3, "HIGH", COLOR_GRAY, COLOR_BLACK, 1);
    }
    if (force || g->score != g->prev_score) {
        display_fill_rect(42, 3, 56, 9, COLOR_BLACK);
        draw_number(42, 3, g->score, COLOR_WHITE, COLOR_BLACK, 1);
    }
    if (force) {
        display_fill_rect((uint16_t)(TFT_WIDTH / 2 + 28), 3, 56, 9, COLOR_BLACK);
        draw_number((uint16_t)(TFT_WIDTH / 2 + 28), 3, g->high_score, COLOR_YELLOW, COLOR_BLACK, 1);
    }
}

/* Draw or erase all enemies at a specific offset snapshot. */
static void draw_enemy_field(const inv_game_t *g, int16_t offs_x, int16_t offs_y, uint16_t color)
{
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (!g->enemies_alive[r][c]) continue;
            int16_t ex = enemy_x_at(c, offs_x);
            int16_t ey = enemy_y_at(r, offs_y);
            if (ex < 0 || ey < HUD_H || ex + ENEMY_W >= TFT_WIDTH || ey + ENEMY_H >= TFT_HEIGHT) continue;
            display_fill_rect((uint16_t)ex, (uint16_t)ey, ENEMY_W, ENEMY_H, color);
        }
    }
}

static void draw_player(int16_t x, int16_t y, uint16_t color)
{
    display_fill_rect((uint16_t)x, (uint16_t)y, PLAYER_W, PLAYER_H, color);
}

static void draw_bullet(const bullet_t *b, uint16_t color)
{
    if (!b->active) return;
    if (b->x < 0 || b->y < HUD_H || b->x + BULLET_W >= TFT_WIDTH || b->y + BULLET_H >= TFT_HEIGHT) return;
    display_fill_rect((uint16_t)b->x, (uint16_t)b->y, BULLET_W, BULLET_H, color);
}

static void render_playing(inv_game_t *g)
{
    if (g->full_redraw) {
        display_fill(COLOR_BLACK);
        draw_hud(g, true);
        draw_enemy_field(g, g->enemy_offs_x, g->enemy_offs_y, COLOR_GREEN);
        draw_player(g->player_x, g->player_y, COLOR_CYAN);
        for (int i = 0; i < BULLET_MAX; i++) draw_bullet(&g->bullets[i], COLOR_WHITE);
        g->full_redraw = false;
        g->prev_score = g->score;
        return;
    }

    /* Erase previous moving objects only, then draw current positions. */
    draw_enemy_field(g, g->enemy_prev_offs_x, g->enemy_prev_offs_y, COLOR_BLACK);
    draw_player(g->player_prev_x, g->player_y, COLOR_BLACK);
    for (int i = 0; i < BULLET_MAX; i++) draw_bullet(&g->bullets_prev[i], COLOR_BLACK);

    draw_enemy_field(g, g->enemy_offs_x, g->enemy_offs_y, COLOR_GREEN);
    draw_player(g->player_x, g->player_y, COLOR_CYAN);
    for (int i = 0; i < BULLET_MAX; i++) draw_bullet(&g->bullets[i], COLOR_WHITE);

    draw_hud(g, false);
    g->prev_score = g->score;
}

static void render_menu(void)
{
    display_fill(COLOR_BLACK);
    display_string(18, 48, "SPACE INVADERS", COLOR_CYAN, COLOR_BLACK, 2);
    display_fill_rect(18, 74, 204, 2, COLOR_CYAN);
    display_string(20, 120, "MOVE: JOYSTICK X", COLOR_WHITE, COLOR_BLACK, 1);
    display_string(20, 138, "SHOOT: BUTTON", COLOR_WHITE, COLOR_BLACK, 1);
    display_string(20, 156, "HIT ALL ENEMIES", COLOR_WHITE, COLOR_BLACK, 1);
    display_string(20, 280, "BUTTON TO START", COLOR_GRAY, COLOR_BLACK, 1);
}

static void render_game_over(const inv_game_t *g)
{
    display_fill(COLOR_BLACK);
    display_string(22, 58, "GAME OVER", COLOR_RED, COLOR_BLACK, 3);
    display_string(28, 150, "SCORE", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(28, 178, g->score, COLOR_WHITE, COLOR_BLACK, 2);
    display_string(28, 220, "HIGH", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(28, 248, g->high_score, COLOR_YELLOW, COLOR_BLACK, 2);
    display_string(18, 296, "BUTTON/BACK TO MENU", COLOR_GRAY, COLOR_BLACK, 1);
}

static void update_player(inv_game_t *g, const input_state_t *in)
{
    g->player_prev_x = g->player_x;
    int16_t dx = (int16_t)(in->raw_x - ADC_MID);
    if (dx < -JOY_X_DEADZONE) g->player_x -= PLAYER_SPEED_PX;
    if (dx > JOY_X_DEADZONE)  g->player_x += PLAYER_SPEED_PX;
    if (g->player_x < 0) g->player_x = 0;
    if (g->player_x > (int16_t)(TFT_WIDTH - PLAYER_W)) g->player_x = (int16_t)(TFT_WIDTH - PLAYER_W);
}

static void spawn_bullet(inv_game_t *g)
{
    if (g->shoot_cooldown_ms > 0) return;
    for (int i = 0; i < BULLET_MAX; i++) {
        if (!g->bullets[i].active) {
            g->bullets[i].active = true;
            g->bullets[i].x = (int16_t)(g->player_x + PLAYER_W / 2 - BULLET_W / 2);
            g->bullets[i].y = (int16_t)(g->player_y - BULLET_H - 1);
            g->shoot_cooldown_ms = AUTO_FIRE_MS;
            /* Keep auto-fire silent to avoid repetitive ear fatigue. */
            return;
        }
    }
}

static void update_enemies(inv_game_t *g, uint32_t dt_ms)
{
    g->enemy_prev_offs_x = g->enemy_offs_x;
    g->enemy_prev_offs_y = g->enemy_offs_y;

    g->enemy_accum_ms += dt_ms;
    while (g->enemy_accum_ms >= g->enemy_tick_ms) {
        g->enemy_accum_ms -= g->enemy_tick_ms;

        int16_t next_offs_x = (int16_t)(g->enemy_offs_x + g->enemy_dir * ENEMY_STEP_X);
        bool hit_edge = false;

        for (int r = 0; r < ENEMY_ROWS && !hit_edge; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (!g->enemies_alive[r][c]) continue;
                int16_t nx = enemy_x_at(c, next_offs_x);
                if (nx <= 2 || nx + ENEMY_W >= (int16_t)(TFT_WIDTH - 2)) {
                    hit_edge = true;
                    break;
                }
            }
        }

        if (hit_edge) {
            g->enemy_dir = (int8_t)-g->enemy_dir;
            g->enemy_offs_y += ENEMY_STEP_Y;
        } else {
            g->enemy_offs_x = next_offs_x;
        }
    }
}

static bool rects_overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                          int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

static bool update_bullets_and_collisions(inv_game_t *g)
{
    for (int i = 0; i < BULLET_MAX; i++) {
        g->bullets_prev[i] = g->bullets[i];

        if (!g->bullets[i].active) continue;
        g->bullets[i].y -= BULLET_SPEED_PX;
        if (g->bullets[i].y < HUD_H) {
            g->bullets[i].active = false;
            continue;
        }

        for (int r = 0; r < ENEMY_ROWS; r++) {
            bool hit_this_row = false;
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (!g->enemies_alive[r][c]) continue;
                int16_t ex = enemy_x_at(c, g->enemy_offs_x);
                int16_t ey = enemy_y_at(r, g->enemy_offs_y);
                if (rects_overlap(g->bullets[i].x, g->bullets[i].y, BULLET_W, BULLET_H,
                                  ex, ey, ENEMY_W, ENEMY_H)) {
                    g->enemies_alive[r][c] = false;
                    g->bullets[i].active = false;
                    g->score += SCORE_PER_ENEMY;
                    g->full_redraw = true; /* remove destroyed enemy immediately */
                    if (g->score > g->high_score) {
                        g->high_score = g->score;
                        eeprom_write_high_score(EEPROM_ADDR_INVADERS_HS, g->high_score);
                    }
                    if (g->enemy_tick_ms > ENEMY_TICK_MIN_MS) {
                        g->enemy_tick_ms = (uint16_t)(g->enemy_tick_ms - 4);
                    }
                    audio_play_tone(320, 14); /* short, softer enemy hit */
                    hit_this_row = true;
                    break;
                }
            }
            if (hit_this_row) break;
        }
    }

    return enemies_any_alive(g);
}

static bool enemies_reached_player_or_bottom(const inv_game_t *g)
{
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (!g->enemies_alive[r][c]) continue;
            int16_t ex = enemy_x_at(c, g->enemy_offs_x);
            int16_t ey = enemy_y_at(r, g->enemy_offs_y);

            if (ey + ENEMY_H >= g->player_y) return true;
            if (rects_overlap(ex, ey, ENEMY_W, ENEMY_H,
                              g->player_x, g->player_y, PLAYER_W, PLAYER_H)) {
                return true;
            }
        }
    }
    return false;
}

void invaders_run(void)
{
    inv_game_t g;
    invaders_game_init(&g);
    render_menu();

    absolute_time_t next_tick = make_timeout_time_ms(GAME_DT_MS);

    while (true) {
        input_state_t in = input_read();
        bool start_pressed = in.button_pressed || in.button_held;
        audio_update();

        if (time_reached(next_tick)) {
            next_tick = delayed_by_ms(next_tick, GAME_DT_MS);

            if (g.state == INV_STATE_MENU) {
                if (start_pressed) {
                    audio_play_sfx(SFX_MENU_SELECT);
                    init_round(&g);
                    g.state = INV_STATE_PLAYING;
                    g.full_redraw = true;
                    INV_DBG("Invaders start\n");
                }
                continue;
            }

            if (g.state == INV_STATE_PLAYING) {
                if (in.back_pressed) {
                    return;
                }

                if (g.shoot_cooldown_ms >= GAME_DT_MS) g.shoot_cooldown_ms -= GAME_DT_MS;
                else g.shoot_cooldown_ms = 0;

                update_player(&g, &in);
                spawn_bullet(&g); /* auto-fire while playing */

                update_enemies(&g, GAME_DT_MS);
                bool any_left = update_bullets_and_collisions(&g);
                bool invaded = enemies_reached_player_or_bottom(&g);

                if (!any_left || invaded) {
                    g.state = INV_STATE_GAME_OVER;
                    audio_play_sfx(SFX_GAME_OVER);
                    render_game_over(&g);
                    continue;
                }

                render_playing(&g);
                continue;
            }

            /* INV_STATE_GAME_OVER */
            if (start_pressed || in.back_pressed) {
                return;
            }
        }

        tight_loop_contents();
    }
}
