/*
 * FunKey audio engine: players, sequencer and mixer.
 *
 * Output is 22050 Hz mono, produced in blocks of the EAS frame (64 samples,
 * ~2.9 ms), which is also the timing resolution of MIDI events. Songs are
 * streamed: events are sent to the synthesizer as playback reaches them, so
 * nothing is pre-rendered, start() is instant, seeking/looping/tempo are
 * exact, and memory stays flat no matter how long the song is.
 */
#include "fka.h"
#include "fka_decode.h"
#include "fka_eas.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PLAYERS  256
#define MAX_SUBWAVES 8
#define MAX_TONES    32
#define EVENT_RING   256
#define FADE_STEP    512          /* q15 per sample: ~64 samples fade */
#define FRAME_MAX    256

typedef struct {
    const FkaWave *w;
    uint64_t pos, step;           /* 32.32 */
    int gain;                     /* q15 */
} SubWave;

typedef struct {
    int id;
    int kind;
    const char *type;
    FkaContent c;
    /* transport */
    int playing;
    int at_end;
    int loop_count;               /* -1 forever, n >= 1 total plays */
    int loops_done;
    int64_t stop_time;            /* us, -1 none */
    int rate;                     /* milli-percent, 100000 = normal */
    int pitch;                    /* semitones */
    /* volume */
    int level, muted, gain;       /* gain q15 after curve */
    int fade, fade_target;        /* q15 click-free start/stop */
    /* PCM */
    uint64_t pos;                 /* 32.32 source samples */
    /* song / device */
    void *stream;
    double song_us;
    int next_ev;
    SubWave sub[MAX_SUBWAVES];
} Player;

typedef struct {
    int player, type, data;
    int64_t arg;
} Event;

typedef struct {
    int ch, note, left;           /* samples until note off */
} Tone;

static Player *g_players[MAX_PLAYERS];
static Event g_events[EVENT_RING];
static int g_ev_head, g_ev_tail;
static int g_ready;
static int g_paused;
static int g_pause_open = 1;   /* output gate, follows g_paused with a fade */
static int g_master = 0x7fff;
static int g_master_percent = 100;
static int g_frame = 64;
static int g_engine = FKA_SYNTH_HYBRID;
static void *g_tone_stream;
static Tone g_tones[MAX_TONES];
static int g_tone_channel;
static int16_t g_carry[FRAME_MAX];
static int g_carry_len, g_carry_pos;

/* ------------------------------------------------------------------ */

static int volume_curve(int level)
{
    /* MMAPI levels are perceptual; a square law is close to phone curves */
    if (level <= 0) return 0;
    if (level >= 100) return 0x7fff;
    return (level * level * 0x7fff) / 10000;
}

static Player *get(int id)
{
    if (id < 1 || id > MAX_PLAYERS) return NULL;
    return g_players[id - 1];
}

static void post(int player, int type, int64_t arg, int data)
{
    int next = (g_ev_head + 1) % EVENT_RING;
    if (next == g_ev_tail) return;   /* full: Java fell behind, drop */
    g_events[g_ev_head].player = player;
    g_events[g_ev_head].type = type;
    g_events[g_ev_head].arg = arg;
    g_events[g_ev_head].data = data;
    g_ev_head = next;
}

static void apply_gain(Player *p)
{
    p->gain = p->muted ? 0 : volume_curve(p->level);
    if (p->stream != NULL) fka_eas_volume(p->stream, p->gain);
}

static int ensure_stream(Player *p)
{
    int i;
    if (p->stream != NULL) return 0;
    p->stream = fka_eas_open();
    if (p->stream == NULL) {
        /* steal from an idle player (re-acquired when it starts again) */
        for (i = 0; i < MAX_PLAYERS; ++i) {
            Player *o = g_players[i];
            if (o != NULL && o != p && o->stream != NULL && !o->playing &&
                o->kind == FKA_KIND_SONG && fka_eas_active(o->stream) == 0) {
                fka_eas_close(o->stream);
                o->stream = NULL;
                break;
            }
        }
        p->stream = fka_eas_open();
        if (p->stream == NULL) return -1;
    }
    fka_eas_reset(p->stream);
    fka_eas_transpose(p->stream, p->pitch);
    apply_gain(p);
    return 0;
}

