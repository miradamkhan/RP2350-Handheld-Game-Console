/*
 * tetris_render.c — Tetris rendering and game loop.
 *
 * Extracted from the original tetris/main.c.
 * tetris_run() contains the complete game loop; it returns when the
 * player exits back to the console main menu (game over -> button).
 */

#include "tetris_render.h"
#include "tetris_game.h"
#include "display.h"
#include "input.h"
#include "audio.h"
#include "eeprom.h"
#include "pins.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

/* ── Layout constants (pixels) ── */

#define BLOCK_SZ     12
#define BOARD_PX_X    8                           /* board left edge      */
#define BOARD_PX_Y   20                           /* board top edge       */
#define INFO_X      138                           /* info-panel left edge */
#define PREVIEW_SZ    8                           /* next-piece block px  */

/* ── Timing ── */

#define FRAME_MS        33   /* ~30 FPS */
#define DAS_DELAY_MS   170   /* auto-repeat initial delay */
#define DAS_REPEAT_MS   50   /* auto-repeat interval      */

/* ── Auto-repeat state ── */

static joy_dir_t s_prev_dir     = JOY_NONE;
static uint32_t  s_dir_start_ms = 0;
static uint32_t  s_dir_last_ms  = 0;

/* ────────────────────────── helpers ───────────────────────────────── */

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

/* ────────────────────────── input mapping ─────────────────────────── */

static uint8_t map_input(const input_state_t *in, tetris_state_t state)
{
    uint8_t actions = TETRIS_ACTION_NONE;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    joy_dir_t dir = in->direction;

    /* Left / right with DAS auto-repeat */
    if (dir == JOY_LEFT || dir == JOY_RIGHT) {
        bool fire = false;
        if (dir != s_prev_dir) {
            fire = true;
            s_dir_start_ms = now;
            s_dir_last_ms  = now;
        } else if ((now - s_dir_start_ms) >= DAS_DELAY_MS) {
            if ((now - s_dir_last_ms) >= DAS_REPEAT_MS) {
                fire = true;
                s_dir_last_ms = now;
            }
        }
        if (fire) {
            actions |= (dir == JOY_LEFT) ? TETRIS_ACTION_LEFT : TETRIS_ACTION_RIGHT;
        }
    }

    /* Soft drop — active every tick while held */
    if (dir == JOY_DOWN) {
        actions |= TETRIS_ACTION_DOWN;
    }

    /* Pause on joystick-up edge */
    if (dir == JOY_UP && s_prev_dir != JOY_UP) {
        actions |= TETRIS_ACTION_PAUSE;
    }

    s_prev_dir = dir;

    /* Button: context-dependent */
    if (in->button_pressed) {
        switch (state) {
        case TETRIS_STATE_MENU:      actions |= TETRIS_ACTION_SELECT; break;
        case TETRIS_STATE_PLAYING:   actions |= TETRIS_ACTION_ROTATE; break;
        case TETRIS_STATE_PAUSED:    actions |= TETRIS_ACTION_SELECT; break;
        case TETRIS_STATE_GAME_OVER: actions |= TETRIS_ACTION_SELECT; break;
        }
    }

    return actions;
}

/* ────────────────────────── rendering ─────────────────────────────── */

static void draw_board(const tetris_game_t *g)
{
    for (int r = 0; r < BOARD_H; r++) {
        for (int c = 0; c < BOARD_W; c++) {
            uint16_t color = COLOR_BLACK;
            uint8_t cell = g->board[r][c];
            if (cell != 0) {
                color = PIECE_COLORS[cell - 1];
            }

            /* Overlay active-piece cells */
            int pr = r - g->current.y;
            int pc = c - g->current.x;
            if (pr >= 0 && pr < TETRO_SIZE && pc >= 0 && pc < TETRO_SIZE) {
                if (TETROMINO_SHAPES[g->current.type]
                                    [g->current.rotation][pr][pc]) {
                    color = PIECE_COLORS[g->current.type];
                }
            }

            uint16_t px = BOARD_PX_X + (uint16_t)c * BLOCK_SZ;
            uint16_t py = BOARD_PX_Y + (uint16_t)r * BLOCK_SZ;
            display_fill_rect(px, py, BLOCK_SZ - 1, BLOCK_SZ - 1, color);
        }
    }
}

