/*
 * main.c — RP2350 Handheld Game Console main menu.
 *
 * Displays a game selection menu on boot.  The user scrolls with the
 * joystick (up/down) and presses the button to launch a game.
 * When the game returns, the menu is re-displayed.
 */

#include "pico/stdlib.h"

#include "pins.h"
#include "display.h"
#include "input.h"
#include "audio.h"
#include "eeprom.h"
#include "tetris_render.h"
#include "flappy_render.h"

#include <stdio.h>

/* ── Menu configuration ── */

#define NUM_GAMES       2
#define MENU_FRAME_MS  50

static const char* game_names[NUM_GAMES] = {
    "TETRIS",
    "FLAPPY BIRD"
};

/* ── Menu rendering ── */

static void draw_menu(int selected)
{
    display_fill(COLOR_BLACK);

    /* Title */
    display_string(30, 30, "SELECT GAME", COLOR_CYAN, COLOR_BLACK, 2);

    /* Decorative line under title */
    display_fill_rect(20, 52, 200, 2, COLOR_CYAN);

    /* Menu items */
    for (int i = 0; i < NUM_GAMES; i++) {
        uint16_t y = (uint16_t)(100 + i * 60);
        uint16_t box_x = 20;
        uint16_t box_w = 200;
        uint16_t box_h = 40;

        if (i == selected) {
            /* Highlighted item: filled box with bright text */
            display_fill_rect(box_x, y, box_w, box_h, COLOR_CYAN);
            display_string(box_x + 12, y + 12, game_names[i],
                           COLOR_BLACK, COLOR_CYAN, 2);

            /* Selection arrow */
            display_string(box_x - 14, y + 12, ">", COLOR_YELLOW, COLOR_BLACK, 2);
        } else {
            /* Normal item: outlined box with dim text */
            /* Top border */
            display_fill_rect(box_x, y, box_w, 2, COLOR_GRAY);
            /* Bottom border */
            display_fill_rect(box_x, y + box_h - 2, box_w, 2, COLOR_GRAY);
            /* Left border */
            display_fill_rect(box_x, y, 2, box_h, COLOR_GRAY);
            /* Right border */
            display_fill_rect(box_x + box_w - 2, y, 2, box_h, COLOR_GRAY);

            display_string(box_x + 12, y + 12, game_names[i],
                           COLOR_WHITE, COLOR_BLACK, 2);
        }
    }

    /* Footer hint */
    display_string(16, 280, "UP/DOWN: Select", COLOR_DARK_GRAY, COLOR_BLACK, 1);
    display_string(16, 294, "BUTTON:  Start", COLOR_DARK_GRAY, COLOR_BLACK, 1);
}

/* ── Main ── */

int main(void)
{
    stdio_init_all();

    gpio_init(PIN_DEBUG_LED);
    gpio_set_dir(PIN_DEBUG_LED, GPIO_OUT);
    gpio_put(PIN_DEBUG_LED, 1);

    /* Initialise hardware drivers. */
    display_init();
    input_init();
    audio_init();
    eeprom_init();

    int selected = 0;
    joy_dir_t prev_dir = JOY_NONE;

    draw_menu(selected);

    while (true) {
        uint32_t frame_start = to_ms_since_boot(get_absolute_time());

        input_state_t in = input_read();
        audio_update();

        /* Joystick navigation (edge-triggered) */
        if (in.direction == JOY_UP && prev_dir != JOY_UP) {
            if (selected > 0) {
                selected--;
                audio_play_sfx(SFX_ROTATE);
                draw_menu(selected);
            }
        }
        if (in.direction == JOY_DOWN && prev_dir != JOY_DOWN) {
            if (selected < NUM_GAMES - 1) {
                selected++;
                audio_play_sfx(SFX_ROTATE);
                draw_menu(selected);
            }
        }
        prev_dir = in.direction;

        /* Button selects the game */
        if (in.button_pressed) {
            audio_play_sfx(SFX_MENU_SELECT);
            sleep_ms(120); /* brief pause so the SFX is audible */

            switch (selected) {
            case 0:
                tetris_run();
                break;
            case 1:
                flappy_run();
                break;
            }

            /* Redraw menu when game returns */
            prev_dir = JOY_NONE;
            draw_menu(selected);
        }

        /* Frame rate limiter */
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - frame_start;
        if (elapsed < MENU_FRAME_MS) {
            sleep_ms(MENU_FRAME_MS - elapsed);
        }
    }

    return 0;
}
