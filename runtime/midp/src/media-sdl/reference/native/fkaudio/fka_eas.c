/* TinySoundFont MIDI backend. TinySoundFont is MIT; the bundled TimGM6mb
 * SoundFont is GPL-2. Calls are serialized by the fkaudio engine lock. */
#include "fka_eas.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tsf.h"

#define RATE 22050
#define FRAME 64
#define TAIL_FRAMES (RATE * 2 / FRAME)

typedef struct Stream {
    tsf *synth;
    struct Stream *next;
    int gain, transpose, notes, tail;
    uint8_t held[16][128], volume[16], program[16], status, data[2];
    uint16_t bank[16];
    int ndata;
} Stream;

static tsf *g_bank;
static Stream *g_streams;

static void reset_stream(Stream *s)
{
    int ch;
    if (!s || !s->synth) return;
    tsf_reset(s->synth);
    memset(s->held, 0, sizeof(s->held));
    s->notes = s->tail = s->status = s->ndata = 0;
    for (ch = 0; ch < 16; ++ch) {
        s->volume[ch] = 100; s->program[ch] = 0; s->bank[ch] = 0;
        tsf_channel_set_presetnumber(s->synth, ch, 0, ch == 9);
        tsf_channel_set_volume(s->synth, ch, 100.0f / 127.0f);
        tsf_channel_set_pan(s->synth, ch, 0.5f);
        tsf_channel_set_pitchwheel(s->synth, ch, 8192);
        tsf_channel_set_tuning(s->synth, ch, (float)s->transpose);
    }
}

int fka_eas_init(int engine) { (void)engine; return 0; }
int fka_eas_frame(void) { return FRAME; }
int fka_eas_set_engine(int engine) { (void)engine; return 0; }
void fka_eas_stream_engine(void *stream, int engine) { (void)stream; (void)engine; }

void fka_eas_shutdown(void)
{
    Stream *s = g_streams;
    while (s) { Stream *next = s->next; tsf_close(s->synth); free(s); s = next; }
    g_streams = NULL;
    if (g_bank) tsf_close(g_bank);
    g_bank = NULL;
}

void *fka_eas_open(void)
{
    Stream *s;
    if (!g_bank) return NULL;
    s = (Stream *)calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->synth = tsf_copy(g_bank);
    if (!s->synth) { free(s); return NULL; }
    tsf_set_output(s->synth, TSF_MONO, RATE, -3.0f);
    s->gain = 0x7fff;
    reset_stream(s);
    s->next = g_streams; g_streams = s;
    return s;
}

void fka_eas_close(void *stream)
{
    Stream *s = (Stream *)stream, **p = &g_streams;
    if (!s) return;
    while (*p && *p != s) p = &(*p)->next;
    if (*p == s) *p = s->next;
    tsf_close(s->synth); free(s);
}

static void off(Stream *s, int ch, int key)
{
    tsf_channel_note_off(s->synth, ch, key);
    if (s->held[ch][key]) { s->held[ch][key] = 0; if (s->notes) --s->notes; }
    if (!s->notes) s->tail = TAIL_FRAMES;
}

static void recount(Stream *s)
{
    int ch, key;
    s->notes = 0;
    for (ch = 0; ch < 16; ++ch) for (key = 0; key < 128; ++key)
        s->notes += s->held[ch][key] != 0;
    if (!s->notes) s->tail = TAIL_FRAMES;
}

static void message(Stream *s, uint8_t st, uint8_t a, uint8_t b)
{
    int ch = st & 15, key, n;
    switch (st & 0xf0) {
    case 0x80: off(s, ch, a & 127); break;
    case 0x90:
        key = a & 127;
        if (!(b & 127)) { off(s, ch, key); break; }
        if (!s->held[ch][key]) ++s->notes;
        s->held[ch][key] = 1; s->tail = 0;
        tsf_channel_note_on(s->synth, ch, key, (float)(b & 127) / 127.0f);
        break;
    case 0xb0:
        if (a == 0) s->bank[ch] = (uint16_t)((s->bank[ch] & 127) | ((b & 127) << 7));
        else if (a == 32) s->bank[ch] = (uint16_t)((s->bank[ch] & 0x3f80) | (b & 127));
        else if (a == 7) s->volume[ch] = b & 127;
        tsf_channel_midi_control(s->synth, ch, a & 127, b & 127);
        if (a == 120 || a == 123) {
            for (n = 0; n < 128; ++n) s->held[ch][n] = 0;
            recount(s);
        }
        break;
    case 0xc0:
        s->program[ch] = a & 127;
        tsf_channel_set_presetnumber(s->synth, ch, a & 127, ch == 9);
        break;
    case 0xe0: tsf_channel_set_pitchwheel(s->synth, ch, (a & 127) | ((b & 127) << 7)); break;
    default: break;
    }
}

