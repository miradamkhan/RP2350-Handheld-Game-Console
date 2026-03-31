#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

void audio_init(void);
void audio_update(void);
void audio_play_tone(uint32_t freq_hz, uint32_t duration_ms);
void audio_play_jump(void);
void audio_play_score(void);
void audio_play_collision(void);
bool audio_is_busy(void);
void audio_test(void);

#endif // AUDIO_H