static void draw_board_border(void)
{
    uint16_t bw = (uint16_t)(BOARD_W * BLOCK_SZ);
    uint16_t bh = (uint16_t)(BOARD_H * BLOCK_SZ);
    uint16_t x0 = BOARD_PX_X - 1;
    uint16_t y0 = BOARD_PX_Y - 1;
    display_fill_rect(x0, y0, bw + 2, 1, COLOR_WHITE);         /* top    */
    display_fill_rect(x0, y0 + bh + 1, bw + 2, 1, COLOR_WHITE);/* bottom */
    display_fill_rect(x0, y0, 1, bh + 2, COLOR_WHITE);         /* left   */
    display_fill_rect(x0 + bw + 1, y0, 1, bh + 2, COLOR_WHITE);/* right  */
}

static void draw_info_panel(const tetris_game_t *g)
{
    uint16_t y = BOARD_PX_Y;
    uint16_t clr_w = 96;  /* width to clear for number fields */

    display_string(INFO_X, y, "SCORE", COLOR_GRAY, COLOR_BLACK, 1);
    y += 10;
    display_fill_rect(INFO_X, y, clr_w, 9, COLOR_BLACK);
    draw_number(INFO_X, y, g->score, COLOR_WHITE, COLOR_BLACK, 1);

    y += 18;
    display_string(INFO_X, y, "HIGH", COLOR_GRAY, COLOR_BLACK, 1);
    y += 10;
    display_fill_rect(INFO_X, y, clr_w, 9, COLOR_BLACK);
    draw_number(INFO_X, y, g->high_score, COLOR_YELLOW, COLOR_BLACK, 1);

    y += 18;
    display_string(INFO_X, y, "LEVEL", COLOR_GRAY, COLOR_BLACK, 1);
    y += 10;
    display_fill_rect(INFO_X, y, clr_w, 9, COLOR_BLACK);
    draw_number(INFO_X, y, (uint32_t)(g->level + 1), COLOR_GREEN, COLOR_BLACK, 1);

    y += 18;
    display_string(INFO_X, y, "LINES", COLOR_GRAY, COLOR_BLACK, 1);
    y += 10;
    display_fill_rect(INFO_X, y, clr_w, 9, COLOR_BLACK);
    draw_number(INFO_X, y, (uint32_t)g->lines_cleared, COLOR_CYAN, COLOR_BLACK, 1);

    /* Next piece preview */
    y += 20;
    display_string(INFO_X, y, "NEXT", COLOR_GRAY, COLOR_BLACK, 1);
    y += 10;
    uint16_t px0 = INFO_X;
    /* Clear preview area */
    display_fill_rect(px0, y, TETRO_SIZE * PREVIEW_SZ,
                      TETRO_SIZE * PREVIEW_SZ, COLOR_BLACK);
    for (int r = 0; r < TETRO_SIZE; r++) {
        for (int c = 0; c < TETRO_SIZE; c++) {
            if (TETROMINO_SHAPES[g->next.type][0][r][c]) {
                display_fill_rect(px0 + (uint16_t)c * PREVIEW_SZ,
                                  y   + (uint16_t)r * PREVIEW_SZ,
                                  PREVIEW_SZ - 1, PREVIEW_SZ - 1,
                                  PIECE_COLORS[g->next.type]);
            }
        }
    }
}



static void draw_playing_full(const tetris_game_t *g)
{
    display_fill(COLOR_BLACK);
    draw_board_border();
    draw_board(g);
    draw_info_panel(g);
}

static void draw_paused(void)
{
    display_fill_rect(30, 120, 180, 60, COLOR_BLACK);
    display_fill_rect(29, 119, 182, 62, COLOR_WHITE);
    display_fill_rect(30, 120, 180, 60, COLOR_BLACK);
    display_string(54, 140, "PAUSED", COLOR_WHITE, COLOR_BLACK, 3);
}

