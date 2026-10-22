/*
 * FunKey audio engine: monophonic ringtone formats -> FkaSong.
 *
 *   JSR-135 tone sequences (ToneControl / audio/x-tone-seq)
 *   Nokia Smart Messaging ringing tones (OTA, com.nokia.mid.sound FORMAT_TONE)
 *   iMelody (EMS / Sony Ericsson, BEGIN:IMELODY)
 *   RTTTL (Nokia text ringtones, "name:d=4,o=5,b=100:...")
 *
 * Note numbering, default octaves, articulation and velocity curves follow
 * the Sonivox EAS parsers, which shipped in real handsets, so melodies come
 * out at the pitch and loudness phones used.
 */
#include "fka_song.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define RING_CHANNEL 0
#define RING_PROGRAM 80     /* GM "Lead 1 (square)": phone buzzer voice */

typedef struct {
    FkaSong *s;
    double t;               /* microseconds */
    int open_note;          /* currently sounding note or -1 */
} Mono;

static void mono_start(Mono *m, FkaSong *s, int program)
{
    fka_song_init(s);
    m->s = s;
    m->t = 0.0;
    m->open_note = -1;
    fka_song_short(s, 0, 0xc0 | RING_CHANNEL, program, 0);
    fka_song_short(s, 0, 0xb0 | RING_CHANNEL, 7, 127);
}

/* Plays key for dur_us, of which the last rest_us are silent. */
static void mono_note(Mono *m, int key, int vel, double dur_us, double rest_us)
{
    uint32_t on = (uint32_t) m->t;
    if (key >= 0 && key < 128 && vel > 0 && dur_us > rest_us) {
        uint32_t off = (uint32_t) (m->t + dur_us - rest_us);
        if (off <= on) off = on + 1;
        fka_song_short(m->s, on, 0x90 | RING_CHANNEL, key, vel > 127 ? 127 : vel);
        fka_song_short(m->s, off, 0x80 | RING_CHANNEL, key, 0);
    }
    m->t += dur_us;
}

