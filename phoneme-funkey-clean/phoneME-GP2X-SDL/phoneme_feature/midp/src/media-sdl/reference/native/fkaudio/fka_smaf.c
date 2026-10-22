/*
 * FunKey audio engine: Yamaha SMAF (.mmf) -> FkaSong.
 *
 * SMAF is the ringtone format of the Yamaha MA-1/2/3/5 chips used by
 * Samsung, LG, Sharp, Panasonic... (com.samsung.util.AudioClip TYPE_MMF).
 * The score is converted to General MIDI for the EAS synthesizer:
 *
 *   HandyPhone Standard (MA-1/2, format 0x00): 4 channels per track,
 *     2-byte events, octave-shift state, short/long control events.
 *   Mobile Standard (MA-3/5, 0x01 Huffman packed / 0x02 plain): MIDI-like
 *     status bytes with note gate times and running velocity.
 *   Audio tracks (ATR) and stream PCM voices (Mtsp + PCM voice exclusive):
 *     Yamaha 4-bit ADPCM decoded once and fired as one-shot waves.
 *
 * Written from the SMAF format description, cross-checked against the
 * behaviour of akustikrausch/yamaha-smaf-player (Apache-2.0) and vavi-sound.
 * Custom FM voice definitions cannot be reproduced on a GM synth; those
 * channels use the GM program they select, as phones without the chip did.
 */
#include "fka_song.h"

#include <stdlib.h>
#include <string.h>

#define SMAF_MAX_EVENTS 400000
#define SMAF_MAX_VOICES 128
#define SMAF_MAX_WAVES  128
#define RHYTHM_BANK     0x78
#define MELODY_BANK     0x79

typedef struct {
    uint32_t t;
    uint8_t kind;       /* 0 short MIDI, 1 note on+off pair, 2 wave */
    uint8_t ch;
    uint8_t a, b;       /* note/cc/program, velocity/value */
    uint8_t c;          /* status high nibble for short events */
    uint32_t len;       /* note gate (us) or wave index */
    int8_t semis;
    uint32_t seq;       /* decode order, keeps equal-time order stable */
} SEv;

typedef struct {
    int msb, lsb, pc, drum_note, wave_id, fs;
} PcmVoice;

typedef struct {
    SEv *ev;
    int count, cap;
    PcmVoice voices[SMAF_MAX_VOICES];
    int voice_count;
    int wave_index[SMAF_MAX_WAVES];     /* SMAF wave number -> song wave */
    int bank_msb[16], bank_lsb[16], program[16];
    int rhythm[16];
    int drum[16];       /* currently on a drum bank */
    int oct_shift[16];
    int fail;
} Smaf;

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3];
}

static int id3(const uint8_t *p, const char *s)
{
    return p[0] == (uint8_t) s[0] && p[1] == (uint8_t) s[1] && p[2] == (uint8_t) s[2];
}

static int id_sane(const uint8_t *p)
{
    return p[0] >= 0x20 && p[0] <= 0x7e && p[1] >= 0x20 && p[1] <= 0x7e &&
           p[2] >= 0x20 && p[2] <= 0x7e;
}

static double timebase_ms(int raw)
{
    switch (raw & 0x0f) {
    case 0: return 1.0;
    case 1: return 2.0;
    case 3: return 5.0;
    default: return 4.0;
    }
}

static SEv *sm_push(Smaf *m)
{
    if (m->count >= SMAF_MAX_EVENTS) {
        m->fail = 1;
        return NULL;
    }
    if (m->count == m->cap) {
        int cap = m->cap ? m->cap * 2 : 512;
        SEv *ev = (SEv *) realloc(m->ev, (size_t) cap * sizeof(SEv));
        if (ev == NULL) {
            m->fail = 1;
            return NULL;
        }
        m->ev = ev;
        m->cap = cap;
    }
    memset(&m->ev[m->count], 0, sizeof(SEv));
    m->ev[m->count].seq = (uint32_t) m->count;
    return &m->ev[m->count++];
}

