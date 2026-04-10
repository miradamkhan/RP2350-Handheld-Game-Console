/*
 * game2048.h — 2048 game module for RP2350 console.
 */

#ifndef GAME2048_H
#define GAME2048_H

#include <stdbool.h>
#include <stdint.h>

#include "input.h"

typedef enum {
    GAME2048_STATE_MENU = 0,
    GAME2048_STATE_PLAYING,
    GAME2048_STATE_WIN,
    GAME2048_STATE_OVER
} game2048_state_t;

typedef struct {
    int board[4][4];
    uint32_t score;
    uint32_t best_score;
    game2048_state_t state;
    bool input_armed;     /* require stick return to center before next move */
    bool board_dirty;     /* request redraw */
    bool won_latched;     /* set once a 2048 tile is created */
} game2048_t;

void game2048_init(game2048_t *g, uint32_t loaded_best);
void game2048_reset(game2048_t *g);
void game2048_update(game2048_t *g, const input_state_t *in);
void game2048_render(const game2048_t *g);

/* Entry point used by main menu. */
void game2048_run(void);

#endif /* GAME2048_H */
