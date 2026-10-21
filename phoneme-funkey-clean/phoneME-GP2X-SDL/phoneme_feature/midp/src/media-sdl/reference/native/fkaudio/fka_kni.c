/*
 * FunKey audio engine: KNI glue for javax.microedition.media.FkaNative and
 * the C entry points used by the SDL UI port (lfjport_st_export.c).
 *
 * Environment:
 *   PHONEME_SYNTH       hybrid (default) | wavetable | fm
 *   PHONEME_SOUNDBANK   optional DLS / SF2 bank replacing the built-in one
 *                       (default: <midp home>/lib/soundbank.dls or .sf2)
 *   PHONEME_SOUND_VOLUME master volume 0..100
 */
#include <kni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fka.h"

extern const char *midpGetHomeDir(void);

static int g_audio_state;   /* 0 = not tried, 1 = ready, < 0 = failed */

static int file_exists(const char *path)
{
    return path != NULL && access(path, R_OK) == 0;
}

static void load_user_bank(void)
{
    const char *env = getenv("PHONEME_SOUNDBANK");
    const char *home;
    char path[512];
    if (env != NULL && env[0] != '\0') {
        if (fka_load_soundbank(env) != 0) fprintf(stderr, "fkaudio: cannot load sound bank %s\n", env);
        return;
    }
    home = midpGetHomeDir();
    if (home == NULL) return;
    snprintf(path, sizeof(path), "%s/lib/soundbank.dls", home);
    if (!file_exists(path)) snprintf(path, sizeof(path), "%s/lib/soundbank.sf2", home);
    if (file_exists(path) && fka_load_soundbank(path) != 0)
        fprintf(stderr, "fkaudio: cannot load sound bank %s\n", path);
}

int InitAudioSubsystem(void)
{
    const char *synth = getenv("PHONEME_SYNTH");
    const char *vol = getenv("PHONEME_SOUND_VOLUME");
    int r;
    if (g_audio_state != 0) return g_audio_state > 0 ? 0 : g_audio_state;
    if (synth != NULL) {
        if (strcmp(synth, "wavetable") == 0 || strcmp(synth, "wt") == 0) fka_set_synth(FKA_SYNTH_WAVETABLE);
        else if (strcmp(synth, "fm") == 0) fka_set_synth(FKA_SYNTH_FM);
        else fka_set_synth(FKA_SYNTH_HYBRID);
    }
    if (vol != NULL && vol[0] != '\0') fka_set_master_volume(atoi(vol));
    r = fka_init();
    if (r == 0) {
        load_user_bank();
        r = fka_output_open();
        if (r != 0) {
            fprintf(stderr, "fkaudio: cannot open audio output (%d)\n", r);
            r -= 10;
            fka_shutdown();
        }
    } else {
        fprintf(stderr, "fkaudio: engine init failed (%d)\n", r);
    }
    g_audio_state = r == 0 ? 1 : r;
    return r;
}

void FinalizeAudioSubsystem(void)
{
    if (g_audio_state != 1) return;
    fka_output_close();
    fka_shutdown();
    g_audio_state = 0;
}

void MediaSDL_SetMasterVolume(int volume)
{
    fka_lock();
    fka_set_master_volume(volume);
    fka_unlock();
}

int MediaSDL_GetMasterVolume(void)
{
    return fka_get_master_volume();
}

void MediaSDL_SetSynth(int engine)
{
    fka_lock();
    fka_set_synth(engine);
    fka_unlock();
}

int MediaSDL_GetSynth(void)
{
    return fka_get_synth();
}

/* system menu open: freeze every player (the MIDlet is frozen too) */
void MediaSDL_PauseAll(int paused)
{
    fka_lock();
    fka_pause_all(paused);
    fka_unlock();
}

/* ------------------------------------------------------------------ */

