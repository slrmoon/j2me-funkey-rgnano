/*
 * RGB565 image scaler for the FunKey presenter (included by
 * lfjport_st_export.c; header-only so it can be tested on a host).
 *
 * Separable resampling with per-axis tap tables built once per layout:
 *   SCALE_FILTER_SHARP   upscale: nearest inside pixels, one blended output
 *                        pixel at each source pixel edge ("sharp bilinear":
 *                        even pixel widths at non-integer factors, no blur)
 *   SCALE_FILTER_SMOOTH  upscale: bilinear
 *   SCALE_FILTER_PIXEL   nearest neighbour everywhere
 * Downscaling (factor < 1) uses an area (box) filter in the first two modes,
 * so text in 240x320 -> 180x240 keeps every line instead of dropping some.
 *
 * Arithmetic: a pixel is "spread" into 0x07E0F81F form (green in the high
 * half), so one 32-bit multiply-add handles all three channels; weights are
 * 5-bit and sum to 32, which leaves exactly enough headroom per field.
 */
#ifndef LFJPORT_SCALE_H
#define LFJPORT_SCALE_H

#include <stdint.h>
#include <string.h>

#define SCALE_FILTER_SHARP  0
#define SCALE_FILTER_SMOOTH 1
#define SCALE_FILTER_PIXEL  2

#define SCALE_MAX_DIM 1024
#define SCALE_MAX_TAPS 3
#define SCALE_ROWS 4

typedef struct {
    uint16_t idx;                 /* first source index */
    uint8_t n;                    /* taps used */
    uint8_t w[SCALE_MAX_TAPS];    /* weights, sum 32 */
} ScaleTap;

typedef struct {
    /* layout the tables were built for */
    int srcX, srcY, srcW, srcH, dstW, dstH, filter, valid;
    int nearest;                  /* every tap is a single pixel */
    ScaleTap xt[SCALE_MAX_DIM];
    ScaleTap yt[SCALE_MAX_DIM];
    uint32_t rows[SCALE_ROWS][SCALE_MAX_DIM];
    int rowY[SCALE_ROWS];
} Scaler;

static uint32_t scale_spread(uint16_t p) {
    return ((uint32_t) p | ((uint32_t) p << 16)) & 0x07E0F81Fu;
}

static uint16_t scale_unspread(uint32_t v) {
    return (uint16_t) ((v & 0xF81Fu) | ((v >> 16) & 0x07E0u));
}

static void scale_set_weights(ScaleTap *t, int first, const double *f, int n,
                              int lo, int hi) {
    int i, k = 0, sum = 0, w[SCALE_MAX_TAPS + 2], idx[SCALE_MAX_TAPS + 2];
    /* merge taps that clamp to the same source pixel, drop empty ones */
    for (i = 0; i < n; i++) {
        int s = first + i, q = (int) (f[i] * 32.0 + 0.5);
        if (s < lo) s = lo;
        if (s > hi) s = hi;
        if (q <= 0) continue;
        if (k > 0 && idx[k - 1] == s) {
            w[k - 1] += q;
        } else {
            idx[k] = s;
            w[k] = q;
            k++;
        }
    }
    if (k == 0) {
        int s = first < lo ? lo : (first > hi ? hi : first);
        idx[0] = s;
        w[0] = 32;
        k = 1;
    }
    if (k > SCALE_MAX_TAPS) {
        /* keep the heaviest consecutive taps */
        int best = 0, bestSum = -1, j;
        for (j = 0; j + SCALE_MAX_TAPS <= k; j++) {
            int s = w[j] + w[j + 1] + w[j + 2];
            if (s > bestSum) {
                bestSum = s;
                best = j;
            }
        }
        for (j = 0; j < SCALE_MAX_TAPS; j++) {
            idx[j] = idx[best + j];
            w[j] = w[best + j];
        }
        k = SCALE_MAX_TAPS;
    }
    for (i = 0; i < k; i++) sum += w[i];
    /* renormalise to exactly 32, fixing up the heaviest tap */
    {
        int heavy = 0;
        for (i = 1; i < k; i++) if (w[i] > w[heavy]) heavy = i;
        w[heavy] += 32 - sum;
    }
    /* idx must be consecutive: fill gaps (can only come from clamping) */
    t->idx = (uint16_t) idx[0];
    t->n = 1;
    t->w[0] = (uint8_t) w[0];
    t->w[1] = t->w[2] = 0;
    for (i = 1; i < k; i++) {
        int off = idx[i] - idx[0];
        if (off <= 0 || off >= SCALE_MAX_TAPS) {
            t->w[0] = (uint8_t) (t->w[0] + w[i]);
            continue;
        }
        t->w[off] = (uint8_t) (t->w[off] + w[i]);
        if (off + 1 > t->n) t->n = (uint8_t) (off + 1);
    }
}

