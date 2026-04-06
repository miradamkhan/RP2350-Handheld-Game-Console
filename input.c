/*
 * input.c — Adafruit analog joystick #512 driver.
 *
 * X/Y are read via ADC (12-bit, 0-4095).  The select button is
 * active-low with an internal pull-up.  Deadzone and debounce are
 * handled here so callers get clean, stable input.
 */

#include "input.h"
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include <stdio.h>
#include <stdlib.h>

/* ── Configuration ── */

/* ADC midpoint (12-bit). */
#define ADC_MID           2048

/* Threshold above/below midpoint to register a direction. */
#define DEADZONE          400

/* Button debounce: ignore presses closer than this (ms). */
#define DEBOUNCE_MS       150

/* ── Internal state ── */

static bool     prev_btn_state  = false;
static uint32_t last_press_time = 0;

/* ── Helpers ── */

static uint16_t read_adc_channel(uint8_t channel)
{
    adc_select_input(channel);
    return adc_read();
}

/* ── Public API ── */

void input_init(void)
{
    adc_init();

    adc_gpio_init(PIN_JOY_X);  /* GP45 */
    adc_gpio_init(PIN_JOY_Y);  /* GP44 */

    gpio_init(PIN_JOY_BTN);
    gpio_set_dir(PIN_JOY_BTN, GPIO_IN);
    gpio_pull_up(PIN_JOY_BTN);
}

input_state_t input_read(void)
{
    input_state_t st;

    /* Read raw ADC values. */
    st.raw_x = (int16_t)read_adc_channel(ADC_CHAN_X);
    st.raw_y = (int16_t)read_adc_channel(ADC_CHAN_Y);

    /* Determine dominant direction using deadzone. */
    int16_t dx = st.raw_x - ADC_MID;
    int16_t dy = st.raw_y - ADC_MID;

    st.direction = JOY_NONE;

    /* Pick the larger-magnitude axis if beyond deadzone. */
    if (abs(dx) > DEADZONE || abs(dy) > DEADZONE) {
        if (abs(dx) >= abs(dy)) {
            st.direction = (dx > 0) ? JOY_RIGHT : JOY_LEFT;
        } else {
            st.direction = (dy > 0) ? JOY_DOWN : JOY_UP;
        }
    }

    /* Button: active-low, debounced edge. */
    bool btn_raw = !gpio_get(PIN_JOY_BTN);
    uint32_t now = to_ms_since_boot(get_absolute_time());

    st.button_held = btn_raw;
    st.button_pressed = false;

    if (btn_raw && !prev_btn_state) {
        if ((now - last_press_time) >= DEBOUNCE_MS) {
            st.button_pressed = true;
            last_press_time = now;
        }
    }
    prev_btn_state = btn_raw;

    return st;
}

void input_test(void)
{
    printf("=== Joystick Test (5 seconds) ===\n");
    uint32_t end = to_ms_since_boot(get_absolute_time()) + 5000;
    while (to_ms_since_boot(get_absolute_time()) < end) {
        input_state_t s = input_read();
        printf("X=%4d  Y=%4d  dir=%d  btn=%d/%d\n",
               s.raw_x, s.raw_y, s.direction,
               s.button_pressed, s.button_held);
        sleep_ms(100);
    }
    printf("=== Joystick Test Done ===\n");
}
