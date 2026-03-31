/*
 * game.h — Tetris game logic, fully separated from hardware.
 *
 * The game module owns the playfield, active piece, scoring, and state
 * machine.  Rendering and input are handled externally; this module
 * exposes a tick-based API.
 */

#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

/* ── Constants ── */
#define BOARD_W         10
#define BOARD_H         20
#define NUM_TETROMINOES  7
#define NUM_ROTATIONS    4
#define TETRO_SIZE       4   /* bounding box is 4×4 */

/* ── Game states ── */
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} game_state_t;

/* ── Piece types ── */
typedef enum {
    PIECE_I = 0,
    PIECE_O,
    PIECE_T,
    PIECE_S,
    PIECE_Z,
    PIECE_J,
    PIECE_L
} piece_type_t;

/* ── Active piece ── */
typedef struct {
    piece_type_t type;
    int8_t       rotation;  /* 0..3 */
    int8_t       x;         /* column of top-left of 4×4 box */
    int8_t       y;         /* row   of top-left of 4×4 box  */
} piece_t;

/* ── Full game state ── */
typedef struct {
    game_state_t state;

    /* Playfield: 0 = empty, 1..7 = piece type that placed it (+1). */
    uint8_t  board[BOARD_H][BOARD_W];

    piece_t  current;
    piece_t  next;

    uint32_t score;
    uint32_t high_score;
    uint16_t lines_cleared;
    uint8_t  level;

    /* Gravity timing (in ticks). */
    uint16_t gravity_interval;
    uint16_t gravity_counter;

    /* Lock delay: piece sits on surface for a few ticks before locking. */
    uint8_t  lock_counter;

    /* Dirty flag so the renderer knows something changed. */
    bool     board_dirty;
    bool     score_dirty;
    bool     state_changed;

    /* Lines cleared this frame (for SFX). */
    uint8_t  lines_cleared_now;
    bool     piece_locked_now;
    bool     rotated_now;
} game_t;

/* ── Input actions the game understands ── */
typedef enum {
    ACTION_NONE    = 0,
    ACTION_LEFT    = (1 << 0),
    ACTION_RIGHT   = (1 << 1),
    ACTION_DOWN    = (1 << 2),   /* soft drop  */
    ACTION_ROTATE  = (1 << 3),
    ACTION_PAUSE   = (1 << 4),   /* toggle pause */
    ACTION_SELECT  = (1 << 5)    /* menu start / restart */
} game_action_t;

/* ── Tetromino shape data (shared read-only) ── */
extern const uint8_t TETROMINO_SHAPES[NUM_TETROMINOES][NUM_ROTATIONS][TETRO_SIZE][TETRO_SIZE];

/* ── Colour per piece type (RGB565 values) ── */
extern const uint16_t PIECE_COLORS[NUM_TETROMINOES];

/* ── API ── */

/* Reset all game state to defaults and pick initial pieces.
 * `high_score` is loaded from EEPROM beforehand and passed in. */
void game_init(game_t *g, uint32_t high_score);

/* Advance one game tick.  `actions` is a bitmask of game_action_t.
 * Returns the current game state after processing. */
game_state_t game_tick(game_t *g, uint8_t actions);

#endif /* GAME_H */
