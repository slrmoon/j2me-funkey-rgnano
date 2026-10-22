/*
 * FunKey audio engine: i-mode MLD ("i-melody", DoJa) sequencer and Yamaha
 * MA-3 FM synthesizer.
 *
 * C port of the Keitai Wiki Community Music Implementation (originally
 * written by Guy Perfect, maintained by SquirrelJME / Stephanie Gawroriski):
 * the MLD parser, MLD player and MA-3 operator/note/channel code are public
 * domain (Unlicense); the MA3Sampler parts follow SquirrelJME's MA3Sampler
 * (MPL-2.0). The arithmetic is kept identical to the Java original
 * (float/int widths, operation order) so the output can be compared sample
 * by sample with tools/doja/mldref.
 *
 * Build with -ffp-contract=off -fwrapv: Java float math has no fused
 * multiply-add and int overflow wraps.
 */
#include "fka_mld.h"
#include "fka_ma3_rom.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Java helpers                                                        */
/* ------------------------------------------------------------------ */

/* Math.round(double) */
static double jround(double x)
{
    double f = floor(x);
    return (x - f) >= 0.5 ? f + 1.0 : f;
}

static int jmin(int a, int b) { return a < b ? a : b; }
static int jmax(int a, int b) { return a > b ? a : b; }

/* ------------------------------------------------------------------ */
/* MA-3 tables                                                         */
/* ------------------------------------------------------------------ */

#define MA3_RATE (33868800.0f / 684)
#define A4_MA3 81
#define ENV_ATTACK 0
#define ENV_DECAY 1
#define ENV_SUSTAIN 2
#define ENV_RELEASE 3
#define ENV_DONE 4
#define WAVE_MINUS ((int32_t) 0x80000000)
#define WZERO 0x1000
#define WFULL 0
#define NTS 1
#define NUM_CHANNELS 32

static const int ENV_FLAGS[8] = { 2, 3, 15, 8, 8, 10, 9, 13 };
static const int KSL_B[4] = { 0, 2, 1, 4 };
static const int KSL_F[16] = { 56, 32, 24, 19, 16, 13, 11, 9, 8, 6, 5, 4, 3, 2, 1, 0 };
static const int MULTIS[16] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };
static const int AM_LFO_B[4] = { 8, 18, 26, 31 };
static const int AICA_STEPS[8] = { 230, 230, 230, 230, 307, 409, 512, 614 };

static int AM_LFO_A[52];
static int EXPT[256];
static int SUSTAINS[16];
static int WAVE_ENV[512];
static int32_t WAVES[32][1024];
static double MAGIC_B;
static double MAGIC_F;

typedef struct {
    int ar, dam, dr, dvb, eam, evb, rr, sl, sr, sus, tl, xof;
    int dt, fb, ksl, ksr, multi, ws;
} OpDef;

typedef struct {
    int lfo, panpot, alg, drumKey, ep, fs, isDrum, isForever, isWave, lp, pe, rm, waveId;
    int nops;
    float freqBase, volLeft, volRight, wavAdvance;
    OpDef ops[4];
} Alg;

static Alg g_inst4[MA3_INST4_COUNT];
static Alg g_drum4[MA3_DRUM4_COUNT];
static Alg g_wdrums[61];
static int g_wdrum_set[61];
static int32_t *g_waverom[8];
static int g_waverom_len[8];
static int g_tables_ready;

static int32_t *decode_aica(const uint8_t *a, int off, int len, int *out_len)
{
    int32_t *ret = (int32_t *) malloc((size_t) (len * 2 > 0 ? len * 2 : 1) * sizeof(int32_t));
    int An = 127, Xn = 0, src, dest = 0;
    if (ret == NULL) return NULL;
    for (src = off; src < off + len; src++) {
        int bits = a[src] & 0xFF, n;
        for (n = 0; n < 2; n++, bits >>= 4, dest++) {
            int m = (((bits & 7) << 1) | 1) * An >> 3;
            m = jmin(jmax(m, 0), 32767);
            Xn = jmin(jmax((1 - ((bits & 8) >> 2)) * m + Xn * 254 / 255, -32768), 32767);
            ret[dest] = Xn;
            An = jmin(jmax(AICA_STEPS[bits & 7] * An >> 8, 127), 24576);
        }
    }
    *out_len = len * 2;
    return ret;
}

/* operator from a ROM voice (7 bytes) */
static void op_from_rom(OpDef *o, const uint8_t *ub, int off)
{
    const int8_t *b = (const int8_t *) ub;
    memset(o, 0, sizeof(*o));
    o->sus = (b[off] >> 3 & 1) != 0;
    o->ksr = b[off] >> 2 & 1;
    o->eam = (b[off] >> 1 & 1) != 0;
    o->evb = (b[off] & 1) != 0;
    o->multi = b[off + 1] >> 4 & 15;
    o->dt = b[off + 1] >> 1 & 7;
    o->xof = (b[off + 1] & 1) != 0;
    o->ar = b[off + 2] >> 4 & 15;
    o->dr = b[off + 2] & 15;
    o->sr = b[off + 3] >> 4 & 15;
    o->rr = b[off + 3] & 15;
    o->sl = b[off + 4] >> 4 & 15;
    o->dam = b[off + 4] >> 2 & 3;
    o->dvb = b[off + 4] & 3;
    o->tl = b[off + 5] >> 2 & 63;
    o->ksl = b[off + 5] & 3;
    o->fb = b[off + 6] >> 5 & 7;
    o->ws = b[off + 6] & 31;
}

/* operator of a wave drum (5 bytes) */
static void op_from_wave(OpDef *o, const uint8_t *m, int off)
{
    int bits;
    memset(o, 0, sizeof(*o));
    bits = m[off++];
    o->sr = bits >> 4 & 15;
    o->xof = (bits >> 3 & 1) != 0;
    o->sus = (bits >> 1 & 1) != 0;
    bits = m[off++];
    o->rr = bits >> 4 & 15;
    o->dr = bits & 15;
    bits = m[off++];
    o->ar = bits >> 4 & 15;
    o->sl = bits & 15;
    bits = m[off++];
    o->tl = bits >> 2 & 63;
    bits = m[off++];
    o->dam = bits >> 5 & 3;
    o->eam = (bits >> 4 & 1) != 0;
    o->dvb = bits >> 1 & 3;
    o->evb = (bits & 1) != 0;
}

/* operator of a sysex FM voice (7 bytes) */
static void op_from_sysex(OpDef *o, const uint8_t *m, int off)
{
    int bits;
    memset(o, 0, sizeof(*o));
    bits = m[off++];
    o->sr = bits >> 4 & 15;
    o->xof = (bits >> 3 & 1) != 0;
    o->sus = (bits >> 1 & 1) != 0;
    o->ksr = bits & 1;
    bits = m[off++];
    o->rr = bits >> 4 & 15;
    o->dr = bits & 15;
    bits = m[off++];
    o->ar = bits >> 4 & 15;
    o->sl = bits & 15;
    bits = m[off++];
    o->tl = bits >> 2 & 63;
    o->ksl = bits & 3;
    bits = m[off++];
    o->dam = bits >> 5 & 3;
    o->eam = (bits >> 4 & 1) != 0;
    o->dvb = bits >> 1 & 3;
    o->evb = (bits & 1) != 0;
    bits = m[off++];
    o->multi = bits >> 4 & 15;
    o->dt = bits & 7;
    bits = m[off++];
    o->ws = bits >> 3 & 31;
    o->fb = bits & 7;
}

