/*
 * FunKey audio engine: public interface.
 *
 * One mixer at 22050 Hz mono (the FunKey S / RG Nano speaker is mono and the
 * MIDI synth renders natively at that rate). Players are identified by small
 * positive integers. Every function is safe to call from the Java thread;
 * the audio callback runs on its own thread and the engine serialises the
 * two with fka_lock()/fka_unlock().
 */
#ifndef FKA_H
#define FKA_H

#include <stdint.h>

#define FKA_RATE 22050

/* content kinds */
#define FKA_KIND_NONE   0
#define FKA_KIND_PCM    1   /* WAV, AU, AMR, MP3 ... decoded to PCM */
#define FKA_KIND_SONG   2   /* MIDI-like: SMF, SMAF, iMelody, OTA, RTTTL, tone seq */
#define FKA_KIND_DEVICE 3   /* interactive MIDI device (MIDIControl only) */

/* events reported to Java (fka_poll_event) */
#define FKA_EVENT_END_OF_MEDIA 1   /* arg = media time (us); data = 1 if looping on */
#define FKA_EVENT_STOPPED_AT   2   /* StopTimeControl reached, arg = media time */

/* synth engines */
#define FKA_SYNTH_WAVETABLE 1
#define FKA_SYNTH_FM        2
#define FKA_SYNTH_HYBRID    3

/* engine */
int  fka_init(void);
void fka_shutdown(void);
void fka_set_master_volume(int percent);
int  fka_get_master_volume(void);
void fka_pause_all(int paused);   /* MIDlet paused / system menu */

/* Render frames of mono audio (called by the output driver). */
void fka_render(int16_t *out, int frames);

/* players */
int  fka_player_new(const uint8_t *data, int len, const char *type_hint);
int  fka_player_new_device(void);
int  fka_player_new_tone_sequence(const uint8_t *seq, int len);
void fka_player_delete(int id);
int  fka_player_kind(int id);
const char *fka_player_content_type(int id);
int  fka_player_prefetch(int id);          /* reserve synth resources */
void fka_player_deallocate(int id);
int  fka_player_start(int id);
void fka_player_stop(int id);
int  fka_player_is_playing(int id);
void fka_player_set_loop_count(int id, int count);   /* -1 = forever */
int64_t fka_player_get_time(int id);       /* us */
int64_t fka_player_set_time(int id, int64_t us);
int64_t fka_player_get_duration(int id);   /* us, -1 unknown */
void fka_player_set_volume(int id, int level, int muted);
void fka_player_set_stop_time(int id, int64_t us);    /* -1 = none */
int  fka_player_set_rate(int id, int milli_percent);  /* returns applied rate */
int  fka_player_get_rate(int id);
int  fka_player_get_tempo(int id);         /* milli-beats per minute */
int  fka_player_set_tempo(int id, int milli_bpm);
void fka_player_set_pitch(int id, int milli_semitones);
int  fka_player_replace_tone_sequence(int id, const uint8_t *seq, int len);

/* MIDIControl */
void fka_midi_short(int id, int status, int d1, int d2);
void fka_midi_long(int id, const uint8_t *data, int len);
int  fka_midi_get_channel_volume(int id, int channel);
int  fka_midi_get_program(int id, int channel, int *bank);

/* Manager.playTone */
void fka_play_tone(int note, int duration_ms, int volume);

/* events */
int  fka_poll_event(int *player, int *type, int64_t *arg, int *data);

/* configuration */
void fka_set_synth(int engine);            /* FKA_SYNTH_* */
int  fka_load_soundbank(const char *path); /* external DLS / SF2 */

/* output driver (fka_sdl.c) */
int  fka_output_open(void);
void fka_output_close(void);
void fka_lock(void);
void fka_unlock(void);

#endif
