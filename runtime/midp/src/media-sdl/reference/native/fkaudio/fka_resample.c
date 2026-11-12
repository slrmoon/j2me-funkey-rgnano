/* Sound quality level and the windowed-sinc kernels of the high one. */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fka_config.h"
#include "fka_resample.h"

int fka_quality_high(void)
{
    static int high = -1;
    if (high < 0) {
        const char *env = FKA_CONFIG("PHONEME_AUDIO_QUALITY");
        if (env != NULL && env[0] != '\0') {
            high = strcmp(env, "low") != 0 && strcmp(env, "0") != 0;
        } else {
#if defined(__arm__) || defined(__aarch64__)
            high = 0;       /* the handheld */
#else
            high = 1;       /* desktop and browser builds */
#endif
        }
    }
    return high;
}

#define TABLES 6
static struct {
    int cutoff_q10;      /* 1024 = the source's own Nyquist limit */
    int16_t *k;
} g_tables[TABLES];

static double bessel_i0(double x)
{
    double sum = 1.0, term = 1.0;
    int n;
    for (n = 1; n < 32; ++n) {
        term *= (x / (2.0 * n)) * (x / (2.0 * n));
        sum += term;
    }
    return sum;
}

const int16_t *fka_sinc_table(int src_rate, int dst_rate)
{
    /* reading faster than writing (44.1 kHz sound into the 22.05 kHz mix):
     * the kernel must also remove what would fold back */
    int cutoff_q10 = src_rate > dst_rate ? (int) ((int64_t) dst_rate * 1024 / src_rate) : 1024;
    const double pi = 3.14159265358979323846, beta = 7.0;
    double cutoff, i0b;
    int16_t *k;
    int t, p, i;
    if (cutoff_q10 < 128) cutoff_q10 = 128;
    cutoff_q10 = (cutoff_q10 + 16) & ~31;
    if (cutoff_q10 > 1024) cutoff_q10 = 1024;
    for (t = 0; t < TABLES; ++t) {
        if (g_tables[t].k != NULL && g_tables[t].cutoff_q10 == cutoff_q10) return g_tables[t].k;
    }
    for (t = 0; t < TABLES && g_tables[t].k != NULL; ++t) { }
    if (t == TABLES) return g_tables[0].k;      /* odd rate after five others */
    k = (int16_t *) malloc(sizeof(int16_t) * FKA_SINC_PHASES * FKA_SINC_TAPS);
    if (k == NULL) return NULL;
    /* a little below the limit: the transition band of the kernel */
    cutoff = cutoff_q10 / 1024.0 * 0.94;
    i0b = bessel_i0(beta);
    for (p = 0; p < FKA_SINC_PHASES; ++p) {
        double frac = (double) p / FKA_SINC_PHASES, w[FKA_SINC_TAPS], sum = 0.0;
        for (i = 0; i < FKA_SINC_TAPS; ++i) {
            double x = (double) (i - (FKA_SINC_TAPS / 2 - 1)) - frac;   /* tap position - read position */
            double r = x / (FKA_SINC_TAPS / 2);
            double win = r * r < 1.0 ? bessel_i0(beta * sqrt(1.0 - r * r)) / i0b : 0.0;
            double s = x == 0.0 ? cutoff : sin(pi * cutoff * x) / (pi * x);
            w[i] = s * win;
            sum += w[i];
        }
        for (i = 0; i < FKA_SINC_TAPS; ++i)
            k[p * FKA_SINC_TAPS + i] = (int16_t) floor(w[i] / sum * 16384.0 + 0.5);
    }
    g_tables[t].cutoff_q10 = cutoff_q10;
    g_tables[t].k = k;
    return k;
}
