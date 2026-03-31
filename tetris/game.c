/*
 * game.c — Tetris game logic, fully separated from hardware.
 *
 * Provides: tetromino shape data, piece colours, game_init(), game_tick().
 * All rendering and I/O are handled externally.
 */

#include "game.h"
#include "pico/stdlib.h"   /* time_us_32 */

#include <string.h>

/* ── Tetromino shape tables (SRS-style) ──
 * Indexed as TETROMINO_SHAPES[type][rotation][row][col].
 * 1 = filled cell, 0 = empty, inside a 4×4 bounding box.              */

const uint8_t TETROMINO_SHAPES[NUM_TETROMINOES][NUM_ROTATIONS][TETRO_SIZE][TETRO_SIZE] = {
    /* PIECE_I */
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}
    },
    /* PIECE_O */
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
    },
    /* PIECE_T */
    {
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    /* PIECE_S */
    {
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
        {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    /* PIECE_Z */
    {
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}
    },
    /* PIECE_J */
    {
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}
    },
    /* PIECE_L */
    {
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}
    }
};

/* RGB565 colours per piece type (index matches piece_type_t). */
const uint16_t PIECE_COLORS[NUM_TETROMINOES] = {
    0x07FF,  /* I — cyan    */
    0xFFE0,  /* O — yellow  */
    0xA01F,  /* T — purple  */
    0x07E0,  /* S — green   */
    0xF800,  /* Z — red     */
    0x001F,  /* J — blue    */
    0xFD20   /* L — orange  */
};

/* ── Simple xorshift32 PRNG ── */

static uint32_t rng_state = 1;

static uint32_t xorshift32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static piece_type_t random_piece(void)
{
    return (piece_type_t)(xorshift32() % NUM_TETROMINOES);
}

/* ── Collision detection ── */

static bool check_collision(const game_t *g, piece_type_t type,
                            int8_t rot, int8_t x, int8_t y)
{
    for (int r = 0; r < TETRO_SIZE; r++) {
        for (int c = 0; c < TETRO_SIZE; c++) {
            if (!TETROMINO_SHAPES[type][rot][r][c]) continue;
            int bx = x + c;
            int by = y + r;
            if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return true;
            if (by >= 0 && g->board[by][bx] != 0)          return true;
        }
    }
    return false;
}

/* ── Piece helpers ── */

static void lock_piece(game_t *g)
{
    const piece_t *p = &g->current;
    for (int r = 0; r < TETRO_SIZE; r++) {
        for (int c = 0; c < TETRO_SIZE; c++) {
            if (!TETROMINO_SHAPES[p->type][p->rotation][r][c]) continue;
            int bx = p->x + c;
            int by = p->y + r;
            if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
                g->board[by][bx] = (uint8_t)(p->type + 1);
            }
        }
    }
}

static bool spawn_piece(game_t *g)
{
    g->current       = g->next;
    g->current.x     = (BOARD_W - TETRO_SIZE) / 2;  /* 3 */
    g->current.y     = 0;
    g->current.rotation = 0;

    g->next.type     = random_piece();
    g->next.rotation = 0;
    g->next.x        = 0;
    g->next.y        = 0;

    g->lock_counter  = 0;

    return !check_collision(g, g->current.type, g->current.rotation,
                            g->current.x, g->current.y);
}

/* ── Line clearing ── */

static uint8_t clear_lines(game_t *g)
{
    uint8_t cleared = 0;
    for (int r = BOARD_H - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < BOARD_W; c++) {
            if (g->board[r][c] == 0) { full = false; break; }
        }
        if (full) {
            cleared++;
            for (int rr = r; rr > 0; rr--) {
                memcpy(g->board[rr], g->board[rr - 1], BOARD_W);
            }
            memset(g->board[0], 0, BOARD_W);
            r++;  /* re-check this row index after shift */
        }
    }
    return cleared;
}

/* ── Scoring ── */

static uint16_t compute_gravity(uint8_t level)
{
    int val = 30 - (int)level * 3;
    return (uint16_t)(val < 2 ? 2 : val);
}

static void update_score(game_t *g, uint8_t lines)
{
    static const uint16_t pts[] = {0, 100, 300, 500, 800};
    uint8_t idx = lines > 4 ? 4 : lines;
    g->score += (uint32_t)pts[idx] * (g->level + 1);
    g->lines_cleared += lines;
    g->level = (uint8_t)(g->lines_cleared / 10);
    g->gravity_interval = compute_gravity(g->level);
    g->score_dirty = true;
    if (g->score > g->high_score) {
        g->high_score = g->score;
    }
}

