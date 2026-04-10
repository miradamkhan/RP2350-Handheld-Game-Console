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

#define GAME_DT_MS  20
#define FLAP_COOLDOWN_MS 110

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

static void fill_rect_clipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;

    int16_t x0 = x;
    int16_t y0 = y;
    int16_t x1 = (int16_t)(x + w);
    int16_t y1 = (int16_t)(y + h);

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > FLAPPY_SCREEN_W) x1 = FLAPPY_SCREEN_W;
    if (y1 > FLAPPY_SCREEN_H) y1 = FLAPPY_SCREEN_H;
    if (x1 <= x0 || y1 <= y0) return;

    display_fill_rect((uint16_t)x0, (uint16_t)y0,
                      (uint16_t)(x1 - x0), (uint16_t)(y1 - y0), color);
}

static void clear_rect_safe(flappy_rect_t r) {
    fill_rect_clipped(r.x, r.y, r.w, r.h, COLOR_BLACK);
}

static void draw_pipe(flappy_rect_t p) {
    fill_rect_clipped(p.x, p.y, p.w, p.h, COLOR_GREEN);
}

/*
 * Reduce SPI overdraw/flicker by updating only the horizontal delta
 * strip for scrolling pipes when geometry is unchanged.
 */
static void draw_pipe_delta(flappy_rect_t prev, flappy_rect_t cur) {
    if (cur.w <= 0 || cur.h <= 0) return;

    bool same_geom = (prev.w == cur.w) && (prev.h == cur.h) && (prev.y == cur.y);
    int dx = cur.x - prev.x;

    if (!same_geom || dx == 0 || dx >= cur.w || dx <= -cur.w) {
        clear_rect_safe(prev);
        draw_pipe(cur);
        return;
    }

    if (dx < 0) {
        /* Moving left: clear exposed right strip, draw new left strip. */
        int strip_w = -dx;
        int clear_x = prev.x + prev.w - strip_w;
        int draw_x  = cur.x;
        if (strip_w > 0) {
            fill_rect_clipped((int16_t)clear_x, cur.y, (int16_t)strip_w, cur.h, COLOR_BLACK);
            fill_rect_clipped((int16_t)draw_x,  cur.y, (int16_t)strip_w, cur.h, COLOR_GREEN);
        }
        return;
    }

    /* Moving right: clear exposed left strip, draw new right strip. */
    {
        int strip_w = dx;
        int clear_x = prev.x;
        int draw_x  = cur.x + cur.w - strip_w;
        fill_rect_clipped((int16_t)clear_x, cur.y, (int16_t)strip_w, cur.h, COLOR_BLACK);
        fill_rect_clipped((int16_t)draw_x,  cur.y, (int16_t)strip_w, cur.h, COLOR_GREEN);
    }
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

    if (!g->redraw_all && g->pipe_rect_count_prev == g->pipe_rect_count) {
        for (int i = 0; i < g->pipe_rect_count; i++) {
            draw_pipe_delta(g->pipe_draw_rects_prev[i], g->pipe_draw_rects[i]);
        }
    } else {
        if (!g->redraw_all) {
            for (int i = 0; i < g->pipe_rect_count_prev; i++) {
                clear_rect_safe(g->pipe_draw_rects_prev[i]);
            }
        }
        for (int i = 0; i < g->pipe_rect_count; i++) {
            draw_pipe(g->pipe_draw_rects[i]);
        }
    }

    fill_rect_clipped(g->bird_rect_now.x, g->bird_rect_now.y,
                      g->bird_rect_now.w, g->bird_rect_now.h, COLOR_YELLOW);

    if (g->redraw_all || g->score != score_prev) {
        draw_number(2, 2, (uint32_t)g->score, COLOR_WHITE, COLOR_BLACK, 1);
    }
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
    bool prev_button_held = false;
    uint32_t flap_cooldown_ms = 0;

    absolute_time_t next_tick = make_timeout_time_ms(GAME_DT_MS);

    while (true) {
        input_state_t in = input_read();
        bool button_edge = in.button_pressed || (in.button_held && !prev_button_held);
        bool start_pressed = in.button_pressed || in.button_held;
        bool flap_pressed = false;

        /* Accept held GP6 with cooldown so short/async presses are not missed. */
        if ((button_edge || in.button_held) && flap_cooldown_ms == 0) {
            flap_pressed = true;
            flap_cooldown_ms = FLAP_COOLDOWN_MS;
        }

        prev_button_held = in.button_held;
        audio_update();

        if (time_reached(next_tick)) {
            next_tick = delayed_by_ms(next_tick, GAME_DT_MS);
            if (flap_cooldown_ms >= GAME_DT_MS) flap_cooldown_ms -= GAME_DT_MS;
            else flap_cooldown_ms = 0;

            if (game.state == FLAPPY_WAITING) {
                /* Bird is suspended — update checks for first flap */
                flappy_game_update(&game, start_pressed, GAME_DT_MS / 1000.0f);

                /* In waiting, render once to avoid static-frame flicker. */
                if (game.redraw_all || last_state != FLAPPY_WAITING) {
                    render_playfield(&game, last_rendered_score);
                    game.redraw_all = false;
                }

            } else if (game.state == FLAPPY_PLAYING) {
                flappy_game_update(&game, flap_pressed, GAME_DT_MS / 1000.0f);

                /* Sound triggers */
                if (flap_pressed) {
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
                if (start_pressed || in.back_pressed) {
                    /* Return to main menu */
                    return;
                }
            }

            last_state = game.state;
        }

        tight_loop_contents();
    }
}
