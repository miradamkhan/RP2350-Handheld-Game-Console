/*
 * flappy_render.c — Flappy Bird rendering and game loop.
 *
 * flappy_run() contains the complete game loop; it returns when the
 * player presses the button on the game-over screen (back to menu).
 */

#include "flappy_render.h"
#include "flappy_game.h"
#include "display.h"
#include "input.h"
#include "audio.h"
#include "eeprom.h"
#include "pins.h"

#include "pico/stdlib.h"
#include <stdio.h>

#define GAME_DT_MS  16

/* ── Drawing helpers ── */

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

static void clear_rect_safe(flappy_rect_t r) {
    if (r.w <= 0 || r.h <= 0) return;
    display_fill_rect((uint16_t)r.x, (uint16_t)r.y,
                      (uint16_t)r.w, (uint16_t)r.h, COLOR_BLACK);
}

static void draw_pipe(flappy_rect_t p) {
    if (p.w <= 0 || p.h <= 0) return;
    display_fill_rect((uint16_t)p.x, (uint16_t)p.y,
                      (uint16_t)p.w, (uint16_t)p.h, COLOR_GREEN);
}

static void render_playfield(const flappy_game_t* g, int score_prev) {
    if (g->redraw_all) {
        display_fill(COLOR_BLACK);
    }

    /* Erase old bird and score area; draw new bird. */
    if (!g->redraw_all) {
        clear_rect_safe(g->bird_rect_prev);
        if (g->score != score_prev) {
            display_fill_rect(2, 2, 60, 10, COLOR_BLACK);
        }
    }

    for (int i = 0; i < g->pipe_rect_count; i++) {
        /* Clear a 2px strip where the pipe used to be then draw current pipe. */
        int clear_x = g->pipe_draw_rects[i].x + g->pipe_draw_rects[i].w;
        if (!g->redraw_all && clear_x >= 0 && clear_x < FLAPPY_SCREEN_W) {
            display_fill_rect((uint16_t)clear_x, 0, 2, FLAPPY_SCREEN_H, COLOR_BLACK);
        }
        draw_pipe(g->pipe_draw_rects[i]);
    }

    display_fill_rect((uint16_t)g->bird_rect_now.x, (uint16_t)g->bird_rect_now.y,
                      (uint16_t)g->bird_rect_now.w, (uint16_t)g->bird_rect_now.h,
                      COLOR_YELLOW);

    draw_number(2, 2, (uint32_t)g->score, COLOR_WHITE, COLOR_BLACK, 1);
}

static void render_game_over(int score, int high_score) {
    display_fill(COLOR_BLACK);
    display_string(15, 60, "GAME OVER", COLOR_RED, COLOR_BLACK, 3);

    display_string(30, 140, "SCORE", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(30, 168, (uint32_t)score, COLOR_WHITE, COLOR_BLACK, 2);

    display_string(30, 210, "HIGH", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(30, 238, (uint32_t)high_score, COLOR_YELLOW, COLOR_BLACK, 2);

    display_string(20, 290, "Press Button", COLOR_GRAY, COLOR_BLACK, 1);
}

/* ────────────────────────── flappy_run ─────────────────────────────── */

void flappy_run(void)
{
    /* Load high score from EEPROM. */
    uint32_t high_raw = eeprom_read_high_score(EEPROM_ADDR_FLAPPY_HS);

    flappy_game_t game;
    flappy_game_init(&game, (int)high_raw);

    int last_rendered_score = -1;
    flappy_state_t last_state = FLAPPY_WAITING;

    absolute_time_t next_tick = make_timeout_time_ms(GAME_DT_MS);

    while (true) {
        input_state_t in = input_read();
        audio_update();

        while (time_reached(next_tick)) {
            next_tick = delayed_by_ms(next_tick, GAME_DT_MS);

            if (game.state == FLAPPY_WAITING) {
                /* Bird is suspended — update checks for first flap */
                flappy_game_update(&game, in.button_pressed, GAME_DT_MS / 1000.0f);

                /* Render the static playfield while waiting */
                render_playfield(&game, last_rendered_score);
                game.redraw_all = false;

            } else if (game.state == FLAPPY_PLAYING) {
                flappy_game_update(&game, in.button_pressed, GAME_DT_MS / 1000.0f);

                /* Sound triggers */
                if (in.button_pressed) {
                    audio_play_sfx(SFX_JUMP);
                }
                if (game.score != game.prev_score) {
                    if (game.prev_score >= 0 && game.score > game.prev_score) {
                        audio_play_sfx(SFX_SCORE);
                    }
                    game.prev_score = game.score;
                }
                if (last_state == FLAPPY_PLAYING && game.state == FLAPPY_OVER) {
                    audio_play_sfx(SFX_COLLISION);
                }

                /* Update high score */
                if (game.state == FLAPPY_OVER && game.score > game.high_score) {
                    game.high_score = game.score;
                    eeprom_write_high_score(EEPROM_ADDR_FLAPPY_HS, (uint32_t)game.high_score);
                }

                /* Render playing state */
                render_playfield(&game, last_rendered_score);
                game.redraw_all = false;
                last_rendered_score = game.score;

            } else {
                /* GAME_OVER */
                if (last_state != FLAPPY_OVER) {
                    render_game_over(game.score, game.high_score);
                }
                if (in.button_pressed) {
                    /* Return to main menu */
                    return;
                }
            }

            last_state = game.state;
        }

        tight_loop_contents();
    }
}