static int mono_end(Mono *m)
{
    fka_song_sort(m->s);
    fka_song_finish(m->s, (uint32_t) m->t);
    if (m->s->error || m->s->count <= 2) {
        fka_song_free(m->s);
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* JSR-135 tone sequence                                               */
/* ------------------------------------------------------------------ */

#define TS_VERSION     (-2)
#define TS_TEMPO       (-3)
#define TS_RESOLUTION  (-4)
#define TS_BLOCK_START (-5)
#define TS_BLOCK_END   (-6)
#define TS_PLAY_BLOCK  (-7)
#define TS_SET_VOLUME  (-8)
#define TS_REPEAT      (-9)
#define TS_SILENCE     (-1)

typedef struct {
    const signed char *d;
    int n;                  /* number of bytes */
    int block_start[128];
    int block_end[128];
    double unit_us;         /* duration of one resolution unit */
    int volume;             /* 0..100 */
    int depth;
    int events;             /* expansion guard */
    Mono m;
} ToneSeq;

static void ts_note(ToneSeq *ts, int note, int dur)
{
    double us = dur * ts->unit_us;
    if (note == TS_SILENCE) {
        ts->m.t += us;
    } else {
        /* a short gap keeps repeated pitches articulated */
        double gap = us / 8.0;
        if (gap > 15000.0) gap = 15000.0;
        mono_note(&ts->m, note, (ts->volume * 127 + 50) / 100, us, gap);
    }
}

static int ts_play(ToneSeq *ts, int from, int to)
{
    int i = from;
    if (++ts->depth > 32) return -1;
    while (i + 1 < to) {
        int a = ts->d[i], b = ts->d[i + 1];
        if (++ts->events > 200000) return -1;
        if (a >= TS_SILENCE) {
            if (b <= 0) return -1;
            ts_note(ts, a, b);
            i += 2;
        } else if (a == TS_SET_VOLUME) {
            ts->volume = b < 0 ? 0 : (b > 100 ? 100 : b);
            i += 2;
        } else if (a == TS_REPEAT) {
            int k;
            if (b < 2 || i + 3 >= to) return -1;
            for (k = 0; k < b; ++k) {
                if (ts->d[i + 2] < TS_SILENCE || ts->d[i + 3] <= 0) return -1;
                ts_note(ts, ts->d[i + 2], ts->d[i + 3]);
            }
            i += 4;
        } else if (a == TS_PLAY_BLOCK) {
            if (b < 0 || ts->block_start[b] < 0 || ts->block_end[b] < 0) return -1;
            if (ts_play(ts, ts->block_start[b], ts->block_end[b]) != 0) return -1;
            i += 2;
        } else if (a == TS_BLOCK_START || a == TS_BLOCK_END ||
                   a == TS_TEMPO || a == TS_RESOLUTION || a == TS_VERSION) {
            i += 2;  /* structure already indexed */
        } else {
            return -1;
        }
    }
    --ts->depth;
    return 0;
}

int fka_parse_tone_sequence(const uint8_t *data, int len, FkaSong *out)
{
    ToneSeq ts;
    int i, tempo = 30, resolution = 64, open = -1, main_start = -1;
    memset(&ts, 0, sizeof(ts));
    ts.d = (const signed char *) data;
    ts.n = len & ~1;
    if (ts.n < 4 || ts.d[0] != TS_VERSION || ts.d[1] != 1) return -1;
    for (i = 0; i < 128; ++i) ts.block_start[i] = ts.block_end[i] = -1;

    /* index header and block structure */
    for (i = 2; i + 1 < ts.n; i += 2) {
        int a = ts.d[i], b = ts.d[i + 1];
        if (a == TS_TEMPO && main_start < 0 && open < 0) {
            if (b < 5) return -1;
            tempo = b;
        } else if (a == TS_RESOLUTION && main_start < 0 && open < 0) {
            if (b < 1) return -1;
            resolution = b;
        } else if (a == TS_BLOCK_START) {
            if (b < 0 || open >= 0 || main_start >= 0) return -1;
            open = b;
            ts.block_start[b] = i + 2;
        } else if (a == TS_BLOCK_END) {
            if (b != open) return -1;
            ts.block_end[b] = i;
            open = -1;
        } else if (a == TS_REPEAT) {
            if (open < 0 && main_start < 0) main_start = i;
            i += 2;  /* skip the repeated note */
        } else if (open < 0 && main_start < 0) {
            main_start = i;
        }
    }
    if (open >= 0) return -1;
    if (main_start < 0) main_start = ts.n;

    /* tempo is in units of 4 beats per minute */
    ts.unit_us = 60.0e6 * 4.0 / ((double) tempo * 4.0 * resolution);
    ts.volume = 100;
    mono_start(&ts.m, out, RING_PROGRAM);
    if (ts_play(&ts, main_start, ts.n) != 0) {
        fka_song_free(out);
        return -1;
    }
    return mono_end(&ts.m);
}

/* ------------------------------------------------------------------ */
/* Nokia Smart Messaging (OTA) ringing tone                            */
/* ------------------------------------------------------------------ */

typedef struct {
    const uint8_t *d;
    int len;
    int bit;                /* absolute bit position */
    int bad;
} Bits;

static int bits_get(Bits *b, int n)
{
    int v = 0;
    while (n-- > 0) {
        int byte = b->bit >> 3;
        if (byte >= b->len) {
            b->bad = 1;
            return 0;
        }
        v = (v << 1) | ((b->d[byte] >> (7 - (b->bit & 7))) & 1);
        ++b->bit;
    }
    return v;
}

/* duration of a 1/32 note in microseconds for each OTA tempo index */
static double ota_tick_us(int idx)
{
    static const int bpm[32] = {
        25, 28, 31, 35, 40, 45, 50, 56, 63, 70, 80, 90, 100, 112, 125, 140,
        160, 180, 200, 225, 250, 285, 320, 355, 400, 450, 500, 565, 635, 715, 800, 900
    };
    return 60.0e6 / bpm[idx & 31] / 8.0;
}

typedef struct {
    Mono m;
    double tick;
    int octave, style, velocity;
} OtaState;

static int ota_pattern(Bits *b, OtaState *st, int len)
{
    while (len-- > 0 && !b->bad) {
        int id = bits_get(b, 3);
        if (id == 1) {
            int note = bits_get(b, 4);
            int dur = bits_get(b, 3);
            int spec = bits_get(b, 2);
            double us;
            if (dur > 5) dur = 5;
            us = st->tick * (0x20 >> dur);
            if (spec == 1) us *= 1.5;
            else if (spec == 2) us *= 1.75;
            else if (spec == 3) us = us * 2.0 / 3.0;
            if (note >= 1 && note <= 12) {
                double rest = st->style == 0 ? us / 16.0 : (st->style == 2 ? us / 2.0 : 0.0);
                mono_note(&st->m, st->octave + note, st->velocity, us, rest);
            } else {
                st->m.t += us;
            }
        } else if (id == 2) {
            st->octave = bits_get(b, 2) * 12 + 59;
        } else if (id == 3) {
            st->style = bits_get(b, 2);
        } else if (id == 4) {
            st->tick = ota_tick_us(bits_get(b, 5));
        } else if (id == 5) {
            int v = bits_get(b, 4);
            st->velocity = v ? v * 4 + 67 : 0;
        } else {
            return -1;
        }
    }
    return b->bad ? -1 : 0;
}

int fka_parse_ota(const uint8_t *data, int len, FkaSong *out)
{
    Bits b;
    OtaState st;
    int commands, i, unicode = 0, type, patterns, p;
    int pat_bit[4] = { -1, -1, -1, -1 }, pat_len[4] = { 0, 0, 0, 0 };

    memset(&b, 0, sizeof(b));
    b.d = data;
    b.len = len;
    commands = bits_get(&b, 8);
    if (commands < 1 || commands > 8) return -1;
    for (i = 0; i < commands; ++i) {
        int cmd;
        b.bit = (b.bit + 7) & ~7;            /* each command is byte aligned */
        cmd = bits_get(&b, 7);
        if (i == 0 && cmd != 0x25) return -1;  /* ringing tone programming */
        if (cmd == 0x1d) break;                /* sound: song follows */
        if (cmd == 0x22) unicode = 1;
        else if (i > 0) return -1;
    }
    type = bits_get(&b, 3);
    if (type == 1) {
        int title = bits_get(&b, 4);
        b.bit += title * (unicode ? 16 : 8);
    } else if (type != 2) {
        return -1;
    }
    patterns = bits_get(&b, 8);
    if (b.bad || patterns == 0) return -1;

    memset(&st, 0, sizeof(st));
    st.tick = ota_tick_us(8);   /* 63 bpm */
    st.octave = 59;
    st.velocity = 95;
    mono_start(&st.m, out, RING_PROGRAM);

    for (p = 0; p < patterns && !b.bad; ++p) {
        int hdr = bits_get(&b, 3), id, loops, plen, rep;
        if (hdr != 0) break;
        id = bits_get(&b, 2);
        loops = bits_get(&b, 4);
        plen = bits_get(&b, 8);
        if (loops == 15) loops = 1;  /* "infinite": the Player loops instead */
        if (plen > 0) {
            pat_bit[id] = b.bit;
            pat_len[id] = plen;
            for (rep = 0; rep <= loops; ++rep) {
                b.bit = pat_bit[id];
                if (ota_pattern(&b, &st, plen) != 0) break;
            }
        } else if (pat_bit[id] >= 0) {
            int resume = b.bit;
            for (rep = 0; rep <= loops; ++rep) {
                b.bit = pat_bit[id];
                if (ota_pattern(&b, &st, pat_len[id]) != 0) break;
            }
            b.bit = resume;
        }
    }
    return mono_end(&st.m);
}

/* ------------------------------------------------------------------ */
/* iMelody                                                             */
/* ------------------------------------------------------------------ */

static int imy_match(const char *p, const char *end, const char *kw)
{
    size_t n = strlen(kw);
    return (size_t) (end - p) >= n && strncasecmp(p, kw, n) == 0;
}

int fka_parse_imelody(const uint8_t *data, int len, FkaSong *out)
{
    const char *p = (const char *) data, *end = p + len, *mel = NULL, *mel_end = NULL;
    const char *rep_start = NULL;
    double tick = 60.0e6 / 120.0 / 8.0;     /* 1/32 note at 120 bpm */
    int style = 0, volume = 7, octave = 60, rep_count = -1, guard = 0;
    Mono m;
    static const signed char note_tab[7] = { 9, 11, 0, 2, 4, 5, 7 };

    if (len < 12 || !imy_match(p, end, "BEGIN:IMELODY")) return -1;
    /* header lines */
    while (p < end) {
        const char *line = p, *eol = p;
        while (eol < end && *eol != '\n' && *eol != '\r') ++eol;
        if (imy_match(line, eol, "BEAT:")) {
            int bpm = atoi(line + 5);
            if (bpm >= 25 && bpm <= 900) tick = 60.0e6 / bpm / 8.0;
        } else if (imy_match(line, eol, "STYLE:")) {
            const char *s = line + 6;
            if (s < eol && (*s == 'S' || *s == 's')) ++s;
            if (s < eol && *s >= '0' && *s <= '2') style = *s - '0';
        } else if (imy_match(line, eol, "VOLUME:")) {
            const char *s = line + 7;
            if (s < eol && (*s == 'V' || *s == 'v')) ++s;
            volume = atoi(s);
            if (volume < 0) volume = 0;
            if (volume > 15) volume = 15;
        } else if (imy_match(line, eol, "MELODY:")) {
            mel = line + 7;
            /* the melody may continue on folded lines (leading space) */
            while (eol + 1 < end && (eol[1] == ' ' || eol[1] == '\t' ||
                   (eol[0] == '\r' && eol + 2 < end && eol[1] == '\n' &&
                    (eol[2] == ' ' || eol[2] == '\t')))) {
                ++eol;
                while (eol < end && *eol != '\n' && *eol != '\r') ++eol;
            }
            mel_end = eol;
        } else if (imy_match(line, eol, "END:IMELODY")) {
            break;
        }
        p = eol;
        while (p < end && (*p == '\n' || *p == '\r')) ++p;
    }
    if (mel == NULL) return -1;

    mono_start(&m, out, RING_PROGRAM);
    p = mel;
    while (p < mel_end) {
        char c = *p++;
        int modifier = 0;
        if (++guard > 100000) break;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c == '(') {
            if (rep_start == NULL) {
                rep_start = p;
                rep_count = -1;
            }
            continue;
        }
        if (c == '@') {
            rep_count = atoi(p);
            while (p < mel_end && *p >= '0' && *p <= '9') ++p;
            if (rep_count == 0) rep_count = 1;   /* infinite: Player loops */
            if (rep_count > 16) rep_count = 16;
            continue;
        }
        if (c == ')') {
            if (rep_start != NULL && rep_count > 1) {
                --rep_count;
                p = rep_start;
            } else {
                rep_start = NULL;
                rep_count = -1;
            }
            continue;
        }
        if (c == 'V' || c == 'v') {
            if (p < mel_end && *p == '+') { if (volume < 15) ++volume; ++p; }
            else if (p < mel_end && *p == '-') { if (volume > 0) --volume; ++p; }
            else {
                volume = atoi(p);
                while (p < mel_end && *p >= '0' && *p <= '9') ++p;
                if (volume > 15) volume = 15;
            }
            continue;
        }
        if (c == '*') {
            if (p < mel_end && *p >= '0' && *p <= '8') octave = (*p++ - '0' + 1) * 12;
            continue;
        }
        /* ledon/ledoff, vibeon/vibeoff, backon/backoff: no sound */
        if ((c == 'l' && imy_match(p, mel_end, "edo")) ||
            (c == 'v' && imy_match(p, mel_end, "ibeo")) ||
            (c == 'b' && imy_match(p, mel_end, "acko"))) {
            while (p < mel_end && *p >= 'a' && *p <= 'z') ++p;
            continue;
        }
        if (c == '&' || c == '#') {
            modifier = c == '#' ? 1 : -1;
            if (p >= mel_end) break;
            c = *p++;
        }
        if ((c >= 'a' && c <= 'g') || c == 'r') {
            int d;
            double us;
            if (p >= mel_end || *p < '0' || *p > '5') continue;
            d = *p++ - '0';
            us = tick * (0x20 >> d);
            if (p < mel_end && *p == '.') { us *= 1.5; ++p; }
            else if (p < mel_end && *p == ':') { us *= 1.75; ++p; }
            else if (p < mel_end && *p == ';') { us = us * 2.0 / 3.0; ++p; }
            if (c == 'r') {
                m.t += us;
            } else {
                double rest = style == 0 ? us / 16.0 : (style == 2 ? us / 2.0 : 0.0);
                int key = octave + note_tab[c - 'a'] + modifier;
                mono_note(&m, key, volume ? volume * 4 + 67 : 0, us, rest);
            }
        }
    }
    return mono_end(&m);
}

/* ------------------------------------------------------------------ */
/* RTTTL                                                               */
/* ------------------------------------------------------------------ */

int fka_parse_rtttl(const uint8_t *data, int len, FkaSong *out)
{
    const char *p = (const char *) data, *end = p + len, *colon;
    int def_d = 4, def_o = 6, bpm = 63, guard = 0;
    double whole_us;
    Mono m;
    static const signed char note_tab[7] = { 9, 11, 0, 2, 4, 5, 7 };

    colon = memchr(p, ':', (size_t) len);
    if (colon == NULL || colon - p > 64) return -1;
    p = colon + 1;
    /* defaults section: d=,o=,b= */
    while (p < end && *p != ':') {
        while (p < end && (*p == ' ' || *p == ',')) ++p;
        if (p + 2 < end && p[1] == '=') {
            int v = atoi(p + 2);
            if (*p == 'd' || *p == 'D') { if (v > 0) def_d = v; }
            else if (*p == 'o' || *p == 'O') { if (v >= 3 && v <= 8) def_o = v; }
            else if (*p == 'b' || *p == 'B') { if (v >= 5 && v <= 900) bpm = v; }
        } else if (p < end && *p != ':') {
            return -1;
        }
        while (p < end && *p != ',' && *p != ':') ++p;
    }
    if (p >= end) return -1;
    ++p;
    whole_us = 60.0e6 / bpm * 4.0;

    mono_start(&m, out, RING_PROGRAM);
    while (p < end) {
        int d = 0, o, key = -1;
        double us;
        if (++guard > 10000) break;
        while (p < end && (*p == ' ' || *p == ',' || *p == '\r' || *p == '\n')) ++p;
        if (p >= end) break;
        while (p < end && *p >= '0' && *p <= '9') d = d * 10 + (*p++ - '0');
        if (d <= 0) d = def_d;
        if (p >= end) break;
        {
            char c = (char) (*p | 0x20);
            ++p;
            if (c >= 'a' && c <= 'g') key = note_tab[c - 'a'];
            else if (c != 'p') {
                while (p < end && *p != ',') ++p;
                continue;
            }
        }
        if (p < end && *p == '#') { ++key; ++p; }
        us = whole_us / d;
        if (p < end && *p == '.') { us *= 1.5; ++p; }
        o = def_o;
        if (p < end && *p >= '3' && *p <= '8') o = *p++ - '0';
        if (p < end && *p == '.') { us *= 1.5; ++p; }
        if (key < 0) m.t += us;
        else mono_note(&m, key + (o + 1) * 12, 100, us, us / 16.0);
    }
    return mono_end(&m);
}