static void sm_short(Smaf *m, double ms, int st, int ch, int a, int b)
{
    SEv *e = sm_push(m);
    if (e == NULL) return;
    e->t = (uint32_t) (ms * 1000.0);
    e->kind = 0;
    e->c = (uint8_t) st;
    e->ch = (uint8_t) (ch & 15);
    e->a = (uint8_t) (a & 0x7f);
    e->b = (uint8_t) (b & 0x7f);
}

static const PcmVoice *sm_pcm_voice(Smaf *m, int ch, int note)
{
    const PcmVoice *melody = NULL;
    int i;
    for (i = 0; i < m->voice_count; ++i) {
        const PcmVoice *v = &m->voices[i];
        if (v->msb != m->bank_msb[ch] || v->lsb != m->bank_lsb[ch] || v->pc != m->program[ch])
            continue;
        if (v->drum_note) {
            if (v->drum_note == note) return v;
        } else {
            melody = v;
        }
    }
    return melody;
}

static void sm_note(Smaf *m, double ms, int ch, int note, int vel, double gate_ms)
{
    const PcmVoice *pv;
    SEv *e;
    ch &= 15;
    if (note < 0 || note > 127 || gate_ms <= 0.0) return;
    pv = sm_pcm_voice(m, ch, note);
    if (pv != NULL && pv->wave_id >= 0 && pv->wave_id < SMAF_MAX_WAVES &&
        m->wave_index[pv->wave_id] >= 0) {
        e = sm_push(m);
        if (e == NULL) return;
        e->t = (uint32_t) (ms * 1000.0);
        e->kind = 2;
        e->ch = (uint8_t) ch;
        e->b = (uint8_t) vel;
        e->len = (uint32_t) m->wave_index[pv->wave_id];
        e->semis = (int8_t) (pv->drum_note ? 0 : note - 60);
        return;
    }
    e = sm_push(m);
    if (e == NULL) return;
    e->t = (uint32_t) (ms * 1000.0);
    e->kind = 1;
    e->ch = (uint8_t) ch;
    e->a = (uint8_t) note;
    e->b = (uint8_t) (vel ? vel : 1);
    e->len = (uint32_t) (gate_ms * 1000.0);
}

/* Bank select: SMAF marks drum banks with bit 7 (HandyPhone) or 0x7D
 * (Mobile). EAS takes the GM2/SP-MIDI banks 0x78 rhythm / 0x79 melody. */
static void sm_bank(Smaf *m, double ms, int ch, int raw, int is_msb)
{
    int drum = (raw & 0x80) || raw == 0x7d;
    ch &= 15;
    m->drum[ch] = drum || m->rhythm[ch];
    if (is_msb) m->bank_msb[ch] = raw;
    else m->bank_lsb[ch] = raw;
    if (is_msb || (raw & 0x80)) {
        sm_short(m, ms, 0xb0, ch, 0, drum || m->rhythm[ch] ? RHYTHM_BANK : MELODY_BANK);
        sm_short(m, ms, 0xb0, ch, 32, 0);
    }
}

/* ---- voice exclusives (only PCM voices matter for a GM rendition) ---- */

static void sm_exclusive(Smaf *m, const uint8_t *p, uint32_t n)
{
    PcmVoice *v;
    if (n && p[n - 1] == 0xf7) --n;
    if (n < 26 || p[0] != 0x43 || p[1] != 0x79 || (p[2] != 0x06 && p[2] != 0x07) ||
        p[3] != 0x7f || p[4] != 0x01 || p[9] == 0)
        return;
    if (m->voice_count >= SMAF_MAX_VOICES) return;
    v = &m->voices[m->voice_count++];
    v->msb = p[5];
    v->lsb = p[6];
    v->pc = p[7];
    v->drum_note = p[8];
    v->fs = (p[10] << 8) | p[11];
    v->wave_id = p[25] & 0x7f;
    if (v->fs < 2000 || v->fs > 48000) v->fs = 8000;
}

