/* FunKey audio engine: content detection and PCM sources. */
#ifndef FKA_DECODE_H
#define FKA_DECODE_H

#include <stdint.h>
#include "fka_song.h"

/*
 * A PCM source yields mono 16-bit samples at its own rate. Fully decoded
 * formats (WAV, AU, AMR) keep the whole buffer; MP3 keeps the compressed
 * bytes and decodes frames while playing so long tracks cost no memory.
 */
typedef struct FkaPcm {
    int rate;
    int64_t length;          /* samples, -1 if unknown */
    /* decoded window: samples [win_start, win_start + win_len) */
    int16_t *win;
    int win_len;
    int64_t win_start;
    int whole;               /* win holds the entire sound */
    /* streaming state (MP3) */
    void *mp3;               /* mp3dec_t */
    uint8_t *data;
    int data_len, data_pos;
    int win_cap;
    int mp3_start;           /* first audio frame (after ID3 / Info frame) */
    int mp3_delay, mp3_to_skip; /* encoder delay to trim (gapless loops) */
    void *mld;               /* FkaMld: i-mode melody synthesised while playing */
    float *mld_buf;
    int mld_loop;            /* set by the player: loop at the MLD cuepoints */
    int64_t rebase;          /* samples the source jumped back (cuepoint loop),
                              * the player moves its position back and clears it */
} FkaPcm;

typedef struct {
    int kind;                /* FKA_KIND_PCM / FKA_KIND_SONG */
    const char *type;        /* detected MIME type */
    FkaPcm pcm;
    FkaSong song;
} FkaContent;

/* Detects the format from the bytes (the declared type is only a hint,
 * games often get it wrong). Returns 0 and fills out, or < 0. */
int fka_decode(const uint8_t *data, int len, const char *hint, FkaContent *out);
void fka_content_free(FkaContent *c);

/* Makes sample idx and idx + 1 available; returns a pointer to sample idx
 * or NULL past the end. Streaming sources only move forward cheaply;
 * going backwards restarts decoding. */
const int16_t *fka_pcm_at(FkaPcm *p, int64_t idx);
void fka_pcm_free(FkaPcm *p);

#endif
