/*
 * audio.h — Non-blocking PWM audio for sound effects.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* Sound-effect identifiers. */
typedef enum {
    SFX_ROTATE,
    SFX_LINE_CLEAR,
    SFX_LOCK,
    SFX_GAME_OVER,
    SFX_MENU_SELECT,
    SFX_JUMP,
    SFX_SCORE,
    SFX_COLLISION
} sfx_id_t;

/* Initialise the PWM slice for the speaker pin. */
void audio_init(void);

/* Start playing a tone at the given frequency (Hz) for duration_ms.
 * Non-blocking: the tone is silenced in audio_update(). */
void audio_play_tone(uint16_t freq_hz, uint16_t duration_ms);

/* Trigger a predefined sound effect. */
void audio_play_sfx(sfx_id_t sfx);

/* Call once per frame from the main loop to manage tone timing. */
void audio_update(void);

/* Play a short test sweep to verify speaker wiring. */
void audio_test(void);

#endif /* AUDIO_H */