void fka_eas_write(void *stream, const uint8_t *msg, int len)
{
    Stream *s = (Stream *)stream;
    int i, need;
    if (!s || !msg) return;
    for (i = 0; i < len; ++i) {
        uint8_t b = msg[i];
        if (b & 0x80) {
            if (b >= 0xf8) continue;
            if (b >= 0xf0) { s->status = 0; s->ndata = 0; continue; }
            s->status = b; s->ndata = 0; continue;
        }
        if (!s->status) continue;
        need = ((s->status & 0xf0) == 0xc0 || (s->status & 0xf0) == 0xd0) ? 1 : 2;
        s->data[s->ndata++] = b;
        if (s->ndata == need) {
            message(s, s->status, s->data[0], need == 2 ? s->data[1] : 0);
            s->ndata = 0;
        }
    }
}

void fka_eas_volume(void *stream, int gain)
{
    Stream *s = (Stream *)stream;
    if (!s) return;
    s->gain = gain < 0 ? 0 : (gain > 0x7fff ? 0x7fff : gain);
}

void fka_eas_transpose(void *stream, int semitones)
{
    Stream *s = (Stream *)stream; int ch;
    if (!s) return;
    s->transpose = semitones;
    for (ch = 0; ch < 16; ++ch) tsf_channel_set_tuning(s->synth, ch, (float)semitones);
}

void fka_eas_silence(void *stream, int immediate)
{
    Stream *s = (Stream *)stream; int ch;
    if (!s) return;
    for (ch = 0; ch < 16; ++ch) {
        if (immediate) tsf_channel_sounds_off_all(s->synth, ch);
        else tsf_channel_note_off_all(s->synth, ch);
    }
    memset(s->held, 0, sizeof(s->held)); s->notes = 0;
    s->tail = immediate ? 0 : TAIL_FRAMES;
}

void fka_eas_reset(void *stream) { reset_stream((Stream *)stream); }
int fka_eas_channel_volume(void *stream, int ch)
{
    Stream *s = (Stream *)stream;
    return s && ch >= 0 && ch < 16 ? s->volume[ch] : -1;
}
int fka_eas_channel_program(void *stream, int ch, int *bank)
{
    Stream *s = (Stream *)stream;
    if (!s || ch < 0 || ch >= 16) return -1;
    if (bank) *bank = s->bank[ch];
    return s->program[ch];
}
int fka_eas_active(void *stream)
{
    Stream *s = (Stream *)stream;
    return s && (s->notes || s->tail);
}

int fka_eas_render(int16_t *out)
{
    Stream *s; int16_t tmp[FRAME]; int i;
    if (!out) return 0;
    memset(out, 0, sizeof(int16_t) * FRAME);
    for (s = g_streams; s; s = s->next) {
        if (!fka_eas_active(s)) continue;
        tsf_render_short(s->synth, tmp, FRAME, 0);
        for (i = 0; i < FRAME; ++i) {
            int v = out[i] + (int)(((int64_t)tmp[i] * s->gain) >> 15);
            out[i] = (int16_t)(v < -32768 ? -32768 : (v > 32767 ? 32767 : v));
        }
        if (!s->notes && s->tail) --s->tail;
    }
    return FRAME;
}

int fka_eas_load_bank(const char *path)
{
    tsf *bank;
    if (!path || g_streams) return -1;
    bank = tsf_load_filename(path);
    if (!bank) return -2;
    tsf_set_output(bank, TSF_MONO, RATE, -3.0f);
    if (g_bank) tsf_close(g_bank);
    g_bank = bank;
    return 0;
}