static void alg_init_post(Alg *a)
{
    if (!a->isWave) {
        int flags = ENV_FLAGS[a->alg], x;
        for (x = 0; !a->isForever && x < a->nops; x++, flags >>= 1) {
            const OpDef *op = &a->ops[x];
            a->isForever = (flags & 1) != 0 && (op->xof ?
                op->sr == 0 || (op->dr == 0 && op->sr != 0) : op->rr == 0);
        }
    }
    a->volRight = a->panpot / (a->panpot <= 15 ? 30.0f : 31.0f);
    a->volLeft = 1 - a->volRight;
}

static void alg_from_rom(Alg *a, const uint8_t *ub, int isDrum)
{
    const int8_t *b = (const int8_t *) ub;
    int x;
    memset(a, 0, sizeof(*a));
    a->lfo = b[0] & 3;
    a->panpot = b[1] >> 3 & 31;
    a->alg = b[1] & 7;
    a->drumKey = b[2] & 127;
    a->nops = a->alg < 2 ? 2 : 4;
    for (x = 0; x < a->nops; x++)
        op_from_rom(&a->ops[x], ub, 3 + x * 7);
    a->freqBase = (float) (440 * pow(2, (a->drumKey - 69) / 12.0));
    a->isDrum = isDrum;
    a->isWave = 0;
    alg_init_post(a);
}

static void alg_from_wave(Alg *a, const uint8_t *m, int off)
{
    int bits;
    memset(a, 0, sizeof(*a));
    a->drumKey = m[off++];
    a->fs = m[off] << 8 | m[off + 1];
    off += 2;
    bits = m[off++];
    a->panpot = bits >> 3 & 31;
    a->pe = (bits & 1) != 0;
    bits = m[off++];
    a->lfo = bits >> 6 & 3;
    a->nops = 1;
    op_from_wave(&a->ops[0], m, off);
    off += 7;
    a->lp = m[off] << 8 | m[off + 1];
    off += 2;
    a->ep = m[off] << 8 | m[off + 1];
    off += 2;
    bits = m[off++];
    a->rm = (bits >> 7 & 1) != 0;
    a->waveId = bits & 7;
    a->isDrum = 1;
    a->isWave = 1;
    a->wavAdvance = a->fs / MA3_RATE;
    alg_init_post(a);
}

/* returns 0 on success, -1 on an operator count mismatch */
static int alg_from_sysex(Alg *a, const uint8_t *m, int len, int off)
{
    int bits, x, type = m[off];
    memset(a, 0, sizeof(*a));
    off += 4;
    bits = m[off++];
    a->panpot = bits >> 3 & 31;
    bits = m[off++];
    a->lfo = bits >> 6 & 3;
    a->pe = (bits >> 5 & 1) != 0;
    a->alg = bits & 7;
    if (a->alg > 1 && type == 0x01) return -1;
    a->nops = a->alg < 2 ? 2 : 4;
    for (x = 0; x < a->nops; x++, off += 7) {
        if (off + 7 > len) return -1;
        op_from_sysex(&a->ops[x], m, off);
    }
    alg_init_post(a);
    return 0;
}

static void tables_init(void)
{
    int x;
    int32_t *saw = WAVES[24], *sin_ = WAVES[0], *tri = WAVES[16], *trp = WAVES[8];
    if (g_tables_ready) return;
    MAGIC_B = 12 / log(2);
    MAGIC_F = 684 / 33868800.0;
    for (x = 0; x < 256; x++) {
        int y;
        EXPT[x] = 1024 | (int) jround((pow(2, (255 - x) / 256.0) - 1) * 1024);
        y = (int) jround(-log(sin((x + 0.5) * M_PI / 256 / 2)) / log(2) * 256);
        sin_[x] = sin_[511 - x] = y;
        sin_[512 + x] = sin_[1023 - x] = y | WAVE_MINUS;
        y = (int) jround(-log((x + 0.5) / 256) / log(2) * 256);
        tri[x] = tri[511 - x] = y;
        tri[512 + x] = tri[1023 - x] = y | WAVE_MINUS;
    }
    for (x = 0; x < 1024; x++) {
        trp[x] = x < 128 ? tri[x << 1] : x < 256 ? WFULL :
            x < 512 ? trp[511 - x] : (trp[1023 - x] | WAVE_MINUS);
    }
    for (x = 0; x < 512; x++) {
        int y = (int) jround(-log((x + 0.5) / 512) / log(2) * 256);
        saw[x] = y;
        saw[1023 - x] = y | WAVE_MINUS;
    }
    for (x = 0; x < 1024; x++) {
        WAVES[1][x] = x < 512 ? sin_[x] : WZERO;
        WAVES[2][x] = sin_[x & 511];
        WAVES[3][x] = (x & 511) < 256 ? sin_[x & 255] : WZERO;
        WAVES[4][x] = x < 512 ? sin_[x << 1] : WZERO;
        WAVES[5][x] = x < 512 ? sin_[x << 1 & 511] : WZERO;
        WAVES[6][x] = x < 512 ? WFULL : WAVE_MINUS;
        WAVES[7][x] = x < 512 ? (EXPT[255 ^ x >> 1] - 1024) << 1 :
            (WAVES[7][1023 - x] | WAVE_MINUS);
        WAVES[9][x] = x < 512 ? trp[x] : WZERO;
        WAVES[10][x] = trp[x & 511];
        WAVES[11][x] = (x & 511) < 256 ? trp[x & 255] : WZERO;
        WAVES[12][x] = x < 512 ? trp[x << 1] : WZERO;
        WAVES[13][x] = x < 512 ? trp[x << 1 & 511] : WZERO;
        WAVES[14][x] = x < 512 ? WFULL : WZERO;
        WAVES[15][x] = WZERO;
        WAVES[17][x] = x < 512 ? tri[x] : WZERO;
        WAVES[18][x] = tri[x & 511];
        WAVES[19][x] = (x & 511) < 256 ? tri[x & 255] : WZERO;
        WAVES[20][x] = x < 512 ? tri[x << 1] : WZERO;
        WAVES[21][x] = x < 512 ? tri[x << 1 & 511] : WZERO;
        WAVES[22][x] = (x & 511) < 256 ? WFULL : WZERO;
        WAVES[23][x] = WZERO;
        WAVES[25][x] = x < 512 ? saw[x] : WZERO;
        WAVES[26][x] = saw[x & 511];
        WAVES[27][x] = x < 128 ? saw[x] : x >= 512 && x < 768 ? saw[x - 512 << 1] : WZERO;
        WAVES[28][x] = x < 512 ? saw[x << 1] : WZERO;
        WAVES[29][x] = x < 512 ? saw[x << 1 & 511] : WZERO;
        WAVES[30][x] = x < 256 ? WFULL : WZERO;
        WAVES[31][x] = WZERO;
    }
    for (x = 0; x < 26; x++)
        AM_LFO_A[x] = AM_LFO_A[51 - x] = x;
    SUSTAINS[0] = 0;
    SUSTAINS[15] = 511;
    for (x = 1; x < 15; x++)
        SUSTAINS[x] = (int) jround(16 * pow(2, log(x) / log(2)));
    for (x = 0; x < 512; x++)
        WAVE_ENV[x] = (int) jround(32767 * pow(10, x * -96.0 / 511 / 20));

    for (x = 0; x < MA3_INST4_COUNT; x++)
        alg_from_rom(&g_inst4[x], ma3_inst4_data + ma3_inst4_off[x], 0);
    for (x = 0; x < MA3_DRUM4_COUNT; x++)
        alg_from_rom(&g_drum4[x], ma3_drum4_data + ma3_drum4_off[x], 1);
    for (x = 0; x < MA3_WDRUM_COUNT; x++) {
        Alg a;
        alg_from_wave(&a, ma3_wdrum_data + ma3_wdrum_off[x], 0);
        if (a.drumKey - 24 >= 0 && a.drumKey - 24 < 61) {
            g_wdrums[a.drumKey - 24] = a;
            g_wdrum_set[a.drumKey - 24] = 1;
        }
    }
    for (x = 0; x < MA3_WROM_COUNT; x++)
        g_waverom[x] = decode_aica(ma3_wrom_data, (int) ma3_wrom_off[x],
                                   (int) (ma3_wrom_off[x + 1] - ma3_wrom_off[x]),
                                   &g_waverom_len[x]);
    g_tables_ready = 1;
}