static uint32_t rd_hps(const uint8_t **pp, const uint8_t *end)
{
    const uint8_t *p = *pp;
    uint32_t v;
    if (p >= end) return 0;
    v = *p++;
    if (v & 0x80) {
        if (p >= end) {
            *pp = p;
            return v & 0x7f;
        }
        v = (((v & 0x7f) + 1) << 7) | *p++;
    }
    *pp = p;
    return v;
}

static uint32_t rd_vlq(const uint8_t **pp, const uint8_t *end)
{
    const uint8_t *p = *pp;
    uint32_t v = 0;
    int i;
    for (i = 0; i < 4 && p < end; ++i) {
        uint8_t b = *p++;
        v = (v << 7) | (b & 0x7f);
        if (!(b & 0x80)) break;
    }
    *pp = p;
    return v;
}

static void sm_setup(Smaf *m, const uint8_t *p, uint32_t n, int mobile)
{
    const uint8_t *end = p + n;
    while (p < end) {
        uint32_t len;
        if (*p == 0xff) ++p;
        if (p >= end) break;
        if (*p != 0xf0) {
            ++p;
            continue;
        }
        ++p;
        len = mobile ? rd_vlq(&p, end) : rd_hps(&p, end);
        if (len == 0 || len > (uint32_t) (end - p)) break;
        sm_exclusive(m, p, len);
        p += len;
    }
}

/* ---- Huffman packed Mobile sequences (Okumura tree) ---- */

typedef struct {
    const uint8_t *d;
    uint32_t n, bit;
} HBits;

static int hb_bit(HBits *h)
{
    int b;
    if ((h->bit >> 3) >= h->n) return -1;
    b = (h->d[h->bit >> 3] >> (7 - (h->bit & 7))) & 1;
    ++h->bit;
    return b;
}

static int hb_tree(HBits *h, short *left, short *right, int *next, int depth)
{
    int b = hb_bit(h), i, v = 0, idx;
    if (b < 0 || depth > 256) return -1;
    if (b == 0) {
        for (i = 0; i < 8; ++i) {
            int x = hb_bit(h);
            if (x < 0) return -1;
            v = (v << 1) | x;
        }
        return v;
    }
    if (*next > 510) return -1;
    idx = (*next)++;
    left[idx] = (short) hb_tree(h, left, right, next, depth + 1);
    right[idx] = (short) hb_tree(h, left, right, next, depth + 1);
    if (left[idx] < 0 || right[idx] < 0) return -1;
    return idx;
}

static uint8_t *sm_inflate(const uint8_t *p, uint32_t n, uint32_t *out_len)
{
    HBits h;
    short left[511], right[511];
    int next = 256, root;
    uint32_t total, i;
    uint8_t *out;
    if (n < 5) return NULL;
    total = be32(p);
    if (total == 0 || total > (4u << 20)) return NULL;
    h.d = p + 4;
    h.n = n - 4;
    h.bit = 0;
    root = hb_tree(&h, left, right, &next, 0);
    if (root < 0) return NULL;
    out = (uint8_t *) malloc(total);
    if (out == NULL) return NULL;
    for (i = 0; i < total; ++i) {
        int node = root, guard = 0;
        while (node >= 256 && guard++ < 256) {
            int b = hb_bit(&h);
            if (b < 0) break;
            node = b ? right[node] : left[node];
        }
        if (node >= 256) break;
        out[i] = (uint8_t) node;
    }
    *out_len = i;
    return out;
}

/* ---- HandyPhone Standard ---- */

