/*
 * FunKey audio engine: Sonivox EAS synthesizer glue.
 *
 * One EAS instance renders every MIDI player: each player owns an EAS MIDI
 * stream, which in EAS means its own virtual synthesizer (16 channels,
 * programs, controllers), so players never disturb each other's channels.
 * Per-player volume and transposition are applied on that virtual synth.
 * Must be called with the engine lock held.
 */
#include "fka_eas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eas.h"
#include "eas_reverb.h"
#include "eas_chorus.h"
#include "eas_report.h"
#include "eas_data.h"
#include "eas_synth.h"
#include "eas_miditypes.h"
#include "eas_vm_protos.h"

#ifndef FKA_EAS_DEFAULT_VOLUME
#define FKA_EAS_DEFAULT_VOLUME 90
#endif

static EAS_DATA_HANDLE g_eas;
static const S_EAS_LIB_CONFIG *g_cfg;
static int g_frame;     /* EAS render granularity (samples) */

static S_SYNTH *stream_synth(void *stream);

int fka_eas_init(int engine)
{
    const char *lib_name;
    E_EAS_SNDLIB_TYPE type;
    if (g_eas != NULL) return 0;
    g_cfg = EAS_Config();
    if (g_cfg == NULL || g_cfg->numChannels != 1 || g_cfg->sampleRate != 22050) return -1;
    g_frame = g_cfg->mixBufferSize;
    EAS_SetDebugLevel(1);
    if (EAS_Init(&g_eas) != EAS_SUCCESS) {
        g_eas = NULL;
        return -2;
    }
    type = engine == 1 ? EAS_SNDLIB_WT : (engine == 2 ? EAS_SNDLIB_FM : EAS_SNDLIB_HYBRID);
    lib_name = EAS_GetDefaultSoundLibrary(type);
    if (lib_name == NULL) lib_name = EAS_GetDefaultSoundLibrary(EAS_SNDLIB_HYBRID);
    if (lib_name != NULL)
        EAS_SetSoundLibrary(g_eas, NULL, EAS_GetSoundLibrary(g_eas, lib_name));
    EAS_SetParameter(g_eas, EAS_MODULE_REVERB, EAS_PARAM_REVERB_BYPASS, EAS_TRUE);
    EAS_SetParameter(g_eas, EAS_MODULE_CHORUS, EAS_PARAM_CHORUS_BYPASS, EAS_TRUE);
    /* EAS volume is in dB (100 = 0 dB). At 0 dB dense GM songs sit in
     * the limiter a tenth of the time; handsets ran it lower too. */
    {
        const char *env = getenv("PHONEME_MIDI_GAIN");
        int v = env != NULL && env[0] != '\0' ? atoi(env) : FKA_EAS_DEFAULT_VOLUME;
        if (v < 50) v = 50;
        if (v > 100) v = 100;
        EAS_SetVolume(g_eas, NULL, v);
    }
    return 0;
}

void fka_eas_shutdown(void)
{
    if (g_eas != NULL) EAS_Shutdown(g_eas);
    g_eas = NULL;
}

int fka_eas_frame(void)
{
    return g_frame;
}

int fka_eas_set_engine(int engine)
{
    const char *lib_name;
    E_EAS_SNDLIB_TYPE type;
    if (g_eas == NULL) return -1;
    type = engine == 1 ? EAS_SNDLIB_WT : (engine == 2 ? EAS_SNDLIB_FM : EAS_SNDLIB_HYBRID);
    lib_name = EAS_GetDefaultSoundLibrary(type);
    if (lib_name == NULL) return -2;
    return EAS_SetSoundLibrary(g_eas, NULL, EAS_GetSoundLibrary(g_eas, lib_name)) == EAS_SUCCESS ? 0 : -3;
}

void fka_eas_stream_engine(void *stream, int engine)
{
    const char *lib_name;
    E_EAS_SNDLIB_TYPE type;
    if (g_eas == NULL || stream == NULL) return;
    type = engine == 1 ? EAS_SNDLIB_WT : (engine == 2 ? EAS_SNDLIB_FM : EAS_SNDLIB_HYBRID);
    lib_name = EAS_GetDefaultSoundLibrary(type);
    if (lib_name == NULL) return;
    fka_eas_silence(stream, 1);
    if (EAS_SetSoundLibrary(g_eas, (EAS_HANDLE) stream, EAS_GetSoundLibrary(g_eas, lib_name)) == EAS_SUCCESS) {
        /* channels cache a region index into the old library: resolve the
         * current program again in the new one */
        S_SYNTH *syn = stream_synth(stream);
        int ch;
        for (ch = 0; syn != NULL && ch < 16; ++ch)
            VMProgramChange(g_eas->pVoiceMgr, syn, (EAS_U8) ch, syn->channels[ch].programNum);
    }
}

void *fka_eas_open(void)
{
    EAS_HANDLE h = NULL;
    if (g_eas == NULL) return NULL;
    if (EAS_OpenMIDIStream(g_eas, &h, NULL) != EAS_SUCCESS) return NULL;
    return h;
}