/* ── Start a new game (transition from menu → playing) ── */

static void start_game(game_t *g)
{
    memset(g->board, 0, sizeof(g->board));
    g->score            = 0;
    g->lines_cleared    = 0;
    g->level            = 0;
    g->gravity_interval = 30;
    g->gravity_counter  = 0;
    g->lock_counter     = 0;
    g->state            = STATE_PLAYING;

    g->next.type     = random_piece();
    g->next.rotation = 0;
    spawn_piece(g);

    g->board_dirty  = true;
    g->score_dirty  = true;
}

/* ── Lock delay (ticks, ~33 ms each → ~330 ms) ── */
#define LOCK_DELAY  10

/* ── Public API ── */

void game_init(game_t *g, uint32_t high_score)
{
    memset(g, 0, sizeof(*g));
    g->state         = STATE_MENU;
    g->high_score    = high_score;
    g->state_changed = true;

    rng_state = time_us_32();
    if (rng_state == 0) rng_state = 1;
}

game_state_t game_tick(game_t *g, uint8_t actions)
{
    /* Clear per-tick event flags. */
    g->lines_cleared_now = 0;
    g->piece_locked_now  = false;
    g->rotated_now       = false;
    g->state_changed     = false;

    switch (g->state) {

    /* ── MENU ── */
    case STATE_MENU:
        if (actions & ACTION_SELECT) {
            start_game(g);
            g->state_changed = true;
        }
        break;

    /* ── PLAYING ── */
    case STATE_PLAYING: {
        /* Pause */
        if (actions & ACTION_PAUSE) {
            g->state = STATE_PAUSED;
            g->state_changed = true;
            break;
        }

        /* Horizontal movement */
        if (actions & ACTION_LEFT) {
            if (!check_collision(g, g->current.type, g->current.rotation,
                                 g->current.x - 1, g->current.y))
                g->current.x--;
        }
        if (actions & ACTION_RIGHT) {
            if (!check_collision(g, g->current.type, g->current.rotation,
                                 g->current.x + 1, g->current.y))
                g->current.x++;
        }

        /* Rotation */
        if (actions & ACTION_ROTATE) {
            int8_t nr = (g->current.rotation + 1) % NUM_ROTATIONS;
            if (!check_collision(g, g->current.type, nr,
                                 g->current.x, g->current.y)) {
                g->current.rotation = nr;
                g->rotated_now = true;
            }
        }

        /* Gravity */
        bool soft_drop = (actions & ACTION_DOWN) != 0;
        g->gravity_counter++;
        uint16_t interval = soft_drop ? 2 : g->gravity_interval;

        if (g->gravity_counter >= interval) {
            g->gravity_counter = 0;
            if (!check_collision(g, g->current.type, g->current.rotation,
                                 g->current.x, g->current.y + 1)) {
                g->current.y++;
                g->lock_counter = 0;
            }
        }

        /* Lock timer */
        bool grounded = check_collision(g, g->current.type,
                                        g->current.rotation,
                                        g->current.x, g->current.y + 1);
        if (grounded) {
            g->lock_counter++;
            if (g->lock_counter >= LOCK_DELAY) {
                lock_piece(g);
                g->piece_locked_now = true;
                g->lock_counter = 0;

                uint8_t lns = clear_lines(g);
                if (lns > 0) {
                    g->lines_cleared_now = lns;
                    update_score(g, lns);
                }
                g->board_dirty = true;

                if (!spawn_piece(g)) {
                    g->state = STATE_GAME_OVER;
                    g->state_changed = true;
                }
            }
        } else {
            g->lock_counter = 0;
        }

        g->board_dirty = true;  /* active piece may have moved */
        break;
    }

    /* ── PAUSED ── */
    case STATE_PAUSED:
        if (actions & (ACTION_PAUSE | ACTION_SELECT)) {
            g->state = STATE_PLAYING;
            g->state_changed = true;
        }
        break;

    /* ── GAME OVER ── */
    case STATE_GAME_OVER:
        if (actions & ACTION_SELECT) {
            game_init(g, g->high_score);
            g->state = STATE_MENU;
            g->state_changed = true;
        }
        break;
    }

    return g->state;
}
