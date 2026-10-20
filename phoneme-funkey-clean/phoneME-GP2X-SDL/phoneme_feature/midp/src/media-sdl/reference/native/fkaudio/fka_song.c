/*
 * FunKey audio engine: song container and Standard MIDI File parser.
 *
 * The SMF parser is deliberately tolerant: J2ME game MIDIs are often
 * produced by odd converters (wrong track lengths, missing end-of-track,
 * miscounted tracks, RIFF/RMID wrappers, junk before the header, running
 * status after meta events). Everything that can be played is played.
 */
#include "fka_song.h"

#include <stdlib.h>
#include <string.h>

void fka_song_init(FkaSong *s)
{
    memset(s, 0, sizeof(*s));
    s->first_tempo = 500000;
}

void fka_song_free(FkaSong *s)
{
    int i;
    for (i = 0; i < s->wave_count; ++i) free(s->waves[i].pcm);
    free(s->waves);
    free(s->ev);
    free(s->pool);
    fka_song_init(s);
}

static FkaEvent *fka_song_push(FkaSong *s)
{
    if (s->count == s->cap) {
        int cap = s->cap ? s->cap * 2 : 256;
        FkaEvent *ev = (FkaEvent *) realloc(s->ev, (size_t) cap * sizeof(FkaEvent));
        if (ev == NULL) {
            s->error = 1;
            return NULL;
        }
        s->ev = ev;
        s->cap = cap;
    }
    return &s->ev[s->count++];
}

void fka_song_short(FkaSong *s, uint32_t t, int status, int d1, int d2)
{
    FkaEvent *e = fka_song_push(s);
    if (e == NULL) return;
    e->t = t;
    e->kind = FKA_EV_SHORT;
    e->status = (uint8_t) status;
    e->d1 = (uint8_t) (d1 & 0x7f);
    e->d2 = (uint8_t) (d2 & 0x7f);
    e->d = 0;
}

void fka_song_sysex(FkaSong *s, uint32_t t, const uint8_t *data, int len)
{
    FkaEvent *e;
    if (len <= 0 || len > 0xffff) return;
    if (s->pool_len + len > s->pool_cap) {
        int cap = s->pool_cap ? s->pool_cap : 256;
        uint8_t *p;
        while (cap < s->pool_len + len) cap *= 2;
        p = (uint8_t *) realloc(s->pool, (size_t) cap);
        if (p == NULL) {
            s->error = 1;
            return;
        }
        s->pool = p;
        s->pool_cap = cap;
    }
    e = fka_song_push(s);
    if (e == NULL) return;
    memcpy(s->pool + s->pool_len, data, (size_t) len);
    e->t = t;
    e->kind = FKA_EV_SYSEX;
    e->status = 0xf0;
    e->d1 = (uint8_t) (len & 0xff);
    e->d2 = (uint8_t) (len >> 8);
    e->d = (uint32_t) s->pool_len;
    s->pool_len += len;
}

void fka_song_tempo(FkaSong *s, uint32_t t, uint32_t us_per_quarter)
{
    FkaEvent *e;
    if (us_per_quarter == 0) return;
    if (t == 0) s->first_tempo = us_per_quarter;
    e = fka_song_push(s);
    if (e == NULL) return;
    e->t = t;
    e->kind = FKA_EV_TEMPO;
    e->status = 0xff;
    e->d1 = e->d2 = 0;
    e->d = us_per_quarter;
}

int fka_song_add_wave(FkaSong *s, int16_t *pcm, int len, int rate)
{
    FkaWave *w = (FkaWave *) realloc(s->waves, (size_t) (s->wave_count + 1) * sizeof(FkaWave));
    if (w == NULL) {
        free(pcm);
        s->error = 1;
        return -1;
    }
    s->waves = w;
    w[s->wave_count].pcm = pcm;
    w[s->wave_count].len = len;
    w[s->wave_count].rate = rate > 0 ? rate : 8000;
    return s->wave_count++;
}