/* ------------------------------------------------------------------ */
/* MA-3 sampler                                                        */
/* ------------------------------------------------------------------ */

typedef struct Sampler Sampler;
typedef struct Note Note;

typedef struct {
    OpDef d;
    int amPhase, envLevel, envOut, envPhase, envRate, envRof, envStage;
    int fb0, fb1, kslOut;
    int32_t oscPhase;
    float wavSample;
} Op;

struct Note {
    const Alg *alg;
    int chan;
    int key;
    Op op[4];
    float ampLeft, ampRight;
    int block, fNumber;
    int envDone, playing;
    float freqBase, volBase, volLeftOut, volRightOut;
};

typedef struct {
    Note *notesOn[128];
    Note **notesOut;
    int nOut, capOut;
    float bendBase, bendOut, bendRange;
    int isDrum, prgBank, prgProgram;
    float volLeft, volLeftOut, volLevel, volPanning, volRight, volRightOut;
} Chan;

typedef struct {
    int key;     /* bank << 8 | program */
    Alg alg;
} AlgEntry;

struct Sampler {
    Chan ch[NUM_CHANNELS];
    float sampleRate;
    float smpNext[2], smpPrev[2];
    float smpWidth, volRate;
    Alg wavDrums[128];
    int wavDrumSet[128];
    int amPhase;
    float bendOut;
    AlgEntry *fm2ops, *fm4ops;
    int n2, n4, cap2, cap4;
    float smpPosition;
    int vibPhase;
    float volFade, volLevel, volOut;
    int32_t *wavRam;
    int wavRamLen;
};

static int chan_add(Chan *c, Note *n)
{
    if (c->nOut == c->capOut) {
        int cap = c->capOut ? c->capOut * 2 : 8;
        Note **p = (Note **) realloc(c->notesOut, (size_t) cap * sizeof(Note *));
        if (p == NULL) return -1;
        c->notesOut = p;
        c->capOut = cap;
    }
    c->notesOut[c->nOut++] = n;
    return 0;
}

static void chan_remove_at(Chan *c, int i)
{
    free(c->notesOut[i]);
    memmove(c->notesOut + i, c->notesOut + i + 1, (size_t) (c->nOut - i - 1) * sizeof(Note *));
    c->nOut--;
}

static void note_stop(Note *n)
{
    int x;
    n->envDone = 1;
    n->playing = 0;
    n->volBase = 0.0f;
    for (x = 0; x < n->alg->nops; x++) {
        n->op[x].envLevel = 511;
        n->op[x].envStage = ENV_DONE;
    }
}

static void note_off(Note *n)
{
    int x;
    if (n->alg->isForever) {
        note_stop(n);
        return;
    }
    if (n->alg->isWave) return;
    for (x = 0; x < n->alg->nops; x++) {
        Op *op = &n->op[x];
        if (op->envStage == ENV_DONE || op->d.xof) continue;
        op->envRate = op->d.rr;
        op->envStage = ENV_RELEASE;
    }
}

static void note_on_envelope_done(Note *n)
{
    int flags = n->alg->isWave ? 1 : ENV_FLAGS[n->alg->alg], x;
    n->envDone = 1;
    for (x = 0; x < n->alg->nops; x++, flags >>= 1) {
        if ((flags & 1) != 0)
            n->envDone = n->envDone && n->op[x].envStage == ENV_DONE;
    }
    if (n->envDone) n->playing = 0;
}

static void op_on_frequency(Op *op, const Note *n)
{
    op->envRof = (n->block << 1 | (n->fNumber >> (8 + NTS) & 1)) >> ((op->d.ksr ^ 1) << 1);
    op->kslOut = jmax(0, KSL_B[op->d.ksl] * ((n->block << 3) - KSL_F[n->fNumber >> 6]));
}

static void note_on_frequency(Note *n, double bend)
{
    double freq;
    int x;
    if (n->alg->isWave) return;
    freq = n->alg->isDrum ? n->freqBase : n->freqBase * bend;
    n->block = jmin(7, jmax(0, (int) ((int64_t) jround(log(freq / 440) * MAGIC_B) + 57) / 12));
    n->fNumber = jmin(1023, jmax(0, (int) jround(freq * (1 << (20 - n->block)) * MAGIC_F)));
    for (x = 0; x < n->alg->nops; x++)
        op_on_frequency(&n->op[x], n);
}

static void note_on_volume(Note *n, const Chan *c)
{
    n->volLeftOut = n->volBase * n->alg->volLeft * c->volLeftOut;
    n->volRightOut = n->volBase * n->alg->volRight * c->volRightOut;
}

static int op_sample(Op *op, Note *note, Sampler *s, int mod, int feedback)
{
    const Alg *alg = note->alg;
    int x, y;
    if (op->envStage == ENV_DONE) return 0;
    if (!alg->isWave) {
        int32_t xv;
        if (feedback && op->d.fb != 0)
            mod += op->fb0 + op->fb1 >> (9 - op->d.fb);
        op->fb1 = op->fb0;
        xv = (int32_t) ((uint32_t) WAVES[op->d.ws][((op->oscPhase >> 9) + mod) & 1023]
                        + (uint32_t) (op->envOut << 3));
        op->fb0 = (EXPT[xv & 0xFF] << 1 >> (xv >> 8 & 31)) ^ (xv >> 31);
    } else {
        const int32_t *samples;
        int slen;
        if (!alg->rm) {
            samples = s->wavRam;
            slen = s->wavRamLen;
        } else {
            samples = g_waverom[alg->waveId];
            slen = g_waverom_len[alg->waveId];
        }
        if (samples != NULL && op->wavSample < alg->ep) {
            x = (int) floor(op->wavSample);
            op->fb0 = x < slen ? samples[x] * WAVE_ENV[op->envOut] / 32767 : 0;
            op->wavSample += alg->wavAdvance;
            if (op->wavSample >= alg->ep) {
                if (alg->lp < alg->ep) {
                    op->wavSample = fmodf(op->wavSample - alg->lp, (float) (alg->ep - alg->lp)) + alg->lp;
                } else {
                    op->wavSample = (float) alg->ep;
                    note_stop(note);
                }
            }
        } else
            op->fb0 = 0;
    }
    x = op->envRate == 0 ? 0 : jmin(63, (op->envRate << 2) + op->envRof);
    op->envPhase += op->envRate == 0 ? 0 : (4 | (x & 3)) << (x >> 2);
    y = op->envPhase >> 15;
    op->envPhase &= 0x7FFF;
    switch (op->envStage) {
    case ENV_ATTACK:
        if (y == 0) break;
        op->envLevel += ~(op->envLevel * y >> 3);
        if (op->envLevel <= 0) {
            op->envLevel = 0;
            op->envRate = op->d.dr;
            op->envStage = ENV_DECAY;
        }
        break;
    case ENV_DECAY:
    case ENV_SUSTAIN:
    case ENV_RELEASE:
        op->envLevel += y;
        if (op->envStage == ENV_DECAY && op->envLevel >= SUSTAINS[op->d.sl]) {
            op->envLevel = SUSTAINS[op->d.sl];
            op->envRate = op->d.sr;
            op->envStage = ENV_SUSTAIN;
        }
        if (op->envLevel >= 511) {
            op->envLevel = 511;
            op->envStage = ENV_DONE;
            note_on_envelope_done(note);
        }
        break;
    case ENV_DONE:
        op->envLevel = 511;
        break;
    }
    op->envOut = op->envLevel + op->kslOut + (op->d.tl << 2);
    if (op->d.eam) {
        op->envOut += AM_LFO_A[op->amPhase >> 12] << op->d.dam >> 2;
        op->amPhase = (op->amPhase + AM_LFO_B[alg->lfo]) % 0x34000;
    }
    op->envOut = jmin(jmax(op->envOut, 0), 511);
    if (alg->isWave) return op->fb0;
    op->oscPhase = (int32_t) ((uint32_t) op->oscPhase
                              + (uint32_t) (((note->fNumber << note->block >> 1) * MULTIS[op->d.multi]) >> 1));
    return op->fb0;
}