static void sm_handyphone(Smaf *m, const uint8_t *p, uint32_t n, int base,
                          double tbd, double tbg, int wave_mode)
{
    const uint8_t *end = p + n;
    double ms = 0.0;
    static const uint8_t mod_tab[16] = {
        0, 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30,
        0x38, 0x40, 0x48, 0x50, 0x60, 0x70, 0x7f, 0x7f
    };
    while (p < end && !m->fail) {
        uint8_t e1;
        ms += rd_hps(&p, end) * tbd;
        if (p >= end) break;
        e1 = *p++;
        if (e1 == 0xff) {
            uint8_t e2;
            if (p >= end) break;
            e2 = *p++;
            if (e2 == 0x00) continue;
            if (e2 == 0xf0) {
                uint32_t len = rd_hps(&p, end);
                if (len > (uint32_t) (end - p)) break;
                sm_exclusive(m, p, len);
                p += len;
                continue;
            }
            if (p >= end) break;
            {
                uint8_t l = *p++;
                p = (l > end - p) ? end : p + l;
            }
            continue;
        }
        if (e1 == 0x00) {
            uint8_t e2;
            int ch, cls, data;
            if (p >= end) break;
            e2 = *p++;
            if (e2 == 0x00) {
                if (p >= end || *p++ == 0x00) break;  /* end of sequence */
                continue;
            }
            ch = (base + ((e2 >> 6) & 3)) & 15;
            cls = (e2 >> 4) & 3;
            data = e2 & 0x0f;
            if (cls == 3) {
                uint8_t v;
                if (p >= end) break;
                v = *p++;
                switch (data) {
                case 0x0:
                    m->program[ch] = v & 0x7f;
                    sm_short(m, ms, 0xc0, ch, m->drum[ch] ? 0 : v, 0);
                    break;
                case 0x1:
                    sm_bank(m, ms, ch, v, 1);
                    break;
                case 0x2:
                    if (v >= 0x81 && v <= 0x84) m->oct_shift[ch] = -12 * (v - 0x80);
                    else if (v <= 0x04) m->oct_shift[ch] = 12 * v;
                    break;
                case 0x3:
                    sm_short(m, ms, 0xb0, ch, 1, v);
                    break;
                case 0x4:
                    sm_short(m, ms, 0xe0, ch, 0, v);
                    break;
                case 0x7:
                    sm_short(m, ms, 0xb0, ch, 7, v);
                    break;
                case 0xa:
                    sm_short(m, ms, 0xb0, ch, 10, v);
                    break;
                case 0xb:
                    sm_short(m, ms, 0xb0, ch, 11, v);
                    break;
                default:
                    break;
                }
            } else if (cls == 0) {
                int v = data <= 1 ? 0 : data * 8 + 15;
                sm_short(m, ms, 0xb0, ch, 11, v > 127 ? 127 : v);
            } else if (cls == 1) {
                int msb = data * 8;
                sm_short(m, ms, 0xe0, ch, 0, msb > 127 ? 127 : msb);
            } else {
                sm_short(m, ms, 0xb0, ch, 1, mod_tab[data]);
            }
            continue;
        }
        {
            int ch = (base + ((e1 >> 6) & 3)) & 15;
            int octave = (e1 >> 4) & 3;
            int note = e1 & 0x0f;
            uint32_t gate = rd_hps(&p, end);
            if (gate == 0) continue;
            if (wave_mode) {
                if (note < SMAF_MAX_WAVES && m->wave_index[note] >= 0) {
                    SEv *e = sm_push(m);
                    if (e == NULL) break;
                    e->t = (uint32_t) (ms * 1000.0);
                    e->kind = 2;
                    e->ch = (uint8_t) ch;
                    e->b = 127;
                    e->len = (uint32_t) m->wave_index[note];
                }
                continue;
            }
            sm_note(m, ms, ch, note + octave * 12 + 36 + m->oct_shift[ch], 127, gate * tbg);
        }
    }
}

/* ---- Mobile Standard ---- */

