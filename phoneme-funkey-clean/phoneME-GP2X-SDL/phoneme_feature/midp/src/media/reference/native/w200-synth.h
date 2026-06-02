#ifndef PHONEME_W200_SYNTH_H
#define PHONEME_W200_SYNTH_H

#include "tinysoundfont/tml.h"

#define W200_SYNTH_OUTPUT_RATE 22050
#define W200_SYNTH_MAX_VOICES 72
#define W200_SYNTH_RELEASE_MS 5000

struct W200Synth;

struct W200Synth *w200_synth_open(void);
void w200_synth_close(struct W200Synth *synth);
void w200_synth_apply_midi(struct W200Synth *synth, tml_message *message);
void w200_synth_render(struct W200Synth *synth, short *pcm, int frames);
int w200_synth_active_voice_count(struct W200Synth *synth);
const char *w200_synth_backend_name(void);

#endif