static float note_sample_fm(Note *n, Sampler *s)
{
    Op *o = n->op;
    int out1, out2, out3, out4, ret = 0;
    switch (n->alg->alg) {
    case 0:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, out1, 0);
        ret = out2;
        break;
    case 1:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, 0, 0);
        ret = out1 + out2;
        break;
    case 2:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, 0, 0);
        out3 = op_sample(&o[2], n, s, 0, 1);
        out4 = op_sample(&o[3], n, s, 0, 0);
        ret = out1 + out2 + out3 + out4;
        break;
    case 3:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, 0, 0);
        out3 = op_sample(&o[2], n, s, out2, 0);
        out4 = op_sample(&o[3], n, s, out1 + out3, 0);
        ret = out4;
        break;
    case 4:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, out1, 0);
        out3 = op_sample(&o[2], n, s, out2, 0);
        out4 = op_sample(&o[3], n, s, out3, 0);
        ret = out4;
        break;
    case 5:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, out1, 0);
        out3 = op_sample(&o[2], n, s, 0, 1);
        out4 = op_sample(&o[3], n, s, out3, 0);
        ret = out2 + out4;
        break;
    case 6:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, 0, 0);
        out3 = op_sample(&o[2], n, s, out2, 0);
        out4 = op_sample(&o[3], n, s, out3, 0);
        ret = out1 + out4;
        break;
    case 7:
        out1 = op_sample(&o[0], n, s, 0, 1);
        out2 = op_sample(&o[1], n, s, 0, 0);
        out3 = op_sample(&o[2], n, s, out2, 0);
        out4 = op_sample(&o[3], n, s, 0, 0);
        ret = out1 + out3 + out4;
        break;
    }
    return ret / 8170.0f;
}

static float ease(const Sampler *s, float level, float target)
{
    if (level < target) {
        float v = level + s->volRate;
        return v < target ? v : target;
    }
    if (level > target) {
        float v = level - s->volRate;
        return v > target ? v : target;
    }
    return level;
}

/* returns 1 when the note can be dropped */
static int note_render(Note *n, Sampler *s)
{
    float tgtLeft = 0.0f, tgtRight = 0.0f, sample;
    if (!n->envDone) {
        tgtLeft = n->volLeftOut;
        tgtRight = n->volRightOut;
    }
    sample = !n->alg->isWave ? note_sample_fm(n, s) : op_sample(&n->op[0], n, s, 0, 0) / 32768.0f;
    s->smpNext[0] += sample * n->ampLeft;
    s->smpNext[1] += sample * n->ampRight;
    n->ampLeft = ease(s, n->ampLeft, tgtLeft);
    n->ampRight = ease(s, n->ampRight, tgtRight);
    return !n->playing && n->ampLeft == 0 && n->ampRight == 0;
}

static void sampler_chan_reset(Chan *c)
{
    int i;
    c->bendBase = 0.0f;
    c->bendOut = 1.0f;
    c->bendRange = 2.0f;
    c->isDrum = 0;
    c->prgBank = 0;
    c->prgProgram = 0;
    c->volLevel = 1.0f;
    c->volPanning = 0.5f;
    c->volLeft = 0.5f;
    c->volLeftOut = 0.5f;
    c->volRight = 0.5f;
    c->volRightOut = 0.5f;
    memset(c->notesOn, 0, sizeof(c->notesOn));
    for (i = 0; i < c->nOut; i++)
        note_stop(c->notesOut[i]);
}

static void sampler_reset(Sampler *s)
{
    int i;
    s->amPhase = 0;
    s->bendOut = 1.0f;
    s->smpPosition = 0.0f;
    s->smpPrev[0] = s->smpPrev[1] = 0.0f;
    s->vibPhase = 0;
    s->volFade = 0.0f;
    s->volLevel = 1.0f;
    s->volOut = 1.0f;
    free(s->wavRam);
    s->wavRam = NULL;
    s->wavRamLen = 0;
    s->n2 = s->n4 = 0;
    for (i = 0; i < NUM_CHANNELS; i++)
        sampler_chan_reset(&s->ch[i]);
    memset(s->wavDrumSet, 0, sizeof(s->wavDrumSet));
}

static void sampler_init(Sampler *s, float rate)
{
    memset(s, 0, sizeof(*s));
    s->sampleRate = rate;
    s->smpWidth = MA3_RATE / rate;
    s->volRate = 1 / (rate * 0.01f);
    sampler_reset(s);
}

static void sampler_free(Sampler *s)
{
    int i, j;
    for (i = 0; i < NUM_CHANNELS; i++) {
        for (j = 0; j < s->ch[i].nOut; j++)
            free(s->ch[i].notesOut[j]);
        free(s->ch[i].notesOut);
    }
    free(s->fm2ops);
    free(s->fm4ops);
    free(s->wavRam);
    memset(s, 0, sizeof(*s));
}

static void chan_on_volume(Sampler *s, Chan *c)
{
    int i;
    c->volLeftOut = s->volOut * c->volLeft;
    c->volRightOut = s->volOut * c->volRight;
    for (i = 0; i < c->nOut; i++)
        note_on_volume(c->notesOut[i], c);
}

static void chan_on_frequency(Chan *c)
{
    int i;
    for (i = 0; i < c->nOut; i++)
        note_on_frequency(c->notesOut[i], c->bendOut);
}

static void sampler_on_volume(Sampler *s)
{
    int i;
    s->volOut = (1.0f - s->volFade) * s->volLevel;
    for (i = 0; i < NUM_CHANNELS; i++)
        chan_on_volume(s, &s->ch[i]);
}

static const Alg *find_alg(const AlgEntry *e, int n, int key)
{
    int i;
    for (i = 0; i < n; i++)
        if (e[i].key == key) return &e[i].alg;
    return NULL;
}

static const Alg *get_fm_instrument(Sampler *s, int bank, int program)
{
    int key = bank << 8 | program;
    const Alg *ret = find_alg(s->fm4ops, s->n4, key);
    if (ret == NULL) ret = find_alg(s->fm2ops, s->n2, key);
    if (ret == NULL) ret = &g_inst4[bank < 2 ? 0 : ((bank & 1) << 6 | (program & 0x3F))];
    return ret;
}

static const Alg *get_drum_fm(int key)
{
    if (key < 0) key += 35;
    if (key < 0 || key >= MA3_DRUM4_COUNT) return NULL;
    return &g_drum4[key];
}

static const Alg *get_drum_wave(Sampler *s, int key)
{
    const Alg *ret = NULL;
    if (key < -24) return NULL;
    if (key < 0) {
        key += 24;
        if (key >= 0 && key < 128 && s->wavDrumSet[key]) ret = &s->wavDrums[key];
    } else if (key < 61 && g_wdrum_set[key]) {
        ret = &g_wdrums[key];
    }
    if (ret != NULL && !ret->rm && (s->wavRam == NULL || ret->ep >= s->wavRamLen))
        ret = NULL;
    return ret;
}

static void sampler_key_off(Sampler *s, int channel, int key)
{
    Chan *c;
    Note *n;
    if (channel < 0 || channel >= NUM_CHANNELS || A4_MA3 + key < 0 || A4_MA3 + key >= 128)
        return;
    c = &s->ch[channel];
    n = c->notesOn[A4_MA3 + key];
    if (n != NULL) note_off(n);
    c->notesOn[A4_MA3 + key] = NULL;
}