static void song_send(Player *p, const FkaEvent *e)
{
    uint8_t m[3];
    if (p->stream == NULL) return;
    if (e->kind == FKA_EV_SHORT) {
        int n = ((e->status & 0xf0) == 0xc0 || (e->status & 0xf0) == 0xd0) ? 2 : 3;
        m[0] = e->status;
        m[1] = e->d1;
        m[2] = e->d2;
        fka_eas_write(p->stream, m, n);
    } else if (e->kind == FKA_EV_SYSEX) {
        fka_eas_write(p->stream, p->c.song.pool + e->d, e->d1 | (e->d2 << 8));
    }
}

static void start_wave(Player *p, const FkaEvent *e)
{
    const FkaWave *w = &p->c.song.waves[e->d];
    int i, slot = 0, semis = (int) e->d1 - 64;
    double ratio = 1.0;
    for (i = 0; i < MAX_SUBWAVES; ++i) {
        if (p->sub[i].w == NULL) {
            slot = i;
            break;
        }
        if (p->sub[i].pos > p->sub[slot].pos) slot = i;   /* steal the oldest */
    }
    while (semis > 0) { ratio *= 1.059463094; --semis; }
    while (semis < 0) { ratio /= 1.059463094; ++semis; }
    p->sub[slot].w = w;
    p->sub[slot].pos = 0;
    p->sub[slot].step = (uint64_t) ((double) w->rate * ratio / FKA_RATE * 4294967296.0);
    p->sub[slot].gain = (e->d2 * 0x7fff) / 127;
}

/* Sends every event before t except notes: programs, controllers, bends
 * and sysex, so playback from t sounds as if it had played from 0. */
static void song_chase(Player *p, double t)
{
    int i;
    if (p->stream != NULL) fka_eas_reset(p->stream);
    for (i = 0; i < MAX_SUBWAVES; ++i) p->sub[i].w = NULL;
    for (i = 0; i < p->c.song.count && p->c.song.ev[i].t < t; ++i) {
        const FkaEvent *e = &p->c.song.ev[i];
        int hi = e->status & 0xf0;
        if (e->kind == FKA_EV_SHORT && (hi == 0x80 || hi == 0x90 || hi == 0xa0)) continue;
        if (e->kind == FKA_EV_SHORT || e->kind == FKA_EV_SYSEX) song_send(p, e);
    }
    p->next_ev = i;
    p->song_us = t;
}

static void notes_off(Player *p)
{
    int ch;
    uint8_t m[3];
    if (p->stream == NULL) return;
    for (ch = 0; ch < 16; ++ch) {
        m[0] = (uint8_t) (0xb0 | ch);
        m[1] = 64; m[2] = 0;          /* sustain off */
        fka_eas_write(p->stream, m, 3);
        m[1] = 123;                   /* all notes off */
        fka_eas_write(p->stream, m, 3);
    }
}

static int64_t media_time(Player *p)
{
    if (p->kind == FKA_KIND_PCM) {
        int64_t idx = (int64_t) (p->pos >> 32);
        return p->c.pcm.rate > 0 ? idx * 1000000 / p->c.pcm.rate : 0;
    }
    if (p->kind == FKA_KIND_SONG) return (int64_t) p->song_us;
    return 0;
}

static int64_t duration(Player *p)
{
    if (p->kind == FKA_KIND_PCM)
        return p->c.pcm.length >= 0 && p->c.pcm.rate > 0 ?
               p->c.pcm.length * 1000000 / p->c.pcm.rate : -1;
    if (p->kind == FKA_KIND_SONG) return (int64_t) p->c.song.duration;
    return -1;
}

/* Lets every sounding note finish its release. Sonivox EAS implements All
 * Notes Off (CC 123) as a fast mute, so notes_off() cut the decay of the last
 * notes (cymbals, timpani, gunshots...) the moment a song ended or looped. */
static void notes_release(Player *p)
{
    if (p->stream != NULL) fka_eas_silence(p->stream, 0);
}