void fka_eas_close(void *stream)
{
    if (g_eas != NULL && stream != NULL) EAS_CloseMIDIStream(g_eas, (EAS_HANDLE) stream);
}

void fka_eas_write(void *stream, const uint8_t *msg, int len)
{
    if (g_eas != NULL && stream != NULL && len > 0)
        EAS_WriteMIDIStream(g_eas, (EAS_HANDLE) stream, (EAS_U8 *) msg, len);
}

static S_SYNTH *stream_synth(void *stream)
{
    S_EAS_STREAM *s = (S_EAS_STREAM *) stream;
    S_INTERACTIVE_MIDI *im;
    if (s == NULL || s->handle == NULL) return NULL;
    im = (S_INTERACTIVE_MIDI *) s->handle;
    return im->pSynth;
}

void fka_eas_volume(void *stream, int gain_q15)
{
    S_SYNTH *syn = stream_synth(stream);
    if (gain_q15 < 0) gain_q15 = 0;
    if (gain_q15 > 0x7fff) gain_q15 = 0x7fff;
    if (syn != NULL) VMSetVolume(syn, (EAS_U16) gain_q15);
}

void fka_eas_transpose(void *stream, int semitones)
{
    S_SYNTH *syn = stream_synth(stream);
    if (syn != NULL) VMSetTranposition(syn, semitones);
}

/* All notes released (MMAPI stop) or silenced at once (close/seek). */
void fka_eas_silence(void *stream, int immediate)
{
    S_SYNTH *syn = stream_synth(stream);
    if (syn == NULL) return;
    if (immediate) VMMuteAllVoices(g_eas->pVoiceMgr, syn);
    else VMReleaseAllVoices(g_eas->pVoiceMgr, syn);
}

/* GM reset of all 16 channels of a stream (programs, controllers, bend),
 * used before a song (re)starts from the beginning or after a seek. */
void fka_eas_reset(void *stream)
{
    S_SYNTH *syn = stream_synth(stream);
    int ch;
    if (syn == NULL) return;
    VMMuteAllVoices(g_eas->pVoiceMgr, syn);
    for (ch = 0; ch < 16; ++ch) {
        uint8_t m[3];
        m[0] = (uint8_t) (0xb0 | ch); m[1] = 121; m[2] = 0;   /* reset controllers */
        fka_eas_write(stream, m, 3);
        m[1] = 0; m[2] = ch == 9 ? 0x78 : 0x79;               /* GM2 bank */
        fka_eas_write(stream, m, 3);
        m[1] = 32; m[2] = 0;
        fka_eas_write(stream, m, 3);
        m[1] = 7; m[2] = 100;
        fka_eas_write(stream, m, 3);
        m[1] = 10; m[2] = 64;
        fka_eas_write(stream, m, 3);
        m[0] = (uint8_t) (0xc0 | ch); m[1] = 0;
        fka_eas_write(stream, m, 2);
        m[0] = (uint8_t) (0xe0 | ch); m[1] = 0; m[2] = 64;
        fka_eas_write(stream, m, 3);
    }
}

int fka_eas_channel_volume(void *stream, int ch)
{
    S_SYNTH *syn = stream_synth(stream);
    if (syn == NULL || ch < 0 || ch > 15) return -1;
    return syn->channels[ch].volume;
}

int fka_eas_channel_program(void *stream, int ch, int *bank)
{
    S_SYNTH *syn = stream_synth(stream);
    if (syn == NULL || ch < 0 || ch > 15) return -1;
    if (bank != NULL) *bank = (int) syn->channels[ch].bankNum;
    return syn->channels[ch].programNum;
}

int fka_eas_active(void *stream)
{
    S_SYNTH *syn = stream_synth(stream);
    return syn != NULL ? syn->numActiveVoices : 0;
}

int fka_eas_render(int16_t *out)
{
    EAS_I32 n = 0;
    if (g_eas == NULL) return 0;
    if (EAS_Render(g_eas, (EAS_PCM *) out, g_frame, &n) != EAS_SUCCESS) return 0;
    return (int) n;
}

/* ---- external DLS / SF2 sound banks ---- */

static int file_read_at(void *handle, void *buf, int offset, int size)
{
    FILE *f = (FILE *) handle;
    if (fseek(f, offset, SEEK_SET) != 0) return 0;
    return (int) fread(buf, 1, (size_t) size, f);
}

static int file_size(void *handle)
{
    FILE *f = (FILE *) handle;
    long cur = ftell(f), end;
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, cur, SEEK_SET);
    return (int) end;
}

int fka_eas_load_bank(const char *path)
{
    EAS_FILE loc;
    EAS_RESULT r;
    FILE *f;
    if (g_eas == NULL || path == NULL) return -1;
    f = fopen(path, "rb");
    if (f == NULL) return -2;
    loc.handle = f;
    loc.readAt = file_read_at;
    loc.size = file_size;
    r = EAS_LoadDLSCollection(g_eas, NULL, &loc);
    fclose(f);
    return r == EAS_SUCCESS ? 0 : -3;
}