static void sm_mobile(Smaf *m, const uint8_t *p, uint32_t n, double tbd, double tbg)
{
    const uint8_t *end = p + n;
    double ms = 0.0;
    uint8_t run_vel[16];
    memset(run_vel, 64, sizeof(run_vel));
    while (p < end && !m->fail) {
        uint8_t s;
        int ch;
        ms += rd_vlq(&p, end) * tbd;
        if (p >= end) break;
        s = *p++;
        if (s < 0x80) continue;
        ch = s & 0x0f;
        switch (s & 0xf0) {
        case 0x80:
        case 0x90: {
            int note, vel;
            uint32_t gate;
            if (p >= end) break;
            note = *p++ & 0x7f;
            if ((s & 0xf0) == 0x90) {
                if (p >= end) break;
                vel = *p++ & 0x7f;
                run_vel[ch] = (uint8_t) vel;
            } else {
                vel = run_vel[ch];
            }
            gate = rd_vlq(&p, end);
            if (gate) sm_note(m, ms, ch, note, vel, gate * tbg);
            break;
        }
        case 0xa0:
            p = (end - p >= 2) ? p + 2 : end;
            break;
        case 0xb0: {
            int cc, val;
            if (end - p < 2) {
                p = end;
                break;
            }
            cc = *p++;
            val = *p++;
            if (cc == 0x00) sm_bank(m, ms, ch, val, 1);
            else if (cc == 0x20) m->bank_lsb[ch] = val;
            else sm_short(m, ms, 0xb0, ch, cc, val);
            break;
        }
        case 0xc0:
            if (p >= end) break;
            m->program[ch] = *p & 0x7f;
            sm_short(m, ms, 0xc0, ch, m->drum[ch] ? 0 : *p, 0);
            ++p;
            break;
        case 0xd0:
            if (p < end) ++p;
            break;
        case 0xe0:
            if (end - p < 2) {
                p = end;
                break;
            }
            sm_short(m, ms, 0xe0, ch, p[0], p[1]);
            p += 2;
            break;
        default:
            if (s == 0xf0) {
                uint32_t len = rd_vlq(&p, end);
                if (len > (uint32_t) (end - p)) {
                    p = end;
                    break;
                }
                sm_exclusive(m, p, len);
                p += len;
            } else if (s == 0xff) {
                uint8_t mt;
                if (p >= end) break;
                mt = *p++;
                if (mt == 0x2f) p = end;
                else if (mt != 0x00 && p < end) {
                    uint8_t l = *p++;
                    p = (l > end - p) ? end : p + l;
                }
            }
            break;
        }
    }
}

/* ---- ADPCM waves ---- */

static int16_t *sm_adpcm(const uint8_t *d, uint32_t n, int *out_len)
{
    int16_t *out = (int16_t *) malloc((size_t) n * 2 * sizeof(int16_t));
    int last = 0, step = 127;
    uint32_t i;
    int k = 0;
    if (out == NULL) return NULL;
    for (i = 0; i < n; ++i) {
        int nib, half;
        for (half = 0; half < 2; ++half) {
            int delta;
            nib = half ? (d[i] >> 4) & 15 : d[i] & 15;  /* low nibble first */
            delta = step >> 3;
            if (nib & 1) delta += step >> 2;
            if (nib & 2) delta += step >> 1;
            if (nib & 4) delta += step;
            if (nib & 8) delta = -delta;
            last += delta;
            if (last > 32767) last = 32767;
            if (last < -32768) last = -32768;
            switch (nib & 7) {
            case 4: step = step * 307 / 256; break;
            case 5: step = step * 409 / 256; break;
            case 6: step = step * 2; break;
            case 7: step = step * 307 / 128; break;
            default: step = step * 115 / 128; break;
            }
            if (step < 127) step = 127;
            if (step > 24576) step = 24576;
            out[k++] = (int16_t) last;
        }
    }
    *out_len = k;
    return out;
}

static void sm_waves(Smaf *m, FkaSong *song, const uint8_t *p, uint32_t n)
{
    uint32_t pos = 0;
    while (pos + 8 <= n) {
        const uint8_t *c = p + pos;
        uint32_t sz, body = pos + 8;
        if (!id3(c, "Mwa") && !id3(c, "Awa")) {
            ++pos;
            continue;
        }
        sz = be32(c + 4);
        if (sz > n - body) sz = n - body;
        if (sz > 2 && c[3] < SMAF_MAX_WAVES) {
            static const int rates[5] = { 4000, 8000, 11025, 22050, 44100 };
            int r = p[body + 1] & 0x0f, len = 0;
            int16_t *pcm = sm_adpcm(p + body + 2, sz - 2, &len);
            if (pcm != NULL)
                m->wave_index[c[3]] = fka_song_add_wave(song, pcm, len, r < 5 ? rates[r] : 8000);
        }
        pos = body + sz;
    }
}