/* end of media reached: loop or stop */
static void reached_end(Player *p)
{
    int more = p->loop_count < 0 || p->loops_done + 1 < p->loop_count;
    int64_t t = duration(p);
    post(p->id, FKA_EVENT_END_OF_MEDIA, t, more);
    if (more) {
        ++p->loops_done;
        if (p->kind == FKA_KIND_PCM) {
            p->pos = 0;
        } else {
            double carry = p->song_us - (double) p->c.song.duration;
            int ch;
            uint8_t m[3];
            notes_release(p);
            for (ch = 0; ch < 16 && p->stream != NULL; ++ch) {
                m[0] = (uint8_t) (0xb0 | ch); m[1] = 121; m[2] = 0;   /* reset controllers */
                fka_eas_write(p->stream, m, 3);
            }
            p->next_ev = 0;
            p->song_us = carry > 0 ? carry : 0;
        }
    } else {
        p->playing = 0;
        p->at_end = 1;
        p->loops_done = 0;
        if (p->kind == FKA_KIND_SONG) notes_release(p);
        else p->fade_target = 0;
    }
}

/* ------------------------------------------------------------------ */
/* rendering                                                           */
/* ------------------------------------------------------------------ */

static void tones_tick(int n)
{
    int i;
    for (i = 0; i < MAX_TONES; ++i) {
        Tone *t = &g_tones[i];
        if (t->left <= 0) continue;
        t->left -= n;
        if (t->left <= 0 && g_tone_stream != NULL) {
            uint8_t m[3];
            m[0] = (uint8_t) (0x80 | t->ch);
            m[1] = (uint8_t) t->note;
            m[2] = 0;
            fka_eas_write(g_tone_stream, m, 3);
        }
    }
}

static void song_tick(Player *p, int n)
{
    double end = p->song_us + (double) n * 1.0e6 / FKA_RATE * p->rate / 100000.0;
    const FkaSong *s = &p->c.song;
    while (p->next_ev < s->count && s->ev[p->next_ev].t <= end) {
        const FkaEvent *e = &s->ev[p->next_ev++];
        if (e->kind == FKA_EV_WAVE) start_wave(p, e);
        else song_send(p, e);
    }
    p->song_us = end;
    if (p->stop_time >= 0 && p->song_us >= (double) p->stop_time) {
        p->playing = 0;
        notes_off(p);
        post(p->id, FKA_EVENT_STOPPED_AT, (int64_t) p->song_us, 0);
        p->stop_time = -1;
        return;
    }
    if (p->next_ev >= s->count && p->song_us >= (double) s->duration) reached_end(p);
}

static void mix_subwaves(Player *p, int32_t *acc, int n)
{
    int i, k;
    int g = (int) (((int64_t) p->gain * g_master) >> 15);
    for (k = 0; k < MAX_SUBWAVES; ++k) {
        SubWave *w = &p->sub[k];
        int gg;
        if (w->w == NULL) continue;
        gg = (w->gain * g) >> 15;
        for (i = 0; i < n; ++i) {
            uint32_t idx = (uint32_t) (w->pos >> 32);
            int s0, s1, frac;
            if (idx + 1 >= (uint32_t) w->w->len) {
                w->w = NULL;
                break;
            }
            s0 = w->w->pcm[idx];
            s1 = w->w->pcm[idx + 1];
            frac = (int) ((w->pos >> 16) & 0xffff);
            acc[i] += ((s0 + (((s1 - s0) * frac) >> 16)) * gg) >> 15;
            w->pos += w->step;
        }
    }
}