static void sampler_key_on(Sampler *s, int channel, int key, float velocity)
{
    const Alg *alg = NULL;
    Chan *c;
    float freqBase = 0;
    int isWave = 0, x;
    Note *n;
    if (channel < 0 || channel >= NUM_CHANNELS || A4_MA3 + key < 0 || A4_MA3 + key >= 128)
        return;
    c = &s->ch[channel];
    n = c->notesOn[A4_MA3 + key];
    if (!c->isDrum) {
        alg = get_fm_instrument(s, c->prgBank, c->prgProgram);
        freqBase = (float) (440 * pow(2, key / 12.0));
    } else {
        alg = get_drum_wave(s, key);
        isWave = alg != NULL;
        if (alg == NULL) alg = get_drum_fm(key);
        if (alg == NULL) return;
        freqBase = alg->freqBase;
        isWave = alg->isWave;
    }
    if (n != NULL && (c->isDrum || n->alg != alg)) {
        sampler_key_off(s, channel, key);
        n = NULL;
    }
    if (n == NULL) {
        n = (Note *) calloc(1, sizeof(Note));
        if (n == NULL) return;
        n->alg = alg;
        n->chan = channel;
        n->key = key;
        n->playing = 1;
        for (x = 0; x < alg->nops; x++) {
            Op *op = &n->op[x];
            op->d = alg->ops[x];
            op->amPhase = s->amPhase;
            op->envLevel = 511;
            op->envRate = op->d.ar;
            op->envStage = ENV_ATTACK;
        }
        if (chan_add(c, n) != 0) {
            free(n);
            return;
        }
        c->notesOn[A4_MA3 + key] = n;
    }
    n->volBase = velocity;
    note_on_volume(n, c);
    if (!isWave) {
        n->freqBase = freqBase;
        note_on_frequency(n, s->bendOut * c->bendOut);
    }
}

static void sampler_master_tune(Sampler *s, float semitones)
{
    int i;
    s->bendOut = (float) pow(2, semitones);
    for (i = 0; i < NUM_CHANNELS; i++)
        chan_on_frequency(&s->ch[i]);
}

static void sampler_master_volume(Sampler *s, float volume)
{
    s->volLevel = volume;
    sampler_on_volume(s);
}

static void sampler_panpot(Sampler *s, int channel, float panpot)
{
    Chan *c;
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    c = &s->ch[channel];
    c->volPanning = (panpot + 1) / 2;
    c->volLeft = (1.0f - c->volPanning) * c->volLevel;
    c->volRight = c->volPanning * c->volLevel;
    chan_on_volume(s, c);
}

static void sampler_pitch_bend(Sampler *s, int channel, float semitones)
{
    Chan *c;
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    c = &s->ch[channel];
    c->bendBase = semitones;
    c->bendOut = (float) pow(2, c->bendBase * c->bendRange);
    chan_on_frequency(c);
}

static void sampler_pitch_bend_range(Sampler *s, int channel, float range)
{
    Chan *c;
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    c = &s->ch[channel];
    c->bendRange = range;
    c->bendOut = (float) pow(2, c->bendBase * c->bendRange);
    chan_on_frequency(c);
}

static void sampler_volume(Sampler *s, int channel, float volume)
{
    Chan *c;
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    c = &s->ch[channel];
    c->volLevel = volume;
    c->volLeft = (1.0f - c->volPanning) * c->volLevel;
    c->volRight = c->volPanning * c->volLevel;
    chan_on_volume(s, c);
}

static void stop_wave_drums(Sampler *s)
{
    int i, j;
    for (i = 0; i < NUM_CHANNELS; i++)
        for (j = 0; j < s->ch[i].nOut; j++)
            if (s->ch[i].notesOut[j]->alg->isWave)
                note_stop(s->ch[i].notesOut[j]);
}

static void set_fm_algorithms(Sampler *s, const uint8_t *m, int len)
{
    int off;
    for (off = 4; off < len;) {
        int type = m[off], size;
        Alg a;
        AlgEntry **tab;
        int *n, *cap, key, i;
        if (type != 1 && type != 2) break;
        size = type == 1 ? 20 : 34;
        if (off + size > len) break;
        if (alg_from_sysex(&a, m, len, off) != 0) break;
        key = m[off + 1] << 8 | m[off + 2];
        tab = type == 1 ? &s->fm2ops : &s->fm4ops;
        n = type == 1 ? &s->n2 : &s->n4;
        cap = type == 1 ? &s->cap2 : &s->cap4;
        for (i = 0; i < *n; i++)
            if ((*tab)[i].key == key) break;
        if (i == *n) {
            if (*n == *cap) {
                int nc = *cap ? *cap * 2 : 8;
                AlgEntry *p = (AlgEntry *) realloc(*tab, (size_t) nc * sizeof(AlgEntry));
                if (p == NULL) break;
                *tab = p;
                *cap = nc;
            }
            (*n)++;
        }
        (*tab)[i].key = key;
        (*tab)[i].alg = a;
        off += size;
    }
}

static void set_wave_drums(Sampler *s, const uint8_t *m, int len)
{
    int count = (len - 4) / 18, x, src;
    memset(s->wavDrumSet, 0, sizeof(s->wavDrumSet));
    for (x = 0, src = 4; x < count; x++, src += 18) {
        Alg d;
        alg_from_wave(&d, m, src + 1);
        if ((d.drumKey >= 24 && d.drumKey <= 91) || d.ep < d.lp ||
            (d.rm && (d.waveId == 7 || d.ep > g_waverom_len[d.waveId])))
            continue;
        if (d.drumKey < 128) {
            s->wavDrums[d.drumKey] = d;
            s->wavDrumSet[d.drumKey] = 1;
        }
    }
}

static void sampler_sysex(Sampler *s, const uint8_t *m, int len)
{
    if (m == NULL || len < 4 || m[0] != 0x11 || m[1] != 0x01 || (m[2] & 0xF0) != 0xF0)
        return;
    switch (m[3]) {
    case 0x03:
        if (len >= 5) {
            s->volFade = (m[4] & 0x7F) / 127.0f;
            sampler_on_volume(s);
        }
        break;
    case 0x04:
        set_fm_algorithms(s, m, len);
        break;
    case 0x05:
        set_wave_drums(s, m, len);
        stop_wave_drums(s);
        break;
    case 0x06:
        free(s->wavRam);
        s->wavRam = decode_aica(m, 4, len - 4, &s->wavRamLen);
        stop_wave_drums(s);
        break;
    default:
        break;
    }
}

static void sampler_sample(Sampler *s)
{
    int i, x;
    s->smpNext[0] = s->smpNext[1] = 0.0f;
    for (i = 0; i < NUM_CHANNELS; i++) {
        Chan *c = &s->ch[i];
        for (x = 0; x < c->nOut; x++) {
            if (note_render(c->notesOut[x], s)) {
                /* a note still listed in notesOn is dropped with it */
                Note *n = c->notesOut[x];
                int k;
                for (k = 0; k < 128; k++)
                    if (c->notesOn[k] == n) c->notesOn[k] = NULL;
                chan_remove_at(c, x--);
            }
        }
    }
    s->amPhase = (s->amPhase + 1) % 0x34000;
    s->vibPhase++;
}