typedef struct {
    const uint8_t *seq, *setup;
    uint32_t seq_len, setup_len;
    int format, number, tbd, tbg, audio;
    const uint8_t *status;
    int status_len;
} Track;

static void sm_track_chunks(Smaf *m, FkaSong *song, const uint8_t *p, uint32_t n, Track *t)
{
    uint32_t pos = 0;
    while (pos + 8 <= n) {
        const uint8_t *c = p + pos;
        uint32_t sz, body = pos + 8;
        if (!id_sane(c)) {
            ++pos;
            continue;
        }
        sz = be32(c + 4);
        if (sz > n - body) sz = n - body;
        if (id3(c, "Mts") && c[3] == 'u') { t->setup = p + body; t->setup_len = sz; }
        else if ((id3(c, "Mts") || id3(c, "Ats")) && c[3] == 'q') { t->seq = p + body; t->seq_len = sz; }
        else if (id3(c, "Mts") && c[3] == 'p') sm_waves(m, song, p + body, sz);
        else if (id3(c, "Awa")) sm_waves(m, song, c, sz + 8);
        pos = body + sz + (sz == 0 ? 1 : 0);
    }
}

static int sev_cmp(const void *a, const void *b)
{
    const SEv *x = (const SEv *) a, *y = (const SEv *) b;
    int nx = x->kind != 0, ny = y->kind != 0;
    if (x->t != y->t) return x->t < y->t ? -1 : 1;
    if (nx != ny) return nx - ny;        /* controllers before notes */
    return x->seq < y->seq ? -1 : (x->seq > y->seq);
}