static void mix_pcm(Player *p, int32_t *acc, int n)
{
    FkaPcm *src = &p->c.pcm;
    uint64_t step = ((uint64_t) src->rate << 32) / FKA_RATE;
    int i, g = (int) (((int64_t) p->gain * g_master) >> 15);
    if (p->rate != 100000) step = (uint64_t) ((double) step * p->rate / 100000.0);
    /* MLD loops at its cuepoints itself while more plays are wanted */
    src->mld_loop = p->loop_count < 0 || p->loops_done + 1 < p->loop_count;
    for (i = 0; i < n; ++i) {
        int64_t idx = (int64_t) (p->pos >> 32);
        const int16_t *s;
        int s0, s1, frac, v;
        if (p->fade != p->fade_target) {
            if (p->fade < p->fade_target) {
                p->fade += FADE_STEP;
                if (p->fade > p->fade_target) p->fade = p->fade_target;
            } else {
                p->fade -= FADE_STEP;
                if (p->fade < p->fade_target) p->fade = p->fade_target;
            }
        }
        if (!p->playing && p->fade == 0) return;
        s = fka_pcm_at(src, idx);
        if (src->rebase != 0) {
            /* the melody jumped back to its loop start */
            p->pos -= (uint64_t) src->rebase << 32;
            idx -= src->rebase;
            src->rebase = 0;
            if (p->playing) {
                post(p->id, FKA_EVENT_END_OF_MEDIA, duration(p), 1);
                ++p->loops_done;
            }
            src->mld_loop = p->loop_count < 0 || p->loops_done + 1 < p->loop_count;
        }
        if (s == NULL) {
            if (p->playing) {
                reached_end(p);
                if (p->playing) continue;   /* looped */
            }
            p->fade = p->fade_target = 0;
            return;
        }
        s0 = s[0];
        if (src->mld != NULL)
            s1 = idx + 1 < src->win_start + src->win_len ? s[1] : s0;
        else
            s1 = (src->length < 0 || idx + 1 < src->length) ? s[1] : s0;
        frac = (int) ((p->pos >> 16) & 0xffff);
        v = s0 + (((s1 - s0) * frac) >> 16);
        acc[i] += (int) (((int64_t) v * g >> 15) * p->fade >> 15);
        if (p->playing) p->pos += step;
        if (p->playing && p->stop_time >= 0 && media_time(p) >= p->stop_time) {
            p->playing = 0;
            p->fade_target = 0;
            post(p->id, FKA_EVENT_STOPPED_AT, media_time(p), 0);
            p->stop_time = -1;
        }
    }
}

static int16_t soft_clip(int32_t x)
{
    /* transparent up to -1.5 dBFS, then a smooth knee instead of hard clipping */
    const int32_t knee = 27500, room = 32767 - 27500;
    int32_t a = x < 0 ? -x : x;
    if (a > knee) {
        int32_t over = a - knee;
        a = knee + (int32_t) (((int64_t) room * over) / (over + room));
    }
    return (int16_t) (x < 0 ? -a : a);
}

static void render_frame(int16_t *out, int n)
{
    int32_t acc[FRAME_MAX];
    int16_t synth[FRAME_MAX];
    int i, any_stream = 0, ramp = 0;
    memset(acc, 0, sizeof(int32_t) * (size_t) n);
    /* pause/resume: one frame (~3 ms) fade instead of a click */
    if (g_paused && g_pause_open) ramp = -1;
    else if (!g_paused && !g_pause_open) ramp = 1;
    if (!g_paused || ramp != 0) {
        tones_tick(n);
        for (i = 0; i < MAX_PLAYERS; ++i) {
            Player *p = g_players[i];
            if (p == NULL) continue;
            if (p->stream != NULL) any_stream = 1;
            if (p->kind == FKA_KIND_SONG && p->playing) song_tick(p, n);
            if (p->kind == FKA_KIND_SONG) mix_subwaves(p, acc, n);
            else if (p->kind == FKA_KIND_PCM && (p->playing || p->fade)) mix_pcm(p, acc, n);
        }
        if ((any_stream || g_tone_stream != NULL) && fka_eas_render(synth) == n) {
            for (i = 0; i < n; ++i) acc[i] += (synth[i] * g_master) >> 15;
        }
    }
    if (ramp != 0) {
        for (i = 0; i < n; ++i) {
            int g = ramp > 0 ? i : n - 1 - i;
            acc[i] = (int32_t) (((int64_t) acc[i] * g) / n);
        }
        g_pause_open = ramp > 0;
    }
    for (i = 0; i < n; ++i) out[i] = soft_clip(acc[i]);
}