void fka_song_wave(FkaSong *s, uint32_t t, int wave, int semitones, int gain)
{
    FkaEvent *e;
    if (wave < 0 || wave >= s->wave_count) return;
    e = fka_song_push(s);
    if (e == NULL) return;
    e->t = t;
    e->kind = FKA_EV_WAVE;
    e->status = 0;
    e->d1 = (uint8_t) (semitones + 64);
    e->d2 = (uint8_t) (gain & 0x7f);
    e->d = (uint32_t) wave;
}

/* Stable merge sort by time: equal times keep insertion order. */
static void fka_merge_sort(FkaEvent *a, FkaEvent *tmp, int n)
{
    int mid, i, j, k;
    if (n < 2) return;
    mid = n / 2;
    fka_merge_sort(a, tmp, mid);
    fka_merge_sort(a + mid, tmp, n - mid);
    if (a[mid - 1].t <= a[mid].t) return;
    memcpy(tmp, a, (size_t) n * sizeof(FkaEvent));
    i = 0; j = mid; k = 0;
    while (i < mid && j < n) a[k++] = (tmp[j].t < tmp[i].t) ? tmp[j++] : tmp[i++];
    while (i < mid) a[k++] = tmp[i++];
    while (j < n) a[k++] = tmp[j++];
}

void fka_song_sort(FkaSong *s)
{
    FkaEvent *tmp;
    if (s->count < 2) return;
    tmp = (FkaEvent *) malloc((size_t) s->count * sizeof(FkaEvent));
    if (tmp == NULL) {
        s->error = 1;
        return;
    }
    fka_merge_sort(s->ev, tmp, s->count);
    free(tmp);
}

void fka_song_finish(FkaSong *s, uint32_t end_us)
{
    if (s->count > 0 && s->ev[s->count - 1].t > end_us) end_us = s->ev[s->count - 1].t;
    s->duration = end_us;
    if (s->count < s->cap && s->count > 0) {
        FkaEvent *ev = (FkaEvent *) realloc(s->ev, (size_t) s->count * sizeof(FkaEvent));
        if (ev != NULL) {
            s->ev = ev;
            s->cap = s->count;
        }
    }
}

uint32_t fka_song_tempo_at(const FkaSong *s, uint32_t t)
{
    uint32_t tempo = s->first_tempo;
    int i;
    for (i = 0; i < s->count && s->ev[i].t <= t; ++i) {
        if (s->ev[i].kind == FKA_EV_TEMPO) tempo = s->ev[i].d;
    }
    return tempo;
}

/* ------------------------------------------------------------------ */
/* Standard MIDI File                                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t tick;
    uint32_t seq;        /* file order, keeps same-tick order stable */
    uint8_t kind;
    uint8_t status, d1, d2;
    uint32_t d;          /* tempo, or sysex offset */
    uint16_t len;        /* sysex length */
} SmfRaw;

typedef struct {
    SmfRaw *ev;
    int count, cap;
    uint8_t *pool;
    int pool_len, pool_cap;
    int error;
} SmfList;

static SmfRaw *smf_push(SmfList *l)
{
    if (l->count == l->cap) {
        int cap = l->cap ? l->cap * 2 : 512;
        SmfRaw *ev = (SmfRaw *) realloc(l->ev, (size_t) cap * sizeof(SmfRaw));
        if (ev == NULL) {
            l->error = 1;
            return NULL;
        }
        l->ev = ev;
        l->cap = cap;
    }
    memset(&l->ev[l->count], 0, sizeof(SmfRaw));
    l->ev[l->count].seq = (uint32_t) l->count;
    return &l->ev[l->count++];
}

static uint32_t rd_be32(const uint8_t *p)
{
    return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3];
}

static int rd_vlq(const uint8_t *p, int end, int *pos, uint32_t *out)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 4; ++i) {
        uint8_t c;
        if (*pos >= end) return -1;
        c = p[(*pos)++];
        v = (v << 7) | (c & 0x7f);
        if ((c & 0x80) == 0) {
            *out = v;
            return 0;
        }
    }
    return -1;
}