static uint8_t *copy_bytes(jobject handle, int offset, int len)
{
    uint8_t *buf;
    if (len <= 0) return NULL;
    buf = (uint8_t *) malloc((size_t) len);
    if (buf != NULL) KNI_GetRawArrayRegion(handle, offset, len, (jbyte *) buf);
    return buf;
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nInit(void)
{
    KNI_ReturnInt(InitAudioSubsystem());
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nNew(void)
{
    int id = -1, len = KNI_GetParameterAsInt(2), n, total;
    char hint[64];
    uint8_t *buf;
    KNI_StartHandles(2);
    KNI_DeclareHandle(data);
    KNI_DeclareHandle(type);
    KNI_GetParameterAsObject(1, data);
    KNI_GetParameterAsObject(3, type);
    hint[0] = '\0';
    if (!KNI_IsNullHandle(type)) {
        jchar tmp[63];
        int i;
        n = KNI_GetStringLength(type);
        if (n > 63) n = 63;
        if (n > 0) KNI_GetStringRegion(type, 0, n, tmp);
        for (i = 0; i < n; ++i) hint[i] = tmp[i] < 128 ? (char) tmp[i] : '?';
        hint[n > 0 ? n : 0] = '\0';
    }
    if (!KNI_IsNullHandle(data)) {
        total = KNI_GetArrayLength(data);
        if (len > total) len = total;
        buf = copy_bytes(data, 0, len);
        if (buf != NULL) {
            id = fka_player_new(buf, len, hint[0] ? hint : NULL);
            free(buf);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(id);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nNewDevice(void)
{
    KNI_ReturnInt(fka_player_new_device());
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nNewToneSequence(void)
{
    int id;
    uint8_t *buf = NULL;
    int len = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(seq);
    KNI_GetParameterAsObject(1, seq);
    if (!KNI_IsNullHandle(seq)) {
        len = KNI_GetArrayLength(seq);
        buf = copy_bytes(seq, 0, len);
    }
    id = fka_player_new_tone_sequence(buf, buf != NULL ? len : 0);
    free(buf);
    KNI_EndHandles();
    KNI_ReturnInt(id);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nReplaceToneSequence(void)
{
    int r = -1, id = KNI_GetParameterAsInt(1), len;
    uint8_t *buf;
    KNI_StartHandles(1);
    KNI_DeclareHandle(seq);
    KNI_GetParameterAsObject(2, seq);
    if (!KNI_IsNullHandle(seq)) {
        len = KNI_GetArrayLength(seq);
        buf = copy_bytes(seq, 0, len);
        if (buf != NULL) {
            r = fka_player_replace_tone_sequence(id, buf, len);
            free(buf);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(r);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nDelete(void)
{
    fka_player_delete(KNI_GetParameterAsInt(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nKind(void)
{
    KNI_ReturnInt(fka_player_kind(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_OBJECT Java_javax_microedition_media_FkaNative_nContentType(void)
{
    const char *t = fka_player_content_type(KNI_GetParameterAsInt(1));
    KNI_StartHandles(1);
    KNI_DeclareHandle(str);
    KNI_NewStringUTF(t, str);
    KNI_EndHandlesAndReturnObject(str);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nPrefetch(void)
{
    KNI_ReturnInt(fka_player_prefetch(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nDeallocate(void)
{
    fka_player_deallocate(KNI_GetParameterAsInt(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nStart(void)
{
    KNI_ReturnInt(fka_player_start(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nStop(void)
{
    fka_player_stop(KNI_GetParameterAsInt(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_javax_microedition_media_FkaNative_nIsPlaying(void)
{
    KNI_ReturnBoolean(fka_player_is_playing(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nSetLoopCount(void)
{
    fka_player_set_loop_count(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_FkaNative_nGetTime(void)
{
    KNI_ReturnLong(fka_player_get_time(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_FkaNative_nSetTime(void)
{
    jint id = KNI_GetParameterAsInt(1);
    jlong us = KNI_GetParameterAsLong(2);
    KNI_ReturnLong(fka_player_set_time(id, us));
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_FkaNative_nGetDuration(void)
{
    KNI_ReturnLong(fka_player_get_duration(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nSetVolume(void)
{
    fka_player_set_volume(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2),
                          KNI_GetParameterAsBoolean(3) ? 1 : 0);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nSetStopTime(void)
{
    jint id = KNI_GetParameterAsInt(1);
    jlong us = KNI_GetParameterAsLong(2);
    fka_player_set_stop_time(id, us);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nSetRate(void)
{
    KNI_ReturnInt(fka_player_set_rate(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2)));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nGetRate(void)
{
    KNI_ReturnInt(fka_player_get_rate(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nGetTempo(void)
{
    KNI_ReturnInt(fka_player_get_tempo(KNI_GetParameterAsInt(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nSetTempo(void)
{
    KNI_ReturnInt(fka_player_set_tempo(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2)));
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nSetPitch(void)
{
    fka_player_set_pitch(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nMidiShort(void)
{
    fka_midi_short(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2),
                   KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nMidiLong(void)
{
    jint id = KNI_GetParameterAsInt(1);
    jint off = KNI_GetParameterAsInt(3);
    jint len = KNI_GetParameterAsInt(4);
    uint8_t *buf;
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    KNI_GetParameterAsObject(2, data);
    if (!KNI_IsNullHandle(data) && off >= 0 && len > 0 && off + len <= KNI_GetArrayLength(data)) {
        buf = copy_bytes(data, off, len);
        if (buf != NULL) {
            fka_midi_long(id, buf, len);
            free(buf);
        }
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nMidiChannelVolume(void)
{
    KNI_ReturnInt(fka_midi_get_channel_volume(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2)));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nMidiProgram(void)
{
    int bank = 0;
    KNI_ReturnInt(fka_midi_get_program(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2), &bank));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nMidiBank(void)
{
    int bank = -1;
    if (fka_midi_get_program(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2), &bank) < 0) bank = -1;
    KNI_ReturnInt(bank);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_FkaNative_nPlayTone(void)
{
    fka_play_tone(KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_FkaNative_nGetMasterVolume(void)
{
    KNI_ReturnInt(fka_get_master_volume());
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_javax_microedition_media_FkaNative_nPollEvent(void)
{
    int player, type, data, got;
    int64_t arg;
    jlong out[4];
    got = fka_poll_event(&player, &type, &arg, &data);
    KNI_StartHandles(1);
    KNI_DeclareHandle(arr);
    KNI_GetParameterAsObject(1, arr);
    if (got && !KNI_IsNullHandle(arr) && KNI_GetArrayLength(arr) >= 4) {
        out[0] = player;
        out[1] = type;
        out[2] = arg;
        out[3] = data;
        KNI_SetRawArrayRegion(arr, 0, (jsize) sizeof(out), (jbyte *) out);
    } else {
        got = 0;
    }
    KNI_EndHandles();
    KNI_ReturnBoolean(got);
}