void fka_render(int16_t *out, int frames)
{
    if (!g_ready) {
        memset(out, 0, sizeof(int16_t) * (size_t) frames);
        return;
    }
    while (frames > 0) {
        if (g_carry_pos < g_carry_len) {
            int n = g_carry_len - g_carry_pos;
            if (n > frames) n = frames;
            memcpy(out, g_carry + g_carry_pos, sizeof(int16_t) * (size_t) n);
            g_carry_pos += n;
            out += n;
            frames -= n;
            continue;
        }
        if (frames >= g_frame) {
            render_frame(out, g_frame);
            out += g_frame;
            frames -= g_frame;
        } else {
            render_frame(g_carry, g_frame);
            g_carry_len = g_frame;
            g_carry_pos = 0;
        }
    }
}

/* ------------------------------------------------------------------ */
/* engine                                                              */
/* ------------------------------------------------------------------ */

int fka_init(void)
{
    if (g_ready) return 0;
    if (fka_eas_init(g_engine) != 0) return -1;
    g_frame = fka_eas_frame();
    if (g_frame <= 0 || g_frame > FRAME_MAX) return -2;
    g_ready = 1;
    return 0;
}

void fka_shutdown(void)
{
    int i;
    for (i = 0; i < MAX_PLAYERS; ++i) {
        if (g_players[i] != NULL) fka_player_delete(i + 1);
    }
    if (g_tone_stream != NULL) fka_eas_close(g_tone_stream);
    g_tone_stream = NULL;
    fka_eas_shutdown();
    g_ready = 0;
}

void fka_set_master_volume(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    g_master_percent = percent;
    g_master = volume_curve(percent);
}

int fka_get_master_volume(void)
{
    return g_master_percent;
}

void fka_pause_all(int paused)
{
    g_paused = paused;
}

/* Switches the instrument set live: new streams and every open one. Voices
 * of the old set are cut (callers do this while audio is paused). */
void fka_set_synth(int engine)
{
    int i;
    if (engine != FKA_SYNTH_WAVETABLE && engine != FKA_SYNTH_FM) engine = FKA_SYNTH_HYBRID;
    g_engine = engine;
    if (!g_ready) return;
    /* A muted voice still renders its release ramp with the region data of
     * the set it started on; switching under it reads the wrong library
     * (crash). Mute everything and run the synth until all voices are gone. */
    {
        int16_t scratch[FRAME_MAX];
        int k, busy = 1;
        for (i = 0; i < MAX_PLAYERS; ++i) {
            Player *p = g_players[i];
            if (p != NULL && p->stream != NULL) fka_eas_silence(p->stream, 1);
        }
        if (g_tone_stream != NULL) fka_eas_silence(g_tone_stream, 1);
        for (k = 0; k < 256 && busy; ++k) {
            busy = 0;
            for (i = 0; i < MAX_PLAYERS && !busy; ++i) {
                Player *p = g_players[i];
                if (p != NULL && p->stream != NULL && fka_eas_active(p->stream) > 0) busy = 1;
            }
            if (g_tone_stream != NULL && fka_eas_active(g_tone_stream) > 0) busy = 1;
            if (busy) fka_eas_render(scratch);
        }
        for (i = 0; i < MAX_TONES; ++i) g_tones[i].left = 0;
    }
    if (fka_eas_set_engine(engine) != 0) return;
    for (i = 0; i < MAX_PLAYERS; ++i) {
        Player *p = g_players[i];
        if (p != NULL && p->stream != NULL) fka_eas_stream_engine(p->stream, engine);
    }
    if (g_tone_stream != NULL) fka_eas_stream_engine(g_tone_stream, engine);
}

int fka_get_synth(void)
{
    return g_engine;
}

int fka_load_soundbank(const char *path)
{
    return fka_eas_load_bank(path);
}

/* ------------------------------------------------------------------ */
/* players                                                             */
/* ------------------------------------------------------------------ */

static int add_player(Player *p)
{
    int i;
    for (i = 0; i < MAX_PLAYERS; ++i) {
        if (g_players[i] == NULL) {
            p->id = i + 1;
            p->loop_count = 1;
            p->stop_time = -1;
            p->rate = 100000;
            p->level = 100;
            p->gain = 0x7fff;
            g_players[i] = p;
            return p->id;
        }
    }
    return -1;
}

