#include "input.h"

#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "pins.h"

#define JOY_CENTER_COUNTS       2048.0f
#define DEBOUNCE_MS             20

static bool last_raw_button = false;
static bool stable_button = false;
static absolute_time_t last_change_time;

static uint16_t read_adc_channel(uint8_t ch) {
    adc_select_input(ch);
    return adc_read();
}

static float normalize_adc(uint16_t raw) {
    float norm = ((float)raw - JOY_CENTER_COUNTS) / JOY_CENTER_COUNTS;
    if (norm > 1.0f) norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;

    if (norm > -JOYSTICK_DEADZONE && norm < JOYSTICK_DEADZONE) {
        norm = 0.0f;
    }
    return norm;
}

void input_init(void) {
    adc_init();
    adc_gpio_init(JOY_ADC_X_PIN);
    adc_gpio_init(JOY_ADC_Y_PIN);

    gpio_init(JOY_BTN_PIN);
    gpio_set_dir(JOY_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOY_BTN_PIN);

    last_raw_button = false;
    stable_button = false;
    last_change_time = get_absolute_time();
}

void input_update(InputState* s) {
    s->raw_x = read_adc_channel(JOY_ADC_X_CH);
    s->raw_y = read_adc_channel(JOY_ADC_Y_CH);
    s->norm_x = normalize_adc(s->raw_x);
    s->norm_y = normalize_adc(s->raw_y);

    bool raw_pressed = (gpio_get(JOY_BTN_PIN) == 0); // active-low
    if (raw_pressed != last_raw_button) {
        last_raw_button = raw_pressed;
        last_change_time = get_absolute_time();
    }

    if (absolute_time_diff_us(last_change_time, get_absolute_time()) >= (DEBOUNCE_MS * 1000)) {
        stable_button = raw_pressed;
    }

    static bool prev_stable = false;
    s->button_pressed = stable_button;
    s->button_just_pressed = (stable_button && !prev_stable);
    prev_stable = stable_button;
}

void input_test(void) {
    InputState s = {0};
    absolute_time_t next_print = get_absolute_time();
    while (true) {
        input_update(&s);
        if (time_reached(next_print)) {
            next_print = delayed_by_ms(next_print, 100);
            printf("JOY raw(x,y)=(%4u,%4u) norm=(%+.2f,%+.2f) btn=%d edge=%d\n",
                   s.raw_x, s.raw_y, s.norm_x, s.norm_y,
                   s.button_pressed ? 1 : 0,
                   s.button_just_pressed ? 1 : 0);
        }
        tight_loop_contents();
    }
}