static void sampler_render(Sampler *s, float *out, int frames)
{
    float *smpPrev = s->smpPrev, *smpNext = s->smpNext;
    float smpWidth = s->smpWidth, smpPosition = s->smpPosition;
    float frame[2];
    int x, y;
    for (x = 0; x < frames; x++) {
        float l = smpPosition, r = l + smpWidth, a, b;
        if (l == 0.0f) sampler_sample(s);
        if (l < 1.0f) {
            a = (l + r) / 2;
            frame[0] = smpPrev[0] + (smpNext[0] - smpPrev[0]) * a;
            frame[1] = smpPrev[1] + (smpNext[1] - smpPrev[1]) * a;
        } else {
            a = (l + 1.0f) / 2;
            b = 1.0f - l;
            frame[0] = (smpPrev[0] + (smpNext[0] - smpPrev[0]) * a) * b;
            frame[1] = (smpPrev[1] + (smpNext[1] - smpPrev[1]) * a) * b;
            for (y = (int) floor(r) - 1; y > 0; y--) {
                smpPrev[0] = smpNext[0];
                smpPrev[1] = smpNext[1];
                sampler_sample(s);
                frame[0] += (smpPrev[0] + smpNext[0]) / 2;
                frame[1] += (smpPrev[1] + smpNext[1]) / 2;
            }
            smpPrev[0] = smpNext[0];
            smpPrev[1] = smpNext[1];
            r = fmodf(r, 1.0f);
            if (r != 0.0f) {
                sampler_sample(s);
                a = r / 2;
                frame[0] += (smpPrev[0] + (smpNext[0] - smpPrev[0]) * a) * r;
                frame[1] += (smpPrev[1] + (smpNext[1] - smpPrev[1]) * a) * r;
            }
            frame[0] /= smpWidth;
            frame[1] /= smpWidth;
        }
        frame[0] = frame[0] < -1.0f ? -1.0f : frame[0] > 1.0f ? 1.0f : frame[0];
        frame[1] = frame[1] < -1.0f ? -1.0f : frame[1] > 1.0f ? 1.0f : frame[1];
        if (out != NULL) {
            out[2 * x] = frame[0];
            out[2 * x + 1] = frame[1];
        }
        smpPosition = r;
    }
    s->smpPosition = smpPosition;
}

/* ------------------------------------------------------------------ */
/* MLD parser                                                          */
/* ------------------------------------------------------------------ */

#define EVENT_TYPE_UNKNOWN (-1)
#define EVENT_TYPE_NOTE 0
#define EVENT_TYPE_EXT_B 1
#define EVENT_TYPE_EXT_INFO 2

#define EV_BANK_CHANGE 0xE1
#define EV_CUEPOINT 0xD0
#define EV_END_OF_TRACK 0xDF
#define EV_EXPRESSION 0xE6
#define EV_JUMP 0xD1
#define EV_MASTER_BALANCE 0xB1
#define EV_MASTER_TUNE 0xB3
#define EV_MASTER_VOLUME 0xB0
#define EV_PANPOT 0xE3
#define EV_PITCHBEND 0xE4
#define EV_PITCHBEND_RANGE 0xE7
#define EV_PROGRAM_CHANGE 0xE0
#define EV_TIMEBASE_TEMPO 0xC0
#define EV_VOLUME 0xE2
#define EV_X_DRUM_ENABLE 0xBA

typedef struct {
    int delta, status, type, id, param;
    int channel, channelIndex;
    int bank, cuepoint, gateTime, key, keyNumber, octaveShift;
    int program, tempo, timebase, enable;
    float panpot, range, semitones, velocity, volume;
    int data_off, data_len;
} MEvent;

typedef struct {
    MEvent *ev;
    int n, cap;
    int cue;
} MTrack;

typedef struct {
    uint8_t *data;
    int len;
    int note;
    int ntracks;
    int cuep[4];
    MTrack tracks[4];
    float masterPan, masterTune, masterVolume;
} Mld;

typedef struct {
    const uint8_t *d;
    int start, len, off;
    int err;
} Rd;

static int rd_eof(Rd *r) { return r->off == r->start + r->len; }

static int rd_u8(Rd *r)
{
    if (r->off >= r->start + r->len) {
        r->err = 1;
        return 0;
    }
    return r->d[r->off++];
}

static int rd_u16(Rd *r)
{
    int v = rd_u8(r) << 8;
    return v | rd_u8(r);
}

static int rd_u32(Rd *r)
{
    int v = rd_u16(r) << 16;
    if (v < 0) r->err = 1;
    return v | rd_u16(r);
}

static Rd rd_sub(Rd *r, int len)
{
    Rd s;
    s.d = r->d;
    s.start = r->off;
    s.len = len;
    s.off = r->off;
    s.err = 0;
    if (len < 0 || r->off + len > r->start + r->len) {
        r->err = 1;
        s.len = 0;
        return s;
    }
    r->off += len;
    return s;
}

static float volume_to_amplitude(float param)
{
    return param == 0.0f ? 0.0f : (float) pow(2, (1 - param) * -96 / 20);
}

static int track_add(MTrack *t, const MEvent *e)
{
    if (t->n == t->cap) {
        int cap = t->cap ? t->cap * 2 : 64;
        MEvent *p = (MEvent *) realloc(t->ev, (size_t) cap * sizeof(MEvent));
        if (p == NULL) return -1;
        t->ev = p;
        t->cap = cap;
    }
    t->ev[t->n++] = *e;
    return 0;
}

static void parse_event(Mld *m, int track, Rd *r, MEvent *e)
{
    memset(e, 0, sizeof(*e));
    e->delta = rd_u8(r);
    e->status = rd_u8(r);
    if ((e->status & 0x3F) != 63) {
        /* note */
        e->type = EVENT_TYPE_NOTE;
        e->channelIndex = e->status >> 6;
        e->gateTime = rd_u8(r);
        e->keyNumber = e->status & 63;
        if (m->note == 0) {
            e->octaveShift = 0;
            e->velocity = 1.0f;
        } else {
            int bits = rd_u8(r);
            e->octaveShift = (int32_t) ((uint32_t) bits << 30) >> 30;
            e->velocity = (bits >> 2) / 63.0f;
        }
        e->channel = track << 2 | e->channelIndex;
        e->key = e->octaveShift * 12 + e->keyNumber - 24;
        return;
    }
    e->id = rd_u8(r);
    if (e->id >= 0xF0) {
        int n;
        e->type = EVENT_TYPE_EXT_INFO;
        n = rd_u16(r);
        e->data_off = r->off;
        e->data_len = n;
        if (r->off + n > r->start + r->len) {
            r->err = 1;
            r->off = r->start + r->len;
        } else
            r->off += n;
        return;
    }
    if (e->id < 0x80) {
        e->type = EVENT_TYPE_UNKNOWN;
        rd_u8(r);
        rd_u8(r);
        return;
    }
    e->type = EVENT_TYPE_EXT_B;
    e->param = rd_u8(r);
    e->channelIndex = e->param >> 6;
    e->channel = track << 2 | e->channelIndex;
    if ((e->id & 0xF0) == EV_TIMEBASE_TEMPO) {
        e->bank = e->id;
        e->tempo = e->param;
        e->timebase = (e->id & 7) == 7 ? -1 : ((e->id & 15) > 7 ? 15 : 6) << (e->id & 7);
        e->id = EV_TIMEBASE_TEMPO;
        return;
    }
    switch (e->id) {
    case EV_BANK_CHANGE:
        e->bank = e->param & 0x3F;
        break;
    case EV_CUEPOINT:
        e->cuepoint = e->param;
        break;
    case EV_EXPRESSION:
        e->volume *= volume_to_amplitude((e->param & 0x3F) / 63.0f);
        break;
    case EV_MASTER_BALANCE:
        m->masterPan = e->param < 64 ? e->param / 64.0f - 1 : (e->param - 64) / 63.0f;
        e->panpot = m->masterPan;
        break;
    case EV_MASTER_TUNE:
        m->masterTune = ((e->param & 0x7F) - 64) / 64.0f;
        e->semitones = m->masterTune;
        break;
    case EV_MASTER_VOLUME:
        m->masterVolume = volume_to_amplitude(e->param / 100.0f);
        e->volume = m->masterVolume;
        break;
    case EV_PANPOT: {
        int p = e->param & 0x3F;
        e->panpot = (p < 32 ? p / 32.0f - 1 : (p - 32) / 31.0f) * m->masterPan;
        break;
    }
    case EV_PITCHBEND:
        e->semitones = ((e->param & 0x3F) - 32) / 3200.0f * m->masterTune;
        break;
    case EV_PITCHBEND_RANGE:
        e->range = (float) (e->param & 0x3F);
        break;
    case EV_PROGRAM_CHANGE:
        e->program = e->param & 0x3F;
        break;
    case EV_VOLUME:
        e->volume = volume_to_amplitude((e->param & 0x3F) / 63.0f) * m->masterVolume;
        break;
    case EV_X_DRUM_ENABLE:
        e->channel = e->param >> 3 & 15;
        e->enable = (e->param & 1) != 0;
        break;
    default:
        break;
    }
}