int fka_player_new(const uint8_t *data, int len, const char *type_hint)
{
    Player *p = (Player *) calloc(1, sizeof(Player));
    int id;
    if (p == NULL) return -1;
    if (fka_decode(data, len, type_hint, &p->c) != 0 || p->c.kind == FKA_KIND_NONE) {
        fka_content_free(&p->c);
        free(p);
        return -2;
    }
    p->kind = p->c.kind;
    p->type = p->c.type;
    fka_lock();
    id = add_player(p);
    fka_unlock();
    if (id < 0) {
        fka_content_free(&p->c);
        free(p);
    }
    return id;
}

int fka_player_new_device(void)
{
    Player *p = (Player *) calloc(1, sizeof(Player));
    int id;
    if (p == NULL) return -1;
    p->kind = FKA_KIND_DEVICE;
    p->type = "audio/midi";
    fka_lock();
    id = add_player(p);
    fka_unlock();
    if (id < 0) free(p);
    return id;
}

int fka_player_new_tone_sequence(const uint8_t *seq, int len)
{
    Player *p = (Player *) calloc(1, sizeof(Player));
    int id;
    if (p == NULL) return -1;
    p->kind = FKA_KIND_SONG;
    p->type = "audio/x-tone-seq";
    p->c.kind = FKA_KIND_SONG;
    fka_song_init(&p->c.song);
    if (seq != NULL && len > 0 && fka_parse_tone_sequence(seq, len, &p->c.song) != 0) {
        free(p);
        return -2;
    }
    fka_lock();
    id = add_player(p);
    fka_unlock();
    if (id < 0) {
        fka_content_free(&p->c);
        free(p);
    }
    return id;
}

int fka_player_replace_tone_sequence(int id, const uint8_t *seq, int len)
{
    FkaSong song, old;
    Player *p;
    if (fka_parse_tone_sequence(seq, len, &song) != 0) return -1;
    fka_lock();
    p = get(id);
    if (p == NULL || p->kind != FKA_KIND_SONG) {
        fka_unlock();
        fka_song_free(&song);
        return -2;
    }
    old = p->c.song;
    p->c.song = song;
    p->next_ev = 0;
    p->song_us = 0;
    p->at_end = 0;
    if (p->stream != NULL) fka_eas_reset(p->stream);
    fka_unlock();
    fka_song_free(&old);
    return 0;
}

void fka_player_delete(int id)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        if (p->stream != NULL) {
            fka_eas_silence(p->stream, 1);
            fka_eas_close(p->stream);
        }
        g_players[id - 1] = NULL;
    }
    fka_unlock();
    if (p != NULL) {
        fka_content_free(&p->c);
        free(p);
    }
}

int fka_player_kind(int id)
{
    Player *p = get(id);
    return p != NULL ? p->kind : FKA_KIND_NONE;
}

const char *fka_player_content_type(int id)
{
    Player *p = get(id);
    return p != NULL && p->type != NULL ? p->type : "audio/unknown";
}

int fka_player_prefetch(int id)
{
    int r = 0;
    Player *p;
    fka_lock();
    p = get(id);
    if (p == NULL) r = -1;
    else if (p->kind == FKA_KIND_SONG || p->kind == FKA_KIND_DEVICE) {
        if (p->stream == NULL) {
            r = ensure_stream(p);
            if (r == 0 && p->kind == FKA_KIND_SONG) song_chase(p, p->song_us);
        }
    }
    fka_unlock();
    return r;
}

void fka_player_deallocate(int id)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        p->playing = 0;
        if (p->stream != NULL) {
            fka_eas_silence(p->stream, 1);
            fka_eas_close(p->stream);
            p->stream = NULL;
        }
    }
    fka_unlock();
}

