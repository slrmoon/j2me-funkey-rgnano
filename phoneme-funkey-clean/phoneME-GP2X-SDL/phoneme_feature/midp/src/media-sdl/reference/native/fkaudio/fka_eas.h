/* FunKey audio engine: thin wrapper over Sonivox EAS (call under the engine lock). */
#ifndef FKA_EAS_H
#define FKA_EAS_H

#include <stdint.h>

int  fka_eas_init(int engine);
void fka_eas_shutdown(void);
int  fka_eas_set_engine(int engine);
void fka_eas_stream_engine(void *stream, int engine);
int  fka_eas_frame(void);                 /* samples per EAS render call */
void *fka_eas_open(void);                 /* new MIDI stream + virtual synth */
void fka_eas_close(void *stream);
void fka_eas_write(void *stream, const uint8_t *msg, int len);
void fka_eas_volume(void *stream, int gain_q15);
void fka_eas_transpose(void *stream, int semitones);
void fka_eas_silence(void *stream, int immediate);
void fka_eas_reset(void *stream);
int  fka_eas_channel_volume(void *stream, int ch);
int  fka_eas_channel_program(void *stream, int ch, int *bank);
int  fka_eas_active(void *stream);
int  fka_eas_render(int16_t *out);        /* fka_eas_frame() samples */
int  fka_eas_load_bank(const char *path);

#endif
