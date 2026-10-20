/*
 * FunKey audio engine: timed MIDI event lists ("songs").
 *
 * Every musical format (SMF, SMAF, iMelody, Nokia OTA, RTTTL, JSR-135 tone
 * sequences) is converted into one FkaSong: MIDI messages stamped with their
 * time in microseconds at normal playback rate. The sequencer only ever
 * plays FkaSongs, so seeking, looping, tempo and MIDIControl behave the same
 * for every format.
 */
#ifndef FKA_SONG_H
#define FKA_SONG_H

#include <stdint.h>

#define FKA_EV_SHORT 0   /* status/d1/d2 MIDI channel message */
#define FKA_EV_SYSEX 1   /* sysex bytes in the song pool: offset d (32 bit) */
#define FKA_EV_TEMPO 2   /* tempo change, microseconds per quarter in d */
#define FKA_EV_WAVE  3   /* one-shot PCM: wave index d, d1 = pitch+64, d2 = gain */

/* Embedded PCM (SMAF ADPCM voices / audio tracks), mono 16 bit. */
typedef struct {
    int16_t *pcm;
    int len;
    int rate;
} FkaWave;

typedef struct {
    uint32_t t;          /* microseconds from the start, rate 1.0 */
    uint8_t kind;
    uint8_t status;
    uint8_t d1;
    uint8_t d2;
    uint32_t d;          /* sysex: pool offset (len in d1|d2<<8); tempo */
} FkaEvent;

typedef struct {
    FkaEvent *ev;
    int count;
    int cap;
    uint8_t *pool;       /* sysex payloads */
    int pool_len;
    int pool_cap;
    FkaWave *waves;
    int wave_count;
    uint32_t duration;   /* microseconds, end of the last track */
    uint32_t first_tempo;/* microseconds per quarter at t=0 */
    int error;           /* allocation failure while building */
} FkaSong;

void fka_song_init(FkaSong *s);
void fka_song_free(FkaSong *s);

/* Builders (append in time order or call fka_song_sort afterwards). */
void fka_song_short(FkaSong *s, uint32_t t, int status, int d1, int d2);
void fka_song_sysex(FkaSong *s, uint32_t t, const uint8_t *data, int len);
void fka_song_tempo(FkaSong *s, uint32_t t, uint32_t us_per_quarter);
/* Takes ownership of pcm (malloc'ed); returns the wave index or -1. */
int fka_song_add_wave(FkaSong *s, int16_t *pcm, int len, int rate);
void fka_song_wave(FkaSong *s, uint32_t t, int wave, int semitones, int gain);
void fka_song_sort(FkaSong *s);
void fka_song_finish(FkaSong *s, uint32_t end_us);

/* Tempo (microseconds per quarter) in effect at time t. */
uint32_t fka_song_tempo_at(const FkaSong *s, uint32_t t);

/* Parsers. Return 0 on success, negative on error. */
int fka_parse_smf(const uint8_t *data, int len, FkaSong *out);
int fka_parse_tone_sequence(const uint8_t *data, int len, FkaSong *out);
int fka_parse_ota(const uint8_t *data, int len, FkaSong *out);
int fka_parse_imelody(const uint8_t *data, int len, FkaSong *out);
int fka_parse_rtttl(const uint8_t *data, int len, FkaSong *out);
int fka_parse_smaf(const uint8_t *data, int len, FkaSong *out);

#endif