int fka_player_start(int id)
{
    int r = 0;
    Player *p;
    fka_lock();
    p = get(id);
    if (p == NULL) {
        fka_unlock();
        return -1;
    }
    if (p->kind == FKA_KIND_SONG || p->kind == FKA_KIND_DEVICE) {
        int fresh = p->stream == NULL;
        /* Out of synth streams (EAS has 16): a song still plays silently so
         * its timing and END_OF_MEDIA behave; games often wait for those. */
        if (ensure_stream(p) != 0 && p->kind == FKA_KIND_DEVICE) r = -2;
        else if (p->kind == FKA_KIND_SONG) {
            if (p->at_end || p->song_us >= (double) p->c.song.duration) {
                p->song_us = 0;
                p->next_ev = 0;
                fresh = 1;
            }
            if (fresh) song_chase(p, p->song_us);
        }
    } else if (p->kind == FKA_KIND_PCM) {
        if (p->at_end || (p->c.pcm.length >= 0 && (int64_t) (p->pos >> 32) >= p->c.pcm.length))
            p->pos = 0;
        p->fade_target = 0x7fff;
    }
    if (r == 0 && p->kind != FKA_KIND_DEVICE) {
        p->at_end = 0;
        p->playing = 1;
    }
    fka_unlock();
    return r;
}

void fka_player_stop(int id)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL && p->playing) {
        p->playing = 0;
        if (p->kind == FKA_KIND_SONG) {
            int i;
            notes_off(p);
            if (p->stream != NULL) fka_eas_silence(p->stream, 0);
            for (i = 0; i < MAX_SUBWAVES; ++i) p->sub[i].w = NULL;
        } else {
            p->fade_target = 0;
        }
    }
    fka_unlock();
}

int fka_player_is_playing(int id)
{
    Player *p = get(id);
    return p != NULL && p->playing;
}

void fka_player_set_loop_count(int id, int count)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        p->loop_count = count == 0 ? 1 : count;
        p->loops_done = 0;
    }
    fka_unlock();
}

int64_t fka_player_get_time(int id)
{
    int64_t t = 0;
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        t = media_time(p);
        if (p->at_end) {
            int64_t d = duration(p);
            if (d >= 0) t = d;
        }
    }
    fka_unlock();
    return t;
}

int64_t fka_player_set_time(int id, int64_t us)
{
    int64_t d, r = 0;
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        d = duration(p);
        if (us < 0) us = 0;
        if (d >= 0 && us > d) us = d;
        p->at_end = 0;
        if (p->kind == FKA_KIND_PCM) {
            p->pos = (uint64_t) (us * p->c.pcm.rate / 1000000) << 32;
        } else if (p->kind == FKA_KIND_SONG) {
            if (p->stream != NULL) notes_off(p);
            song_chase(p, (double) us);
        }
        r = media_time(p);
    }
    fka_unlock();
    return r;
}

int64_t fka_player_get_duration(int id)
{
    Player *p = get(id);
    return p != NULL ? duration(p) : -1;
}

void fka_player_set_volume(int id, int level, int muted)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        p->level = level < 0 ? 0 : (level > 100 ? 100 : level);
        p->muted = muted;
        apply_gain(p);
    }
    fka_unlock();
}

void fka_player_set_stop_time(int id, int64_t us)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) p->stop_time = us;
    fka_unlock();
}

int fka_player_set_rate(int id, int milli_percent)
{
    Player *p;
    int r = 100000;
    if (milli_percent < 10000) milli_percent = 10000;
    if (milli_percent > 400000) milli_percent = 400000;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        p->rate = milli_percent;
        r = p->rate;
    }
    fka_unlock();
    return r;
}

int fka_player_get_rate(int id)
{
    Player *p = get(id);
    return p != NULL ? p->rate : 100000;
}

int fka_player_get_tempo(int id)
{
    Player *p = get(id);
    uint32_t us;
    if (p == NULL || p->kind != FKA_KIND_SONG) return 120000;
    us = fka_song_tempo_at(&p->c.song, (uint32_t) p->song_us);
    if (us == 0) us = 500000;
    return (int) ((60000000000.0 / us) * p->rate / 100000.0);
}