static void scale_build_axis(ScaleTap *t, int dstN, int srcStart, int srcN, int filter) {
    double s = (double) dstN / (double) srcN;
    int i, lo = srcStart, hi = srcStart + srcN - 1;
    for (i = 0; i < dstN; i++) {
        double f[SCALE_MAX_TAPS + 2];
        if (filter == SCALE_FILTER_PIXEL || (s < 1.0 && s < 0.5)) {
            int x = srcStart + (int) (((double) i + 0.5) / s);
            f[0] = 1.0;
            scale_set_weights(&t[i], x, f, 1, lo, hi);
        } else if (s >= 1.0) {
            double c = ((double) i + 0.5) / s - 0.5;
            int x0 = (int) (c >= 0 ? c : c - 1.0);
            double fr = c - x0;
            if (filter == SCALE_FILTER_SHARP) {
                fr = (fr - 0.5) * s + 0.5;
                if (fr < 0) fr = 0;
                if (fr > 1) fr = 1;
            }
            f[0] = 1.0 - fr;
            f[1] = fr;
            scale_set_weights(&t[i], srcStart + x0, f, 2, lo, hi);
        } else {
            /* area: output pixel covers [a, b) in source pixels */
            double a = (double) i / s, b = (double) (i + 1) / s;
            int x0 = (int) a, x, n = 0;
            for (x = x0; x < b && n < SCALE_MAX_TAPS + 2; x++, n++) {
                double l = a > x ? a : x, r = b < x + 1 ? b : x + 1;
                f[n] = (r - l) * s;
            }
            scale_set_weights(&t[i], srcStart + x0, f, n, lo, hi);
        }
    }
}

/* Rebuilds the tables when the layout changed; returns 0 if unsupported. */
static int scale_prepare(Scaler *sc, int srcX, int srcY, int srcW, int srcH,
                         int dstW, int dstH, int filter) {
    int i;
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0 ||
        dstW > SCALE_MAX_DIM || dstH > SCALE_MAX_DIM ||
        srcX + srcW > 65535 || srcY + srcH > 65535) {
        sc->valid = 0;
        return 0;
    }
    if (sc->valid && sc->srcX == srcX && sc->srcY == srcY && sc->srcW == srcW &&
        sc->srcH == srcH && sc->dstW == dstW && sc->dstH == dstH && sc->filter == filter) {
        return 1;
    }
    scale_build_axis(sc->xt, dstW, srcX, srcW, filter);
    scale_build_axis(sc->yt, dstH, srcY, srcH, filter);
    sc->nearest = 1;
    for (i = 0; i < dstW && sc->nearest; i++) if (sc->xt[i].n != 1) sc->nearest = 0;
    for (i = 0; i < dstH && sc->nearest; i++) if (sc->yt[i].n != 1) sc->nearest = 0;
    sc->srcX = srcX;
    sc->srcY = srcY;
    sc->srcW = srcW;
    sc->srcH = srcH;
    sc->dstW = dstW;
    sc->dstH = dstH;
    sc->filter = filter;
    sc->valid = 1;
    return 1;
}

/* horizontal pass of one source row into a spread row (cached) */
static const uint32_t *scale_row(Scaler *sc, const uint16_t *src, int pitch, int y, int keep) {
    int r, x;
    uint32_t *out;
    const uint16_t *line;
    for (r = 0; r < SCALE_ROWS; r++) if (sc->rowY[r] == y) return sc->rows[r];
    /* replace the oldest row that the current output row does not use
     * (rows keep..keep+2); rows are visited in increasing order */
    r = -1;
    for (x = 0; x < SCALE_ROWS; x++) {
        if (sc->rowY[x] >= keep && sc->rowY[x] <= keep + SCALE_MAX_TAPS - 1) continue;
        if (r < 0 || sc->rowY[x] < sc->rowY[r]) r = x;
    }
    sc->rowY[r] = y;
    out = sc->rows[r];
    line = src + y * pitch;
    for (x = 0; x < sc->dstW; x++) {
        const ScaleTap *t = &sc->xt[x];
        const uint16_t *p = line + t->idx;
        uint32_t acc;
        if (t->n == 1) {
            out[x] = scale_spread(p[0]);
            continue;
        }
        acc = scale_spread(p[0]) * t->w[0] + scale_spread(p[1]) * t->w[1];
        if (t->n == 3) acc += scale_spread(p[2]) * t->w[2];
        out[x] = (acc >> 5) & 0x07E0F81Fu;
    }
    return out;
}

static void scale_run(Scaler *sc, const uint16_t *src, int srcPitch,
                      uint16_t *dst, int dstPitch) {
    int x, y, r;
    if (!sc->valid) return;
    if (sc->nearest) {
        for (y = 0; y < sc->dstH; y++) {
            const uint16_t *line = src + sc->yt[y].idx * srcPitch;
            uint16_t *o = dst + y * dstPitch;
            for (x = 0; x < sc->dstW; x++) o[x] = line[sc->xt[x].idx];
        }
        return;
    }
    for (r = 0; r < SCALE_ROWS; r++) sc->rowY[r] = -1;
    for (y = 0; y < sc->dstH; y++) {
        const ScaleTap *t = &sc->yt[y];
        uint16_t *o = dst + y * dstPitch;
        const uint32_t *r0 = scale_row(sc, src, srcPitch, t->idx, t->idx);
        if (t->n == 1) {
            for (x = 0; x < sc->dstW; x++) o[x] = scale_unspread(r0[x]);
        } else {
            const uint32_t *r1 = scale_row(sc, src, srcPitch, t->idx + 1, t->idx);
            uint32_t w0 = t->w[0], w1 = t->w[1];
            if (t->n == 3) {
                const uint32_t *r2 = scale_row(sc, src, srcPitch, t->idx + 2, t->idx);
                uint32_t w2 = t->w[2];
                for (x = 0; x < sc->dstW; x++)
                    o[x] = scale_unspread(((r0[x] * w0 + r1[x] * w1 + r2[x] * w2) >> 5) & 0x07E0F81Fu);
            } else {
                for (x = 0; x < sc->dstW; x++)
                    o[x] = scale_unspread(((r0[x] * w0 + r1[x] * w1) >> 5) & 0x07E0F81Fu);
            }
        }
    }
}

#endif
