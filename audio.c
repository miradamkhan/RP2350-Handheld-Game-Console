#include "audio.h"

#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "pins.h"

typedef struct {
    bool active;
    absolute_time_t stop_time;
    uint slice;
    uint channel;
} ToneState;

static ToneState g_tone;

void audio_init(void) {
    gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);
    g_tone.slice = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);
    g_tone.channel = pwm_gpio_to_channel(AUDIO_PWM_PIN);
    pwm_set_enabled(g_tone.slice, false);
    g_tone.active = false;
}

void audio_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0 || duration_ms == 0) return;

    uint32_t clk_hz = clock_get_hz(clk_sys);
    uint16_t divider = 4; // keeps wrap in range for audible frequencies
    uint32_t top = (clk_hz / divider / freq_hz) - 1;
    if (top > 65535) top = 65535;
    if (top < 10) top = 10;

    pwm_set_clkdiv_int_frac(g_tone.slice, divider, 0);
    pwm_set_wrap(g_tone.slice, (uint16_t)top);
    pwm_set_chan_level(g_tone.slice, g_tone.channel, (uint16_t)(top / 2)); // 50% duty
    pwm_set_enabled(g_tone.slice, true);

    g_tone.active = true;
    g_tone.stop_time = delayed_by_ms(get_absolute_time(), duration_ms);
}

void audio_update(void) {
    if (g_tone.active && absolute_time_diff_us(get_absolute_time(), g_tone.stop_time) <= 0) {
        pwm_set_enabled(g_tone.slice, false);
        g_tone.active = false;
    }
}

bool audio_is_busy(void) {
    return g_tone.active;
}

void audio_play_jump(void) {
    audio_play_tone(1200, 70);
}

void audio_play_score(void) {
    audio_play_tone(1800, 60);
}

void audio_play_collision(void) {
    audio_play_tone(250, 220);
}

void audio_test(void) {
    printf("Audio test: 3 tones\n");
    audio_play_tone(600, 120);
    while (audio_is_busy()) audio_update();
    sleep_ms(80);

    audio_play_tone(1000, 120);
    while (audio_is_busy()) audio_update();
    sleep_ms(80);

    audio_play_tone(1400, 120);
    while (audio_is_busy()) audio_update();
}