static void mld_free(Mld *m)
{
    int i;
    for (i = 0; i < 4; i++)
        free(m->tracks[i].ev);
    free(m->data);
    memset(m, 0, sizeof(*m));
}

static int mld_parse(Mld *m, const uint8_t *src, int srclen)
{
    Rd r, h;
    int length, i, numTracks, nAdpcm = 0, contentType;
    memset(m, 0, sizeof(*m));
    m->masterPan = 0.0f;
    m->masterTune = 1.0f;
    m->masterVolume = 1.0f;
    if (srclen < 8 || memcmp(src, "melo", 4) != 0) return -1;
    length = (int) ((uint32_t) src[4] << 24 | src[5] << 16 | src[6] << 8 | src[7]);
    if (length < 0 || 8 + length > srclen) return -1;
    m->data = (uint8_t *) malloc((size_t) length + 8);
    if (m->data == NULL) return -1;
    memcpy(m->data, src, (size_t) length + 8);
    m->len = length + 8;
    r.d = m->data;
    r.start = 8;
    r.len = length;
    r.off = 8;
    r.err = 0;
    /* header */
    h = rd_sub(&r, rd_u16(&r));
    contentType = rd_u16(&h);
    if (contentType != 0x0101) goto fail;
    numTracks = rd_u8(&h);
    if (numTracks > 4) goto fail;
    m->ntracks = numTracks;
    while (!rd_eof(&h) && !h.err) {
        int id = rd_u32(&h);
        Rd c = rd_sub(&h, rd_u16(&h));
        if (h.err) break;
        if (id == 0x61696E66) { /* ainf */
            if (c.len > 0) nAdpcm = c.d[c.off];
        } else if (id == 0x63756570) { /* cuep */
            for (i = 0; i < numTracks; i++)
                m->cuep[i] = rd_u32(&c);
        } else if (id == 0x6E6F7465) { /* note */
            m->note = rd_u16(&c);
            if (m->note >> 1 != 0) goto fail;
        }
    }
    if (h.err || r.err) goto fail;
    /* ADPCM audio chunks are skipped (not played by the MA-3 FM path) */
    for (i = 0; i < nAdpcm; i++) {
        int sz;
        if (rd_u32(&r) != 0x61646174) goto fail;
        sz = rd_u32(&r);
        rd_sub(&r, sz);
        if (r.err) goto fail;
    }
    for (i = 0; i < numTracks; i++) {
        Rd t;
        int cue;
        MTrack *tr = &m->tracks[i];
        if (rd_u32(&r) != 0x74726163) goto fail; /* trac */
        t = rd_sub(&r, rd_u32(&r));
        if (r.err) goto fail;
        cue = t.off + m->cuep[i];
        while (!rd_eof(&t)) {
            MEvent e;
            parse_event(m, i, &t, &e);
            if (t.err) goto fail;
            if (t.off == cue || (e.id == EV_CUEPOINT && e.type == EVENT_TYPE_EXT_B && e.cuepoint == 0))
                tr->cue = tr->n;
            if (track_add(tr, &e) != 0) goto fail;
        }
    }
    return 0;
fail:
    mld_free(m);
    return -1;
}

/* ------------------------------------------------------------------ */
/* MLD player                                                          */
/* ------------------------------------------------------------------ */

#define A4_MLD 48

typedef struct {
    int channel, gateTime, key;
} PNote;

typedef struct {
    PNote *on[100];
    PNote **out;
    int nOut, capOut;
} PChan;

typedef struct {
    int cuepoint, finished, offset, ticks;
} PTrack;

struct FkaMld {
    Mld mld;
    Sampler s;
    PChan ch[16];
    PTrack tr[4];
    float sampleRate;
    int finished;
    float framesPerTick;
    float pendingFrames;
    int pendingTicks;
    long long position;
    int seeking;
    long long tickNow;
};

static void set_tempo(FkaMld *p, int timebase, int tempo)
{
    p->framesPerTick = (60 * p->sampleRate) / (float) (timebase * tempo);
}

static void set_track_offset(FkaMld *p, PTrack *t, int off, int index)
{
    t->offset = off;
    t->finished = off >= p->mld.tracks[index].n;
}

static void pchan_remove(PChan *c, PNote *n)
{
    int i;
    for (i = 0; i < c->nOut; i++) {
        if (c->out[i] == n) {
            memmove(c->out + i, c->out + i + 1, (size_t) (c->nOut - i - 1) * sizeof(PNote *));
            c->nOut--;
            free(n);
            return;
        }
    }
}

static void evt_note(FkaMld *p, PTrack *t, int index, const MEvent *e)
{
    PChan *c = &p->ch[e->channel & 15];
    int k = A4_MLD + e->key;
    PNote *n = (k >= 0 && k < 100) ? c->on[k] : NULL;
    set_track_offset(p, t, t->offset + 1, index);
    if (k < 0 || k >= 100) return;
    if (e->velocity == 0) {
        sampler_key_off(&p->s, e->channel, e->key);
        if (n != NULL) {
            c->on[k] = NULL;
            pchan_remove(c, n);
        }
        return;
    }
    if (!p->seeking)
        sampler_key_on(&p->s, e->channel, e->key, e->velocity);
    if (n == NULL) {
        n = (PNote *) calloc(1, sizeof(PNote));
        if (n == NULL) return;
        n->channel = e->channel;
        n->key = e->key;
        if (c->nOut == c->capOut) {
            int cap = c->capOut ? c->capOut * 2 : 16;
            PNote **q = (PNote **) realloc(c->out, (size_t) cap * sizeof(PNote *));
            if (q == NULL) {
                free(n);
                return;
            }
            c->out = q;
            c->capOut = cap;
        }
        c->on[k] = n;
        c->out[c->nOut++] = n;
    }
    n->gateTime = e->gateTime;
}