static int smf_data_bytes(int status)
{
    switch (status & 0xf0) {
    case 0xc0: case 0xd0: return 1;
    default: return 2;
    }
}

/* Parses one MTrk body into the list; returns the track's last tick. */
static uint32_t smf_track(const uint8_t *p, int start, int end, uint32_t tick0, SmfList *l)
{
    int pos = start;
    uint32_t tick = tick0;
    int running = 0;
    while (pos < end && !l->error) {
        uint32_t delta;
        int st;
        if (rd_vlq(p, end, &pos, &delta) != 0) break;
        tick += delta;
        if (pos >= end) break;
        st = p[pos];
        if (st & 0x80) {
            ++pos;
        } else {
            if (running == 0) {
                /* stray data byte without running status: resync */
                ++pos;
                continue;
            }
            st = running;
        }
        if (st < 0xf0) {
            SmfRaw *e;
            int n = smf_data_bytes(st);
            uint8_t d1, d2 = 0;
            running = st;
            if (pos + n > end) break;
            d1 = p[pos] & 0x7f;
            if (n == 2) d2 = p[pos + 1] & 0x7f;
            pos += n;
            e = smf_push(l);
            if (e == NULL) break;
            e->tick = tick;
            e->kind = FKA_EV_SHORT;
            e->status = (uint8_t) st;
            e->d1 = d1;
            e->d2 = d2;
        } else if (st == 0xff) {
            uint32_t len;
            int type;
            if (pos >= end) break;
            type = p[pos++];
            if (rd_vlq(p, end, &pos, &len) != 0) break;
            if (len > (uint32_t) (end - pos)) len = (uint32_t) (end - pos);
            if (type == 0x51 && len >= 3) {
                SmfRaw *e = smf_push(l);
                if (e == NULL) break;
                e->tick = tick;
                e->kind = FKA_EV_TEMPO;
                e->d = ((uint32_t) p[pos] << 16) | ((uint32_t) p[pos + 1] << 8) | p[pos + 2];
            }
            pos += (int) len;
            if (type == 0x2f) break;  /* end of track */
            /* running status is kept: many converters rely on it */
        } else if (st == 0xf0 || st == 0xf7) {
            uint32_t len;
            if (rd_vlq(p, end, &pos, &len) != 0) break;
            if (len > (uint32_t) (end - pos)) len = (uint32_t) (end - pos);
            if (st == 0xf0 && len > 0 && len < 0xfff0) {
                /* store as F0 <payload>; payload normally ends with F7 */
                if (l->pool_len + (int) len + 2 > l->pool_cap) {
                    int cap = l->pool_cap ? l->pool_cap : 256;
                    uint8_t *np;
                    while (cap < l->pool_len + (int) len + 2) cap *= 2;
                    np = (uint8_t *) realloc(l->pool, (size_t) cap);
                    if (np == NULL) {
                        l->error = 1;
                        break;
                    }
                    l->pool = np;
                    l->pool_cap = cap;
                }
                {
                    SmfRaw *e = smf_push(l);
                    int n = (int) len;
                    if (e == NULL) break;
                    e->tick = tick;
                    e->kind = FKA_EV_SYSEX;
                    e->d = (uint32_t) l->pool_len;
                    l->pool[l->pool_len++] = 0xf0;
                    memcpy(l->pool + l->pool_len, p + pos, (size_t) n);
                    l->pool_len += n;
                    if (p[pos + n - 1] != 0xf7) l->pool[l->pool_len++] = 0xf7;
                    e->len = (uint16_t) (l->pool_len - (int) e->d);
                }
            }
            pos += (int) len;
        } else {
            /* system common / realtime bytes are not valid in files */
            if (st == 0xf2) pos += 2;
            else if (st == 0xf1 || st == 0xf3) pos += 1;
        }
    }
    return tick;
}

static int smf_cmp(const void *a, const void *b)
{
    const SmfRaw *x = (const SmfRaw *) a;
    const SmfRaw *y = (const SmfRaw *) b;
    if (x->tick != y->tick) return x->tick < y->tick ? -1 : 1;
    if (x->seq != y->seq) return x->seq < y->seq ? -1 : 1;
    return 0;
}

