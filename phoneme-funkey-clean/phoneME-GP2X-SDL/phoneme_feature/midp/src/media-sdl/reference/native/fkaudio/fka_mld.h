/* FunKey audio engine: i-mode MLD sequencer + Yamaha MA-3 FM synthesizer. */
#ifndef FKA_MLD_H
#define FKA_MLD_H

#include <stdint.h>

typedef struct FkaMld FkaMld;

FkaMld *fka_mld_open(const uint8_t *data, int len, float sample_rate);
void fka_mld_close(FkaMld *p);
void fka_mld_reset(FkaMld *p);
/* Stereo float frames; returns frames made, -1 once finished (MLDPlayer.render). */
int fka_mld_render(FkaMld *p, float *out, int frames);
int fka_mld_finished(FkaMld *p);
int fka_mld_is_silent(FkaMld *p);
long long fka_mld_count_frames(const uint8_t *data, int len, float sample_rate);

#endif
