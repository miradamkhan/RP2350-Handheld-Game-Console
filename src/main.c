/*
 * main.c — RP2350 Handheld Game Console main menu.
 *
 * Displays a game selection menu on boot. The user scrolls with the
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
#include "game2048.h"
#include "invaders.h"

#include <stdio.h>

#define NUM_GAMES      3
#define MENU_FRAME_MS 50

static const char *game_names[NUM_GAMES] = {
    "TETRIS",
    "2048",
    "SPACE INVADERS"
};

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

static void show_boot_splash(void)
{
    display_fill(COLOR_WHITE);

    display_string(10, 26, "AbdulMalik Busari, Enio Hysa,", COLOR_BLACK, COLOR_WHITE, 1);
    display_string(12, 40, "Mir Adam Khan, Dixon Wagner", COLOR_BLACK, COLOR_WHITE, 1);

    /* Approximate the GAME BOY 362 startup card styling. */
    display_string(34, 88,  "GAME", COLOR_BLUE,    COLOR_WHITE, 4);
    display_string(130, 88, "BOY",  COLOR_GREEN,   COLOR_WHITE, 4);

    display_fill_rect(74, 168, 92, 100, COLOR_DARK_GRAY);
    display_fill_rect(72, 166, 96, 104, COLOR_ORANGE);
    display_fill_rect(76, 170, 88, 96, COLOR_DARK_GRAY);
    display_string(90, 208, "362", COLOR_WHITE, COLOR_DARK_GRAY, 3);

    sleep_ms(2200);
}

static void draw_menu(int selected)
{
    uint32_t hs_tetris   = eeprom_read_high_score(EEPROM_ADDR_TETRIS_HS);
    uint32_t hs_2048     = eeprom_read_high_score(EEPROM_ADDR_2048_HS);
    uint32_t hs_invaders = eeprom_read_high_score(EEPROM_ADDR_INVADERS_HS);

    display_fill(COLOR_BLACK);

    display_string(30, 30, "SELECT GAME", COLOR_CYAN, COLOR_BLACK, 2);
    display_fill_rect(20, 52, 200, 2, COLOR_CYAN);

    for (int i = 0; i < NUM_GAMES; i++) {
        uint16_t y = (uint16_t)(80 + i * 52);
        uint16_t box_x = 20;
        uint16_t box_w = 200;
        uint16_t box_h = 40;

        if (i == selected) {
            display_fill_rect(box_x, y, box_w, box_h, COLOR_CYAN);
            display_string(box_x + 10, y + 12, game_names[i],
                           COLOR_BLACK, COLOR_CYAN, 2);
            display_string(box_x - 14, y + 12, ">", COLOR_YELLOW, COLOR_BLACK, 2);
        } else {
            display_fill_rect(box_x, y, box_w, 2, COLOR_GRAY);
            display_fill_rect(box_x, y + box_h - 2, box_w, 2, COLOR_GRAY);
            display_fill_rect(box_x, y, 2, box_h, COLOR_GRAY);
            display_fill_rect(box_x + box_w - 2, y, 2, box_h, COLOR_GRAY);
            display_string(box_x + 10, y + 12, game_names[i],
                           COLOR_WHITE, COLOR_BLACK, 2);
        }
    }

    /* Leaderboard card */
    display_fill_rect(12, 232, 216, 70, COLOR_DARK_GRAY);
    display_fill_rect(14, 234, 212, 66, COLOR_BLACK);
    display_string(66, 238, "LEADERBOARD", COLOR_CYAN, COLOR_BLACK, 1);

    display_string(20, 252, "TETRIS", COLOR_GRAY, COLOR_BLACK, 1);
    draw_number(164, 252, hs_tetris, COLOR_WHITE, COLOR_BLACK, 1);

    display_string(20, 266, "2048", COLOR_GRAY, COLOR_BLACK, 1);
    draw_number(164, 266, hs_2048, COLOR_WHITE, COLOR_BLACK, 1);

    display_string(20, 280, "INVADERS", COLOR_GRAY, COLOR_BLACK, 1);
    draw_number(164, 280, hs_invaders, COLOR_WHITE, COLOR_BLACK, 1);

    display_string(16, 306, "UP/DOWN + BUTTON", COLOR_DARK_GRAY, COLOR_BLACK, 1);
}

int main(void)
{
    stdio_init_all();

    gpio_init(PIN_DEBUG_LED);
    gpio_set_dir(PIN_DEBUG_LED, GPIO_OUT);
    gpio_put(PIN_DEBUG_LED, 1);

    display_init();
    show_boot_splash();
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

        if (in.direction == JOY_UP && prev_dir != JOY_UP) {
            if (selected > 0) {
                selected--;
                audio_play_tone(280, 10); /* subtle menu hover */
                draw_menu(selected);
            }
        }
        if (in.direction == JOY_DOWN && prev_dir != JOY_DOWN) {
            if (selected < NUM_GAMES - 1) {
                selected++;
                audio_play_tone(280, 10); /* subtle menu hover */
                draw_menu(selected);
            }
        }
        prev_dir = in.direction;

        if (in.button_pressed) {
            audio_play_sfx(SFX_MENU_SELECT);
            sleep_ms(120);

            switch (selected) {
            case 0:
                tetris_run();
                break;
            case 1:
                game2048_run();
                break;
            case 2:
                invaders_run();
                break;
            }

            prev_dir = JOY_NONE;
            draw_menu(selected);
        }

        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - frame_start;
        if (elapsed < MENU_FRAME_MS) {
            sleep_ms(MENU_FRAME_MS - elapsed);
        }
    }

    return 0;
}
