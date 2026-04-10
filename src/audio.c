/*
 * audio.c — Non-blocking PWM audio for sound effects.
 *
 * A single tone plays at a time.  audio_play_tone() sets up the PWM
 * frequency and records the end time; audio_update() silences it when
 * the duration expires.  Sound effects are just canned
 * frequency/duration pairs.
 */

#include "audio.h"
#include "pins.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include <stdio.h>

/* ── Internal state ── */

static uint     pwm_slice    = 0;
static uint     pwm_channel  = 0;
static uint32_t tone_end_ms  = 0;
static bool     tone_active  = false;

typedef struct {
    uint16_t freq_hz;
    uint16_t duration_ms;
} tone_step_t;

static const tone_step_t *s_seq = NULL;
static uint8_t  s_seq_len = 0;
static uint8_t  s_seq_idx = 0;
static bool     s_seq_active = false;

/* ── Helpers ── */

static void speaker_off(void)
{
    pwm_set_enabled(pwm_slice, false);
    tone_active = false;
}

static void start_tone_now(uint16_t freq_hz, uint16_t duration_ms)
{
    if (freq_hz == 0 || duration_ms == 0) {
        speaker_off();
        return;
    }

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t wrap = sys_clk / freq_hz;
    uint16_t divider_int = 1;
    while (wrap > 65535 && divider_int < 256) {
        divider_int++;
        wrap = sys_clk / (freq_hz * divider_int);
    }
    if (wrap < 2) wrap = 2;

    pwm_set_clkdiv_int_frac(pwm_slice, divider_int, 0);
    pwm_set_wrap(pwm_slice, (uint16_t)(wrap - 1));
    pwm_set_chan_level(pwm_slice, pwm_channel, (uint16_t)(wrap / 2));
    pwm_set_enabled(pwm_slice, true);

    tone_end_ms = to_ms_since_boot(get_absolute_time()) + duration_ms;
    tone_active = true;
}

static void play_sequence(const tone_step_t *seq, uint8_t len)
{
    if (seq == NULL || len == 0) {
        s_seq_active = false;
        speaker_off();
        return;
    }
    s_seq = seq;
    s_seq_len = len;
    s_seq_idx = 0;
    s_seq_active = true;
    start_tone_now(s_seq[0].freq_hz, s_seq[0].duration_ms);
}

/* ── Public API ── */

void audio_init(void)
{
    gpio_set_function(PIN_SPEAKER, GPIO_FUNC_PWM);
    pwm_slice   = pwm_gpio_to_slice_num(PIN_SPEAKER);
    pwm_channel = pwm_gpio_to_channel(PIN_SPEAKER);

    pwm_set_enabled(pwm_slice, false);
}

void audio_play_tone(uint16_t freq_hz, uint16_t duration_ms)
{
    s_seq_active = false; /* one-shot tone overrides sequence */
    start_tone_now(freq_hz, duration_ms);
}

void audio_play_sfx(sfx_id_t sfx)
{
    static const tone_step_t seq_rotate[] = {
        {760, 22}, {980, 22}
    };
    static const tone_step_t seq_line_clear[] = {
        {900, 38}, {1160, 38}, {1460, 46}
    };
    static const tone_step_t seq_lock[] = {
        {420, 45}, {320, 35}
    };
    static const tone_step_t seq_game_over[] = {
        {520, 70}, {430, 70}, {330, 90}, {240, 130}
    };
    static const tone_step_t seq_menu_select[] = {
        {330, 12}, {392, 14}
    };
    static const tone_step_t seq_jump[] = {
        {420, 18}, {500, 20}, {600, 22}
    };
    static const tone_step_t seq_score[] = {
        {440, 16}, {520, 18}, {620, 22}
    };
    static const tone_step_t seq_collision[] = {
        {220, 18}, {180, 22}, {150, 28}
    };

    switch (sfx) {
    case SFX_ROTATE:      play_sequence(seq_rotate,      2); break;
    case SFX_LINE_CLEAR:  play_sequence(seq_line_clear,  3); break;
    case SFX_LOCK:        play_sequence(seq_lock,        2); break;
    case SFX_GAME_OVER:   play_sequence(seq_game_over,   4); break;
    case SFX_MENU_SELECT: play_sequence(seq_menu_select, 2); break;
    case SFX_JUMP:        play_sequence(seq_jump,        3); break;
    case SFX_SCORE:       play_sequence(seq_score,       3); break;
    case SFX_COLLISION:   play_sequence(seq_collision,   3); break;
    }
}

void audio_update(void)
{
    if (tone_active) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now >= tone_end_ms) {
            if (s_seq_active) {
                s_seq_idx++;
                if (s_seq_idx < s_seq_len) {
                    start_tone_now(s_seq[s_seq_idx].freq_hz,
                                   s_seq[s_seq_idx].duration_ms);
                } else {
                    s_seq_active = false;
                    speaker_off();
                }
            } else {
                speaker_off();
            }
        }
    }
}

void audio_test(void)
{
    printf("=== Audio Test ===\n");
    uint16_t freqs[] = {262, 330, 392, 523, 659, 784, 1047};
    for (int i = 0; i < 7; i++) {
        audio_play_tone(freqs[i], 200);
        sleep_ms(250);
    }
    speaker_off();
    printf("=== Audio Test Done ===\n");
}