int fka_player_set_tempo(int id, int milli_bpm)
{
    Player *p = get(id);
    uint32_t us;
    double base;
    if (p == NULL || p->kind != FKA_KIND_SONG || milli_bpm <= 0) return 120000;
    us = fka_song_tempo_at(&p->c.song, (uint32_t) p->song_us);
    if (us == 0) us = 500000;
    base = 60000000000.0 / us;
    fka_player_set_rate(id, (int) (milli_bpm / base * 100000.0));
    return fka_player_get_tempo(id);
}

void fka_player_set_pitch(int id, int milli_semitones)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL) {
        p->pitch = (milli_semitones >= 0 ? milli_semitones + 500 : milli_semitones - 500) / 1000;
        if (p->stream != NULL) fka_eas_transpose(p->stream, p->pitch);
    }
    fka_unlock();
}

/* ------------------------------------------------------------------ */
/* MIDIControl, tones, events                                          */
/* ------------------------------------------------------------------ */

void fka_midi_short(int id, int status, int d1, int d2)
{
    Player *p;
    uint8_t m[3];
    int n;
    fka_lock();
    p = get(id);
    if (p != NULL && (p->kind == FKA_KIND_DEVICE || p->kind == FKA_KIND_SONG) &&
        ensure_stream(p) == 0) {
        n = ((status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0) ? 2 : 3;
        m[0] = (uint8_t) status;
        m[1] = (uint8_t) (d1 & 0x7f);
        m[2] = (uint8_t) (d2 & 0x7f);
        fka_eas_write(p->stream, m, n);
    }
    fka_unlock();
}

void fka_midi_long(int id, const uint8_t *data, int len)
{
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL && len > 0 && (p->kind == FKA_KIND_DEVICE || p->kind == FKA_KIND_SONG) &&
        ensure_stream(p) == 0)
        fka_eas_write(p->stream, data, len);
    fka_unlock();
}

int fka_midi_get_channel_volume(int id, int channel)
{
    int v = -1;
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL && p->stream != NULL) v = fka_eas_channel_volume(p->stream, channel);
    fka_unlock();
    return v;
}

int fka_midi_get_program(int id, int channel, int *bank)
{
    int v = -1;
    Player *p;
    fka_lock();
    p = get(id);
    if (p != NULL && p->stream != NULL) v = fka_eas_channel_program(p->stream, channel, bank);
    fka_unlock();
    return v;
}

void fka_play_tone(int note, int duration_ms, int volume)
{
    int i, slot = 0;
    uint8_t m[3];
    if (!g_ready || note < 0 || note > 127 || duration_ms <= 0) return;
    fka_lock();
    if (g_tone_stream == NULL) {
        g_tone_stream = fka_eas_open();
        if (g_tone_stream != NULL) {
            int ch;
            fka_eas_reset(g_tone_stream);
            for (ch = 0; ch < 16; ++ch) {
                m[0] = (uint8_t) (0xc0 | ch);
                m[1] = 80;   /* square lead: the phone beeper */
                fka_eas_write(g_tone_stream, m, 2);
            }
        }
    }
    if (g_tone_stream != NULL) {
        for (i = 0; i < MAX_TONES; ++i) {
            if (g_tones[i].left <= 0) {
                slot = i;
                break;
            }
        }
        g_tone_channel = (g_tone_channel + 1) % 16;
        if (g_tone_channel == 9) g_tone_channel = 10;
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        m[0] = (uint8_t) (0x90 | g_tone_channel);
        m[1] = (uint8_t) note;
        m[2] = (uint8_t) (volume * 127 / 100);
        if (m[2] > 0) {
            fka_eas_write(g_tone_stream, m, 3);
            g_tones[slot].ch = g_tone_channel;
            g_tones[slot].note = note;
            g_tones[slot].left = (int) ((int64_t) duration_ms * FKA_RATE / 1000);
        }
    }
    fka_unlock();
}

int fka_poll_event(int *player, int *type, int64_t *arg, int *data)
{
    int r = 0;
    fka_lock();
    if (g_ev_tail != g_ev_head) {
        Event *e = &g_events[g_ev_tail];
        *player = e->player;
        *type = e->type;
        *arg = e->arg;
        *data = e->data;
        g_ev_tail = (g_ev_tail + 1) % EVENT_RING;
        r = 1;
    }
    fka_unlock();
    return r;
}