static void evt_ext_b(FkaMld *p, PTrack *t, int index, const MEvent *e)
{
    int i;
    switch (e->id) {
    case EV_BANK_CHANGE:
        if (e->channel >= 0 && e->channel < NUM_CHANNELS && e->bank <= 255)
            p->s.ch[e->channel].prgBank = e->bank;
        break;
    case EV_CUEPOINT:
        set_track_offset(p, t, t->offset + 1, index);
        if (e->cuepoint == 0) {
            for (i = 0; i < p->mld.ntracks; i++)
                p->tr[i].cuepoint = p->tr[i].offset;
            return;
        }
        if (e->cuepoint == 1 && p->tr[0].cuepoint != -1)
            p->finished = 1;
        return;
    case EV_END_OF_TRACK:
        t->finished = 1;
        return;
    case EV_MASTER_VOLUME:
        sampler_master_volume(&p->s, e->volume);
        break;
    case EV_MASTER_TUNE:
        sampler_master_tune(&p->s, e->semitones);
        break;
    case EV_PANPOT:
        sampler_panpot(&p->s, e->channel, e->panpot);
        break;
    case EV_PITCHBEND:
        sampler_pitch_bend(&p->s, e->channel, e->semitones);
        break;
    case EV_PITCHBEND_RANGE:
        sampler_pitch_bend_range(&p->s, e->channel, e->range);
        break;
    case EV_PROGRAM_CHANGE:
        if (e->channel >= 0 && e->channel < NUM_CHANNELS && e->program <= 255)
            p->s.ch[e->channel].prgProgram = e->program;
        break;
    case EV_TIMEBASE_TEMPO:
        if (e->timebase != -1) {
            float prev = p->framesPerTick;
            set_tempo(p, e->timebase, e->tempo);
            p->pendingFrames = p->pendingFrames * p->framesPerTick / prev;
        }
        /* the Java player keeps the offset on timebase -1 and never
         * advances; skip the event instead of hanging */
        break;
    case EV_VOLUME:
        sampler_volume(&p->s, e->channel, e->volume);
        break;
    case EV_X_DRUM_ENABLE:
        if (e->channel >= 0 && e->channel < NUM_CHANNELS)
            p->s.ch[e->channel].isDrum = e->enable;
        break;
    default:
        break;
    }
    set_track_offset(p, t, t->offset + 1, index);
}

static void process(FkaMld *p, int index, int ticks)
{
    PTrack *t = &p->tr[index];
    MTrack *mt = &p->mld.tracks[index];
    if (t->finished) return;
    t->ticks -= ticks;
    if (t->ticks > 0) return;
    while (t->ticks == 0) {
        const MEvent *e = &mt->ev[t->offset];
        switch (e->type) {
        case EVENT_TYPE_NOTE:
            evt_note(p, t, index, e);
            break;
        case EVENT_TYPE_EXT_B:
            evt_ext_b(p, t, index, e);
            break;
        case EVENT_TYPE_EXT_INFO:
            sampler_sysex(&p->s, p->mld.data + e->data_off, e->data_len);
            set_track_offset(p, t, t->offset + 1, index);
            break;
        default:
            set_track_offset(p, t, t->offset + 1, index);
        }
        if (t->finished) return;
        t->ticks = mt->ev[t->offset].delta;
    }
}

static int until_note(FkaMld *p)
{
    int ret = -1, i, j;
    for (i = 0; i < 16; i++)
        for (j = 0; j < p->ch[i].nOut; j++) {
            int g = p->ch[i].out[j]->gateTime;
            if (ret == -1 || g < ret) ret = g;
        }
    return ret;
}

static int until_track(FkaMld *p)
{
    int ret = -1, i;
    for (i = 0; i < p->mld.ntracks; i++) {
        if (p->tr[i].finished) continue;
        if (ret == -1 || p->tr[i].ticks < ret) ret = p->tr[i].ticks;
    }
    return ret;
}

void fka_mld_reset(FkaMld *p)
{
    int i, j;
    p->pendingFrames = 0;
    p->pendingTicks = 0;
    p->position = 0;
    p->tickNow = 0;
    set_tempo(p, 48, 125);
    sampler_reset(&p->s);
    /* the Java reset keeps the notes the sampler still holds: they are
     * stopped (note_stop) and fade out on the next samples */
    for (i = 0; i < 16; i++) {
        PChan *c = &p->ch[i];
        for (j = 0; j < c->nOut; j++)
            free(c->out[j]);
        c->nOut = 0;
        memset(c->on, 0, sizeof(c->on));
    }
    for (i = 0; i < p->mld.ntracks; i++) {
        PTrack *t = &p->tr[i];
        t->cuepoint = -1;
        t->offset = p->mld.tracks[i].cue;
        t->ticks = 0;
        t->finished = t->offset >= p->mld.tracks[i].n;
    }
    p->finished = 1;
    for (i = 0; i < p->mld.ntracks; i++) {
        process(p, i, 0);
        p->finished = p->finished && p->tr[i].finished;
    }
}

FkaMld *fka_mld_open(const uint8_t *data, int len, float sample_rate)
{
    FkaMld *p;
    tables_init();
    p = (FkaMld *) calloc(1, sizeof(FkaMld));
    if (p == NULL) return NULL;
    if (mld_parse(&p->mld, data, len) != 0) {
        free(p);
        return NULL;
    }
    p->sampleRate = sample_rate;
    sampler_init(&p->s, sample_rate);
    fka_mld_reset(p);
    return p;
}

void fka_mld_close(FkaMld *p)
{
    int i, j;
    if (p == NULL) return;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < p->ch[i].nOut; j++)
            free(p->ch[i].out[j]);
        free(p->ch[i].out);
    }
    sampler_free(&p->s);
    mld_free(&p->mld);
    free(p);
}

/*
 * Renders up to frames stereo frames (interleaved floats, may be NULL to
 * skip). Returns the frames produced, -1 once playback has finished (the
 * buffer is still filled, with the release tails), like MLDPlayer.render.
 */
int fka_mld_render(FkaMld *p, float *out, int frames)
{
    int ret = 0, i, j;
    if (p->finished) p->pendingFrames = (float) frames;
    while (frames > 0) {
        while (p->pendingFrames > 0) {
            int f = (int) floor(p->pendingFrames);
            if (f > frames) f = frames;
            if (!p->seeking) sampler_render(&p->s, out, f);
            frames -= f;
            if (out != NULL) out += f * 2;
            p->pendingFrames -= f;
            p->position += f;
            ret += f;
            if (frames == 0) return p->finished ? -1 : ret;
        }
        if (p->pendingTicks > 0) {
            p->tickNow += p->pendingTicks;
            for (i = 0; i < 16; i++)
                for (j = 0; j < p->ch[i].nOut; j++)
                    p->ch[i].out[j]->gateTime -= p->pendingTicks;
            for (i = 0; i < p->mld.ntracks; i++)
                process(p, i, p->pendingTicks);
            for (i = 0; i < 16; i++) {
                PChan *c = &p->ch[i];
                for (j = 0; j < c->nOut; j++) {
                    PNote *n = c->out[j];
                    if (n->gateTime != 0) continue;
                    sampler_key_off(&p->s, n->channel, n->key);
                    if (A4_MLD + n->key >= 0 && A4_MLD + n->key < 100)
                        c->on[A4_MLD + n->key] = NULL;
                    memmove(c->out + j, c->out + j + 1, (size_t) (c->nOut - j - 1) * sizeof(PNote *));
                    c->nOut--;
                    free(n);
                    j--;
                }
            }
        }
        {
            int ut = until_track(p), un;
            if (ut == -1) {
                p->finished = 1;
                return ret;
            }
            un = until_note(p);
            p->pendingTicks = un == -1 ? ut : (un < ut ? un : ut);
            p->pendingFrames += (float) floor(p->pendingTicks * p->framesPerTick);
        }
    }
    return ret;
}

int fka_mld_is_silent(FkaMld *p)
{
    int i;
    for (i = 0; i < NUM_CHANNELS; i++)
        if (p->s.ch[i].nOut != 0) return 0;
    return 1;
}

int fka_mld_finished(FkaMld *p)
{
    return p->finished;
}

/* Frames until the sequence ends (no synthesis), for durations. */
long long fka_mld_count_frames(const uint8_t *data, int len, float sample_rate)
{
    FkaMld *p = fka_mld_open(data, len, sample_rate);
    long long total = 0;
    int guard = 0;
    if (p == NULL) return -1;
    p->seeking = 1;
    while (guard++ < 1000000) {
        int n = fka_mld_render(p, NULL, 4096);
        if (n < 0) break;
        total += n;
        if (n < 4096) break;
    }
    fka_mld_close(p);
    return total;
}