int fka_parse_smaf(const uint8_t *data, int len, FkaSong *out)
{
    Smaf *m;
    Track tracks[16];
    int ntracks = 0, i, have_score = 0;
    uint32_t pos, end;
    double score_tb = 0.0;

    if (len < 16 || memcmp(data, "MMMD", 4) != 0) return -1;
    m = (Smaf *) calloc(1, sizeof(Smaf));
    if (m == NULL) return -2;
    for (i = 0; i < SMAF_MAX_WAVES; ++i) m->wave_index[i] = -1;
    fka_song_init(out);

    end = 8 + be32(data + 4);
    if (end > (uint32_t) len || end < 8) end = (uint32_t) len;
    pos = 8;
    while (pos + 8 <= end && ntracks < 16) {
        const uint8_t *c = data + pos;
        uint32_t sz, body = pos + 8;
        if (!id_sane(c)) {
            ++pos;
            continue;
        }
        sz = be32(c + 4);
        if (sz > (uint32_t) len - body) sz = (uint32_t) len - body;
        if ((id3(c, "MTR") || id3(c, "ATR")) && sz >= 4) {
            Track *t = &tracks[ntracks++];
            const uint8_t *b = data + body;
            int hdr;
            memset(t, 0, sizeof(*t));
            t->audio = id3(c, "ATR");
            t->number = c[3];
            t->format = b[0];
            t->tbd = b[2];
            t->tbg = b[3];
            if (t->audio) {
                hdr = 4;
            } else {
                t->status_len = t->format == 0 ? 2 : (t->format == 3 ? 32 : 16);
                hdr = 4 + t->status_len;
                if ((uint32_t) hdr > sz) hdr = (int) sz;
                t->status = b + 4;
            }
            sm_track_chunks(m, out, b + hdr, sz - (uint32_t) hdr, t);
        }
        pos = body + sz + (sz == 0 ? 1 : 0);
    }

    for (i = 0; i < ntracks; ++i) {
        Track *t = &tracks[i];
        if (t->audio) continue;
        if (t->format == 0) {
            int k;
            for (k = 0; k < 4 && k / 2 < t->status_len; ++k) {
                int nib = (k & 1) ? (t->status[k / 2] & 0x0f) : (t->status[k / 2] >> 4);
                m->rhythm[(t->number * 4 + k) & 15] |= (nib & 3) == 3;
            }
        } else {
            int k;
            for (k = 0; k < t->status_len && k < 16; ++k)
                m->rhythm[k] |= (t->status[k] & 3) == 3;
        }
        if (t->setup != NULL) sm_setup(m, t->setup, t->setup_len, t->format != 0);
    }
    for (i = 0; i < 16; ++i) {
        m->drum[i] = m->rhythm[i];
        if (m->rhythm[i]) {
            sm_short(m, 0, 0xb0, i, 0, RHYTHM_BANK);
            sm_short(m, 0, 0xc0, i, 0, 0);
        } else if (i == 9) {
            sm_short(m, 0, 0xb0, i, 0, MELODY_BANK);
            sm_short(m, 0, 0xc0, i, 0, 0);
        }
    }

    for (i = 0; i < ntracks && !m->fail; ++i) {
        Track *t = &tracks[i];
        double tbd = timebase_ms(t->tbd), tbg = timebase_ms(t->tbg);
        if (t->audio || t->seq == NULL) continue;
        if (score_tb == 0.0) score_tb = tbd;
        have_score = 1;
        if (t->format == 0) {
            sm_handyphone(m, t->seq, t->seq_len, t->number * 4, tbd, tbg, 0);
        } else if (t->format == 1) {
            uint32_t n = 0;
            uint8_t *raw = sm_inflate(t->seq, t->seq_len, &n);
            if (raw != NULL) {
                sm_mobile(m, raw, n, tbd, tbg);
                free(raw);
            }
        } else {
            sm_mobile(m, t->seq, t->seq_len, tbd, tbg);
        }
    }
    for (i = 0; i < ntracks && !m->fail; ++i) {
        Track *t = &tracks[i];
        double tb = timebase_ms(t->tbd);
        if (!t->audio || t->seq == NULL) continue;
        if (t->tbd == 0 && score_tb > 0.0) tb = score_tb;
        sm_handyphone(m, t->seq, t->seq_len, 0, tb, tb, 1);
        have_score = 1;
    }

    if (m->fail || !have_score || m->count == 0) {
        free(m->ev);
        free(m);
        fka_song_free(out);
        return -3;
    }

    /* setup ramp: a channel's first bank/program inside 50 ms goes to t=0 */
    {
        int seen[16][2];
        memset(seen, 0, sizeof(seen));
        for (i = 0; i < m->count; ++i) {
            SEv *e = &m->ev[i];
            int k = -1;
            if (e->kind != 0) continue;
            if (e->c == 0xc0) k = 1;
            else if (e->c == 0xb0 && (e->a == 0 || e->a == 32)) k = 0;
            if (k < 0 || seen[e->ch][k]) continue;
            seen[e->ch][k] = 1;
            if (e->t <= 50000) e->t = 0;
        }
    }
    qsort(m->ev, (size_t) m->count, sizeof(SEv), sev_cmp);

    {
        uint32_t last = 0;
        for (i = 0; i < m->count; ++i) {
            SEv *e = &m->ev[i];
            if (e->kind == 0) {
                fka_song_short(out, e->t, e->c | e->ch, e->a, e->b);
                if (e->t > last) last = e->t;
            } else if (e->kind == 1) {
                fka_song_short(out, e->t, 0x90 | e->ch, e->a, e->b);
                fka_song_short(out, e->t + e->len, 0x80 | e->ch, e->a, 0);
                if (e->t + e->len > last) last = e->t + e->len;
            } else {
                const FkaWave *w = &out->waves[e->len];
                uint32_t wl = (uint32_t) ((double) w->len * 1.0e6 / w->rate);
                fka_song_wave(out, e->t, (int) e->len, e->semis, e->b);
                if (e->t + wl > last) last = e->t + wl;
            }
        }
        free(m->ev);
        free(m);
        fka_song_sort(out);
        fka_song_finish(out, last);
    }
    if (out->error) {
        fka_song_free(out);
        return -4;
    }
    return 0;
}