static void draw_game_over(const tetris_game_t *g)
{
    display_fill(COLOR_BLACK);
    display_string(15, 50, "GAME OVER", COLOR_RED, COLOR_BLACK, 3);

    display_string(24, 130, "SCORE", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(24, 158, g->score, COLOR_WHITE, COLOR_BLACK, 2);

    display_string(24, 200, "HIGH", COLOR_GRAY, COLOR_BLACK, 2);
    draw_number(24, 228, g->high_score, COLOR_YELLOW, COLOR_BLACK, 2);

    display_string(18, 290, "Press Button", COLOR_GRAY, COLOR_BLACK, 1);
}

/* ────────────────────────── audio events ──────────────────────────── */

static void handle_audio(const tetris_game_t *g)
{
    if (g->state_changed && g->state == TETRIS_STATE_GAME_OVER) {
        audio_play_sfx(SFX_GAME_OVER);
        return;
    }
    if (g->state_changed && g->state == TETRIS_STATE_PLAYING) {
        audio_play_sfx(SFX_MENU_SELECT);
        return;
    }
    if (g->lines_cleared_now > 0) {
        audio_play_sfx(SFX_LINE_CLEAR);
        return;
    }
    if (g->piece_locked_now) {
        audio_play_sfx(SFX_LOCK);
        return;
    }
    if (g->rotated_now) {
        audio_play_sfx(SFX_ROTATE);
    }
}

/* ────────────────────────── tetris_run ─────────────────────────────── */

void tetris_run(void)
{
    /* Reset auto-repeat state */
    s_prev_dir     = JOY_NONE;
    s_dir_start_ms = 0;
    s_dir_last_ms  = 0;

    /* Load high score from EEPROM. */
    uint32_t high = eeprom_read_high_score(EEPROM_ADDR_TETRIS_HS);

    /* Initialise game state — start directly in playing, skip tetris menu */
    tetris_game_t g;
    tetris_game_init(&g, high);

    /* Skip the internal menu — go straight to playing */
    tetris_game_tick(&g, TETRIS_ACTION_SELECT);

    /* Draw initial playing screen */
    draw_playing_full(&g);

    /* ── Main loop ── */
    while (true) {
        uint32_t frame_start = to_ms_since_boot(get_absolute_time());

        /* 1. Read input */
        input_state_t in = input_read();
        uint8_t actions = map_input(&in, g.state);

        /* 2. Advance game logic */
        tetris_state_t cur = tetris_game_tick(&g, actions);

        /* 3. If game returned to MENU state, exit back to console menu */
        if (cur == TETRIS_STATE_MENU) {
            /* Save high score before exiting */
            eeprom_write_high_score(EEPROM_ADDR_TETRIS_HS, g.high_score);
            return;  /* Back to main menu */
        }

        /* 4. Render based on state transitions and dirty flags */
        if (g.state_changed) {
            switch (cur) {
            case TETRIS_STATE_MENU:      break; /* handled above */
            case TETRIS_STATE_PLAYING:   draw_playing_full(&g);   break;
            case TETRIS_STATE_PAUSED:    draw_paused();           break;
            case TETRIS_STATE_GAME_OVER:
                eeprom_write_high_score(EEPROM_ADDR_TETRIS_HS, g.score);
                draw_game_over(&g);
                break;
            }
        } else if (cur == TETRIS_STATE_PLAYING) {
            if (g.board_dirty)  draw_board(&g);
            if (g.score_dirty)  draw_info_panel(&g);
        }

        /* Clear dirty flags after rendering. */
        g.board_dirty = false;
        g.score_dirty = false;

        /* 5. Audio */
        handle_audio(&g);
        audio_update();

        /* 6. Frame-rate limiter */
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - frame_start;
        if (elapsed < FRAME_MS) {
            sleep_ms(FRAME_MS - elapsed);
        }
    }
}
