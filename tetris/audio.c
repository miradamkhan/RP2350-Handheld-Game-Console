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

/* ── Helpers ── */

static void speaker_off(void)
{
    pwm_set_enabled(pwm_slice, false);
    tone_active = false;
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
    if (freq_hz == 0) {
        speaker_off();
        return;
    }

    uint32_t sys_clk = clock_get_hz(clk_sys);

    /*
     * PWM frequency = sys_clk / (divider * wrap).
     * Choose divider so wrap fits in 16 bits.
     * Start with divider = 1, increase if wrap overflows.
     */
    uint32_t wrap = sys_clk / freq_hz;
    uint16_t divider_int = 1;
    while (wrap > 65535 && divider_int < 256) {
        divider_int++;
        wrap = sys_clk / (freq_hz * divider_int);
    }
    if (wrap < 2) wrap = 2;

    pwm_set_clkdiv_int_frac(pwm_slice, divider_int, 0);
    pwm_set_wrap(pwm_slice, (uint16_t)(wrap - 1));
    pwm_set_chan_level(pwm_slice, pwm_channel, (uint16_t)(wrap / 2));  /* 50% duty */
    pwm_set_enabled(pwm_slice, true);

    tone_end_ms = to_ms_since_boot(get_absolute_time()) + duration_ms;
    tone_active = true;
}

void audio_play_sfx(sfx_id_t sfx)
{
    switch (sfx) {
    case SFX_ROTATE:      audio_play_tone(800,  60);  break;
    case SFX_LINE_CLEAR:  audio_play_tone(1200, 150); break;
    case SFX_LOCK:        audio_play_tone(400,  80);  break;
    case SFX_GAME_OVER:   audio_play_tone(200,  500); break;
    case SFX_MENU_SELECT: audio_play_tone(1000, 100); break;
    }
}

void audio_update(void)
{
    if (tone_active) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now >= tone_end_ms) {
            speaker_off();
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
