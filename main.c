#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "audio.h"
#include "display.h"
#include "eeprom.h"
#include "game.h"
#include "input.h"
#include "pins.h"

#define GAME_DT_MS                 16
#define HIGH_SCORE_EEPROM_ADDR     0x0000
#define RUN_HW_TESTS               0

static void run_hardware_tests(void) {
    // Run each peripheral test independently before game starts.
    display_test();
    sleep_ms(500);
    audio_test();
    eeprom_test();
    // NOTE: input_test() is continuous and blocks by design.
    // Uncomment for dedicated joystick bring-up:
    // input_test();
}

int main(void) {
    stdio_init_all();

    gpio_init(DEBUG_LED_PIN);
    gpio_set_dir(DEBUG_LED_PIN, GPIO_OUT);
    gpio_put(DEBUG_LED_PIN, 1);

    display_init();
    input_init();
    audio_init();
    eeprom_init();

#if RUN_HW_TESTS
    run_hardware_tests();
#endif

    uint16_t high_score_raw = 0;
    if (!eeprom_read_u16(HIGH_SCORE_EEPROM_ADDR, &high_score_raw)) {
        high_score_raw = 0;
    }

    Game game;
    game_init(&game, (int)high_score_raw);

    InputState input = {0};
    absolute_time_t next_tick = make_timeout_time_ms(GAME_DT_MS);
    GameState last_state = GAME_OVER;
    int last_rendered_score = -1;

    while (true) {
        input_update(&input);
        audio_update();

        while (time_reached(next_tick)) {
            next_tick = delayed_by_ms(next_tick, GAME_DT_MS);
            game_update(&game, &input, GAME_DT_MS / 1000.0f);

            // Sound trigger handling by state/score transitions
            if (game.state == GAME_PLAYING && input.button_just_pressed) {
                audio_play_jump();
            }
            if (game.state == GAME_PLAYING && game.score != game.prev_score) {
                if (game.prev_score >= 0 && game.score > game.prev_score) {
                    audio_play_score();
                }
                game.prev_score = game.score;
            }
            if (last_state == GAME_PLAYING && game.state == GAME_OVER) {
                audio_play_collision();
            }

            if (game.state == GAME_OVER && game.score > game.high_score) {
                game.high_score = game.score;
                (void)eeprom_write_u16(HIGH_SCORE_EEPROM_ADDR, (uint16_t)game.high_score);
            }

            // Render by game state
            if (game.state == GAME_MENU && last_state != GAME_MENU) {
                display_render_menu(game.high_score);
            } else if (game.state == GAME_PLAYING) {
                if (last_state != GAME_PLAYING) {
                    last_rendered_score = -1;
                }
                display_render_playfield(
                    game.pipe_draw_rects,
                    game.pipe_rect_count,
                    game.bird_rect_now,
                    game.bird_rect_prev,
                    game.redraw_all,
                    game.score,
                    last_rendered_score
                );
                game.redraw_all = false;
                last_rendered_score = game.score;
            } else { // GAME_OVER
                if (last_state != GAME_OVER) {
                    display_render_game_over(game.score, game.high_score);
                }
            }

            last_state = game.state;
        }

        tight_loop_contents();
    }

    return 0;
}
