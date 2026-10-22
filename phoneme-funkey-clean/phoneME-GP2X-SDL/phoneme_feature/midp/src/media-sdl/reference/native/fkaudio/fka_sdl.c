/*
 * FunKey audio engine: SDL 1.2 output driver.
 *
 * FunKey OS routes the default ALSA device through "monocard", a route
 * plugin that adds left and right into the single (right) channel wired to
 * the speaker amplifier. Writing the same sample to both channels would
 * double it and clip, so on that setup each channel gets half the sample;
 * elsewhere (desktop builds) both channels get the full sample.
 *
 * PHONEME_AUDIO_BUFFER  output buffer in samples (default 1024, ~46 ms)
 * PHONEME_AUDIO_SUM     1/0 forces the halving on/off
 */
#include "fka.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_open;
static int g_channels;
static int g_sum;
static int g_rate;
static int16_t *g_buf;
static int g_buf_cap;
/* resampler state when the device refused 22050 Hz */
static uint32_t g_phase, g_step;      /* 16.16 */
static int16_t g_prev, g_next;
static int g_have_next;

static int detect_monocard(void)
{
    char line[256];
    const char *env = getenv("PHONEME_AUDIO_SUM");
    FILE *f;
    int found = 0;
    if (env != NULL && env[0] != '\0') return atoi(env) != 0;
    f = fopen("/etc/asound.conf", "r");
    if (f == NULL) return 0;
    while (!found && fgets(line, sizeof(line), f) != NULL) {
        if (strstr(line, "pcm.!default") != NULL && strstr(line, "monocard") != NULL) found = 1;
    }
    fclose(f);
    return found;
}

static int16_t next_sample(void)
{
    int16_t s;
    fka_render(&s, 1);
    return s;
}

void PhoneMEProfileThread(void);   /* lfjport_profile.c: sample this thread too */

static void callback(void *user, Uint8 *stream, int bytes)
{
    int16_t *out = (int16_t *) stream;
    int frames = bytes / (int) (sizeof(int16_t) * (size_t) g_channels);
    int i;
    (void) user;
    PhoneMEProfileThread();
    if (frames > g_buf_cap) frames = g_buf_cap;

    if (g_rate == FKA_RATE) {
        fka_render(g_buf, frames);
    } else {
        /* linear interpolation, pulling engine samples one at a time is
         * fine: fka_render keeps a frame carry buffer internally */
        for (i = 0; i < frames; ++i) {
            if (!g_have_next) {
                g_prev = next_sample();
                g_next = next_sample();
                g_have_next = 1;
            }
            g_buf[i] = (int16_t) (g_prev + (((int) g_next - g_prev) * (int) (g_phase & 0xffff) >> 16));
            g_phase += g_step;
            while (g_phase >= 0x10000) {
                g_phase -= 0x10000;
                g_prev = g_next;
                g_next = next_sample();
            }
        }
    }

    if (g_channels == 1) {
        memcpy(out, g_buf, sizeof(int16_t) * (size_t) frames);
    } else {
        for (i = 0; i < frames; ++i) {
            int16_t s = g_sum ? (int16_t) (g_buf[i] / 2) : g_buf[i];
            int c;
            for (c = 0; c < g_channels; ++c) *out++ = s;
        }
    }
}

int fka_output_open(void)
{
    SDL_AudioSpec want, got;
    const char *env = getenv("PHONEME_AUDIO_BUFFER");
    int samples = 1024;
    if (g_open) return 0;
    if (env != NULL && atoi(env) >= 128 && atoi(env) <= 8192) samples = atoi(env);
    /* SDL wants a power of two */
    {
        int p = 128;
        while (p < samples) p <<= 1;
        samples = p;
    }
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return -1;
    memset(&want, 0, sizeof(want));
    want.freq = FKA_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = (Uint16) samples;
    want.callback = callback;
    if (SDL_OpenAudio(&want, &got) != 0) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -2;
    }
    if (got.format != AUDIO_S16SYS || got.channels < 1) {
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -3;
    }
    g_channels = got.channels;
    g_rate = got.freq;
    g_step = (uint32_t) (((uint64_t) FKA_RATE << 16) / (uint64_t) g_rate);
    g_phase = 0;
    g_have_next = 0;
    g_sum = g_channels == 2 && detect_monocard();
    g_buf_cap = got.samples > 0 ? got.samples * 2 : 8192;
    g_buf = (int16_t *) malloc(sizeof(int16_t) * (size_t) g_buf_cap);
    if (g_buf == NULL) {
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -4;
    }
    g_open = 1;
    SDL_PauseAudio(0);
    if (getenv("PHONEME_AUDIO_TRACE") != NULL)
        fprintf(stderr, "fkaudio: %d Hz, %d ch, %d samples, sum=%d\n",
                g_rate, g_channels, got.samples, g_sum);
    return 0;
}

void fka_output_close(void)
{
    if (!g_open) return;
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    free(g_buf);
    g_buf = NULL;
    g_open = 0;
}

void fka_lock(void)
{
    if (g_open) SDL_LockAudio();
}

void fka_unlock(void)
{
    if (g_open) SDL_UnlockAudio();
}