int fka_parse_smf(const uint8_t *data, int len, FkaSong *out)
{
    SmfList l;
    int pos, i, format, ntrk, division, tracks = 0;
    uint32_t end_tick = 0, cur_tick = 0, tempo = 500000;
    uint64_t cur_us = 0;
    uint32_t smpte_us_per_tick = 0;

    /* RIFF RMID wrapper or junk in front of the header */
    pos = -1;
    for (i = 0; i + 14 <= len && i < 4096; ++i) {
        if (data[i] == 'M' && memcmp(data + i, "MThd", 4) == 0) {
            pos = i;
            break;
        }
    }
    if (pos < 0) return -1;
    {
        uint32_t hl = rd_be32(data + pos + 4);
        format = (data[pos + 8] << 8) | data[pos + 9];
        ntrk = (data[pos + 10] << 8) | data[pos + 11];
        division = (data[pos + 12] << 8) | data[pos + 13];
        if (hl < 6 || hl > 64) hl = 6;
        pos += 8 + (int) hl;
    }
    (void) ntrk;
    if (division & 0x8000) {
        int fps = -(signed char) (division >> 8);
        int tpf = division & 0xff;
        double f = fps == 29 ? 29.97 : (double) fps;
        if (fps <= 0 || tpf <= 0) return -2;
        smpte_us_per_tick = (uint32_t) (1000000.0 / (f * tpf) + 0.5);
    } else if (division == 0) {
        division = 96;
    }

    memset(&l, 0, sizeof(l));
    while (pos + 8 <= len && !l.error) {
        uint32_t clen = rd_be32(data + pos + 4);
        int body = pos + 8;
        int bend = (clen > (uint32_t) (len - body)) ? len : body + (int) clen;
        if (memcmp(data + pos, "MTrk", 4) == 0) {
            uint32_t t0 = (format == 2) ? end_tick : 0;
            uint32_t last = smf_track(data, body, bend, t0, &l);
            if (last > end_tick) end_tick = last;
            ++tracks;
            pos = bend;
        } else if (data[pos] >= 'A' && data[pos] <= 'z') {
            pos = bend;  /* unknown chunk: skip */
        } else {
            /* garbage: look for the next track header */
            int k;
            for (k = pos + 1; k + 8 <= len; ++k)
                if (data[k] == 'M' && memcmp(data + k, "MTrk", 4) == 0) break;
            pos = k;
        }
    }
    if (l.error || tracks == 0) {
        free(l.ev);
        free(l.pool);
        return l.error ? -3 : -4;
    }
    qsort(l.ev, (size_t) l.count, sizeof(SmfRaw), smf_cmp);

    fka_song_init(out);
    for (i = 0; i < l.count; ++i) {
        SmfRaw *e = &l.ev[i];
        uint32_t t;
        if (smpte_us_per_tick) {
            t = (uint32_t) ((uint64_t) e->tick * smpte_us_per_tick);
        } else {
            cur_us += (uint64_t) (e->tick - cur_tick) * tempo / (uint32_t) division;
            cur_tick = e->tick;
            t = (uint32_t) cur_us;
        }
        if (e->kind == FKA_EV_TEMPO) {
            if (!smpte_us_per_tick) tempo = e->d ? e->d : tempo;
            fka_song_tempo(out, t, e->d);
        } else if (e->kind == FKA_EV_SYSEX) {
            fka_song_sysex(out, t, l.pool + e->d, e->len);
        } else {
            fka_song_short(out, t, e->status, e->d1, e->d2);
        }
    }
    {
        uint32_t end_us;
        if (smpte_us_per_tick) end_us = (uint32_t) ((uint64_t) end_tick * smpte_us_per_tick);
        else end_us = (uint32_t) (cur_us + (uint64_t) (end_tick - cur_tick) * tempo / (uint32_t) division);
        fka_song_finish(out, end_us);
    }
    free(l.ev);
    free(l.pool);
    if (out->error) {
        fka_song_free(out);
        return -3;
    }
    return 0;
}
