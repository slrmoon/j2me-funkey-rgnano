/*
 * Sound quality level and the band-limited resampler of the high one.
 *
 * FunKey S / RG Nano (ARM build): "low" - linear interpolation, exactly
 * what the device has always played; the speaker is a 15 mm mono one and
 * the CPU is busy with the game. Desktop and browser builds: "high" -
 * sampled sounds and the output are resampled with a windowed sinc, the
 * synth presets are level-matched and kept out of the limiter.
 * PHONEME_AUDIO_QUALITY=low|high overrides the default of the build.
 */
#ifndef FKA_RESAMPLE_H
#define FKA_RESAMPLE_H
#include <stdint.h>

#define FKA_SINC_TAPS   32
#define FKA_SINC_PHASES 256

int fka_quality_high(void);

/* Kernel for reading a signal sampled at src_rate while writing at
 * dst_rate: FKA_SINC_PHASES rows of FKA_SINC_TAPS coefficients (Q14),
 * low-pass at the lower of the two Nyquist limits. Tables are cached. */
const int16_t *fka_sinc_table(int src_rate, int dst_rate);

/* x points at sample idx - (FKA_SINC_TAPS / 2 - 1); frac16 is the 16-bit
 * fraction of the read position between sample idx and idx + 1. */
static inline int fka_sinc_eval(const int16_t *table, const int16_t *x, unsigned frac16)
{
    const int16_t *k = table + (frac16 >> 8) * FKA_SINC_TAPS;
    int32_t acc = 0;
    int i;
    for (i = 0; i < FKA_SINC_TAPS; ++i) acc += (int32_t) x[i] * k[i];
    acc >>= 14;
    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;
    return (int) acc;
}

#endif
