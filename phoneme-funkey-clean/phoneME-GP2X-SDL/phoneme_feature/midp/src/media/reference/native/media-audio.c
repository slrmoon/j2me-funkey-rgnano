#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <kni.h>
#include <SDL.h>
#include "tinysoundfont/tml.h"
#include "w200-synth.h"

#define AUDIO_RATE 22050
#define AUDIO_BLOCK 1024
#define MIDI_MAX_MS 30000
#define MIDI_DENSE_MAX_MS 12000
#define MIDI_MEDIUM_MAX_MS 20000
#define MIDI_RELEASE_RENDER_MS 1000
#define MIDI_RENDER_BLOCK 512
#define MIDI_MAX_MESSAGES 200000
#define MIDI_MAX_PCM_BYTES (8U * 1024U * 1024U)
#define TONE_FADE_FRAMES 256
#define MIDI_ONESHOT_MAX_FRAMES (AUDIO_RATE * 3)
#define MIDI_STOP_FADE_FRAMES (AUDIO_RATE / 80)

struct MidiPlayer {
    short *pcm;
    unsigned int frames;
    unsigned int pos;
    int playing;
    int finished;
    int loop_remaining;
    int volume;
    int auto_close;
    unsigned int stop_fade_remaining;
    unsigned int stop_fade_total;
    struct MidiPlayer *next;
};

static int audio_ready = 0;
static struct MidiPlayer *players = NULL;

static double midi_note_frequency(int note);
static unsigned int read_le32(const unsigned char *p);
static unsigned int read_le16(const unsigned char *p);
static int safe_midi_get_info(tml_message *midi, int *used_channels,
                              int *used_programs, int *total_notes,
                              unsigned int *first_note,
                              unsigned int *length_ms);
static unsigned int midi_render_limit_ms(unsigned int length_ms,
                                         int total_notes);
static void polish_midi_pcm(short *pcm, unsigned int frames);

static void audio_log(const char *message)
{
    printf("phoneME audio native: %s\n", message);
    fflush(stdout);
}

static short clip_sample(int value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (short)value;
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    struct MidiPlayer *player;
    struct MidiPlayer **cursor;
    short *out;
    int samples;
    int i;
    int left;
    int right;
    int sample_left;
    int sample_right;
    unsigned int fade_remaining;
    unsigned int fade_total;
    (void)userdata;

    memset(stream, 0, (size_t)len);
    out = (short *)stream;
    samples = len / (int)sizeof(short) / 2;

    cursor = &players;
    while (*cursor != NULL) {
        player = *cursor;
        if (!player->playing || player->pcm == NULL || player->frames == 0) {
            if (player->auto_close && player->finished) {
                *cursor = player->next;
                if (player->pcm != NULL) {
                    free(player->pcm);
                }
                free(player);
            } else {
                cursor = &(player->next);
            }
            continue;
        }
        for (i = 0; i < samples; i++) {
            if (player->pos >= player->frames) {
                if (player->loop_remaining < 0) {
                    player->pos = 0;
                } else if (player->loop_remaining > 1) {
                    player->loop_remaining--;
                    player->pos = 0;
                } else {
                    player->playing = 0;
                    player->finished = 1;
                    break;
                }
            }
            sample_left = ((int)player->pcm[player->pos * 2] *
                    player->volume) / 100;
            sample_right = ((int)player->pcm[player->pos * 2 + 1] *
                    player->volume) / 100;
            if (player->stop_fade_remaining > 0) {
                fade_remaining = player->stop_fade_remaining;
                fade_total = player->stop_fade_total;
                if (fade_total == 0) {
                    fade_total = 1;
                }
                sample_left = (sample_left * (int)fade_remaining) /
                        (int)fade_total;
                sample_right = (sample_right * (int)fade_remaining) /
                        (int)fade_total;
                player->stop_fade_remaining--;
                if (player->stop_fade_remaining == 0) {
                    player->playing = 0;
                    player->finished = 1;
                    break;
                }
            }
            left = (int)out[i * 2] + sample_left;
            right = (int)out[i * 2 + 1] + sample_right;
            out[i * 2] = clip_sample(left);
            out[i * 2 + 1] = clip_sample(right);
            player->pos++;
        }
        if (player->auto_close && !player->playing && player->finished) {
            *cursor = player->next;
            if (player->pcm != NULL) {
                free(player->pcm);
            }
            free(player);
        } else {
            cursor = &(player->next);
        }
    }
}

static int ensure_audio(void)
{
    SDL_AudioSpec spec;

    if (audio_ready) {
        return 0;
    }
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 &&
            SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        printf("phoneME audio native: SDL_InitSubSystem audio failed: %s\n",
               SDL_GetError());
        fflush(stdout);
        return -1;
    }
    memset(&spec, 0, sizeof(spec));
    spec.freq = AUDIO_RATE;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = AUDIO_BLOCK;
    spec.callback = audio_callback;
    if (SDL_OpenAudio(&spec, NULL) != 0) {
        printf("phoneME audio native: SDL_OpenAudio failed: %s\n", SDL_GetError());
        fflush(stdout);
        return -2;
    }
    SDL_PauseAudio(0);
    audio_ready = 1;
    audio_log("SDL audio ready");
    return 0;
}

static void fade_out_tail(short *pcm, unsigned int frames,
                          unsigned int fade_frames)
{
    unsigned int fade;
    unsigned int i;
    unsigned int start;
    int scale;

    if (pcm == NULL || frames == 0) {
        return;
    }
    fade = fade_frames;
    if (fade > frames) {
        fade = frames;
    }
    start = frames - fade;
    for (i = 0; i < fade; i++) {
        scale = (int)((fade - i) * 256 / fade);
        pcm[(start + i) * 2] =
                (short)(((int)pcm[(start + i) * 2] * scale) / 256);
        pcm[(start + i) * 2 + 1] =
                (short)(((int)pcm[(start + i) * 2 + 1] * scale) / 256);
    }
}

static double midi_note_frequency(int note)
{
    return 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
}

static unsigned int read_le32(const unsigned char *p)
{
    return ((unsigned int)p[0]) |
            ((unsigned int)p[1] << 8) |
            ((unsigned int)p[2] << 16) |
            ((unsigned int)p[3] << 24);
}

static unsigned int read_le16(const unsigned char *p)
{
    return ((unsigned int)p[0]) | ((unsigned int)p[1] << 8);
}

static int safe_midi_get_info(tml_message *midi, int *used_channels,
                              int *used_programs, int *total_notes,
                              unsigned int *first_note,
                              unsigned int *length_ms)
{
    tml_message *msg;
    unsigned char channels[16];
    unsigned char programs[128];
    unsigned int count;

    memset(channels, 0, sizeof(channels));
    memset(programs, 0, sizeof(programs));
    *used_channels = 0;
    *used_programs = 0;
    *total_notes = 0;
    *first_note = 0xffffffffU;
    *length_ms = 0;
    count = 0;

    for (msg = midi; msg != NULL && count < MIDI_MAX_MESSAGES;
            msg = msg->next, count++) {
        int channel;
        int program;

        if (msg->time > *length_ms) {
            *length_ms = msg->time;
        }
        switch (msg->type) {
        case TML_PROGRAM_CHANGE:
            program = msg->program & 0x7f;
            if (!programs[program]) {
                programs[program] = 1;
                (*used_programs)++;
            }
            break;
        case TML_NOTE_ON:
            if ((msg->velocity & 0x7f) == 0) {
                break;
            }
            channel = msg->channel & 0x0f;
            if (*first_note == 0xffffffffU) {
                *first_note = msg->time;
            }
            if (!channels[channel]) {
                channels[channel] = 1;
                (*used_channels)++;
            }
            (*total_notes)++;
            break;
        }
    }
    if (*first_note == 0xffffffffU) {
        *first_note = 0;
    }
    if (count >= MIDI_MAX_MESSAGES) {
        audio_log("midi message scan truncated");
        return -1;
    }
    return 0;
}

static unsigned int midi_render_limit_ms(unsigned int length_ms,
                                         int total_notes)
{
    unsigned int limit_ms;

    limit_ms = length_ms + MIDI_RELEASE_RENDER_MS;
    if (total_notes > 1000 && limit_ms > MIDI_DENSE_MAX_MS) {
        limit_ms = MIDI_DENSE_MAX_MS;
    } else if (total_notes > 250 && limit_ms > MIDI_MEDIUM_MAX_MS) {
        limit_ms = MIDI_MEDIUM_MAX_MS;
    } else if (limit_ms > MIDI_MAX_MS) {
        limit_ms = MIDI_MAX_MS;
    }
    if (limit_ms < 500) {
        limit_ms = 500;
    }
    return limit_ms;
}

static int soft_limit_sample(int sample)
{
    int sign;
    int value;

    sign = sample < 0 ? -1 : 1;
    value = sample < 0 ? -sample : sample;
    if (value > 26000) {
        value = 26000 + ((value - 26000) / 6);
    }
    if (value > 32767) {
        value = 32767;
    }
    return sign * value;
}

static void polish_midi_pcm(short *pcm, unsigned int frames)
{
    unsigned int i;
    unsigned int fade_frames;
    int left;
    int right;
    int prev_left;
    int prev_right;
    int smooth_left;
    int smooth_right;

    if (pcm == NULL || frames == 0) {
        return;
    }
    fade_frames = 128;
    if (fade_frames > frames / 2) {
        fade_frames = frames / 2;
    }
    prev_left = 0;
    prev_right = 0;
    for (i = 0; i < frames; i++) {
        left = pcm[i * 2];
        right = pcm[i * 2 + 1];
        smooth_left = (left * 3 + prev_left) / 4;
        smooth_right = (right * 3 + prev_right) / 4;
        prev_left = left;
        prev_right = right;
        left = smooth_left;
        right = smooth_right;
        if (fade_frames > 0 && i < fade_frames) {
            left = (left * (int)i) / (int)fade_frames;
            right = (right * (int)i) / (int)fade_frames;
        } else if (fade_frames > 0 && i + fade_frames >= frames) {
            unsigned int remain;
            remain = frames - i;
            left = (left * (int)remain) / (int)fade_frames;
            right = (right * (int)remain) / (int)fade_frames;
        }
        pcm[i * 2] = clip_sample(soft_limit_sample(left));
        pcm[i * 2 + 1] = clip_sample(soft_limit_sample(right));
    }
}

static int render_tone(struct MidiPlayer *player, int note,
                       int duration_ms, int volume)
{
    short *pcm;
    unsigned int frames;
    unsigned int i;
    unsigned int fade_frames;
    double phase;
    double step;
    int sample;
    int amp;

    if (duration_ms < 1) {
        return -1;
    }
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    frames = (unsigned int)(((double)duration_ms * AUDIO_RATE) / 1000.0);
    if (frames < 1) {
        frames = 1;
    }
    pcm = (short *)calloc((size_t)frames * 2, sizeof(short));
    if (pcm == NULL) {
        return -2;
    }
    phase = 0.0;
    step = (2.0 * 3.14159265358979323846 * midi_note_frequency(note)) /
            (double)AUDIO_RATE;
    amp = (16000 * volume) / 100;
    for (i = 0; i < frames; i++) {
        sample = (int)(sin(phase) * amp);
        pcm[i * 2] = (short)sample;
        pcm[i * 2 + 1] = (short)sample;
        phase += step;
    }
    fade_frames = TONE_FADE_FRAMES;
    if (fade_frames > frames / 2) {
        fade_frames = frames / 2;
    }
    if (fade_frames > 0) {
        fade_out_tail(pcm, frames, fade_frames);
    }

    SDL_LockAudio();
    if (player->pcm != NULL) {
        free(player->pcm);
    }
    player->pcm = pcm;
    player->frames = frames;
    player->pos = 0;
    player->finished = 0;
    player->stop_fade_remaining = 0;
    player->stop_fade_total = 0;
    player->volume = 100;
    SDL_UnlockAudio();
    return 0;
}

static int render_midi(struct MidiPlayer *player,
                       unsigned char *data,
                       unsigned int data_size)
{
    struct W200Synth *synth;
    tml_message *midi;
    tml_message *msg;
    short *pcm;
    int used_channels;
    int used_programs;
    int total_notes;
    unsigned int first_note;
    unsigned int length_ms;
    unsigned int total_frames;
    unsigned int rendered;
    double msec;
    midi = tml_load_memory(data, (int)data_size);
    if (midi == NULL) {
        audio_log("tml_load_memory failed");
        return -1;
    }
    synth = w200_synth_open();
    if (synth == NULL) {
        tml_free(midi);
        return -2;
    }

    if (safe_midi_get_info(midi, &used_channels, &used_programs, &total_notes,
                           &first_note, &length_ms) != 0) {
        w200_synth_close(synth);
        tml_free(midi);
        return -4;
    }
    length_ms = midi_render_limit_ms(length_ms, total_notes);
    total_frames = (unsigned int)(((double)length_ms *
                  (double)AUDIO_RATE) / 1000.0);
    if ((size_t)total_frames * 2 * sizeof(short) > MIDI_MAX_PCM_BYTES) {
        total_frames = MIDI_MAX_PCM_BYTES / (2 * sizeof(short));
    }
    printf("phoneME audio native: midi render begin bytes=%u frames=%u ms=%u notes=%d channels=%d programs=%d\n",
           data_size, total_frames, length_ms, total_notes, used_channels,
           used_programs);
    fflush(stdout);
    pcm = (short *)calloc((size_t)total_frames * 2, sizeof(short));
    if (pcm == NULL) {
        w200_synth_close(synth);
        tml_free(midi);
        return -3;
    }

    msg = midi;
    msec = 0.0;
    rendered = 0;
    while (rendered < total_frames) {
        unsigned int block;
        block = total_frames - rendered;
        if (block > MIDI_RENDER_BLOCK) {
            block = MIDI_RENDER_BLOCK;
        }
        msec += (double)block * 1000.0 / (double)AUDIO_RATE;
        while (msg != NULL && msec >= (double)msg->time) {
            w200_synth_apply_midi(synth, msg);
            msg = msg->next;
        }
        w200_synth_render(synth, pcm + rendered * 2, (int)block);
        rendered += block;
        if (msg == NULL && w200_synth_active_voice_count(synth) == 0 &&
                rendered > AUDIO_RATE / 4) {
            break;
        }
    }
    polish_midi_pcm(pcm, rendered);

    SDL_LockAudio();
    if (player->pcm != NULL) {
        free(player->pcm);
    }
    player->pcm = pcm;
    player->frames = rendered;
    player->pos = 0;
    player->finished = 0;
    player->stop_fade_remaining = 0;
    player->stop_fade_total = 0;
    player->volume = 100;
    SDL_UnlockAudio();

    printf("phoneME audio native: midi rendered backend=%s bytes=%u frames=%u voices=%d notes=%d channels=%d programs=%d\n",
           w200_synth_backend_name(), data_size, rendered, W200_SYNTH_MAX_VOICES,
           total_notes, used_channels, used_programs);
    fflush(stdout);
    w200_synth_close(synth);
    tml_free(midi);
    return 0;
}

static int render_wav(struct MidiPlayer *player,
                      unsigned char *data,
                      unsigned int data_size)
{
    unsigned int offset;
    unsigned int chunk_size;
    unsigned int fmt_offset;
    unsigned int data_offset;
    unsigned int data_bytes;
    unsigned int audio_format;
    unsigned int channels;
    unsigned int sample_rate;
    unsigned int bits_per_sample;
    unsigned int source_frames;
    unsigned int out_frames;
    unsigned int i;
    unsigned int channel;
    unsigned int src_frame;
    unsigned int src_pos;
    short *pcm;
    int left;
    int right;
    int sample;

    if (data_size < 44 || memcmp(data, "RIFF", 4) != 0 ||
            memcmp(data + 8, "WAVE", 4) != 0) {
        audio_log("wav header not found");
        return -1;
    }

    fmt_offset = 0;
    data_offset = 0;
    data_bytes = 0;
    offset = 12;
    while (offset + 8 <= data_size) {
        chunk_size = read_le32(data + offset + 4);
        if (memcmp(data + offset, "fmt ", 4) == 0) {
            fmt_offset = offset + 8;
        } else if (memcmp(data + offset, "data", 4) == 0) {
            data_offset = offset + 8;
            data_bytes = chunk_size;
        }
        offset += 8 + chunk_size + (chunk_size & 1);
    }
    if (fmt_offset == 0 || data_offset == 0 || data_offset >= data_size) {
        audio_log("wav missing fmt/data chunk");
        return -2;
    }
    if (data_offset + data_bytes > data_size) {
        data_bytes = data_size - data_offset;
    }

    audio_format = read_le16(data + fmt_offset);
    channels = read_le16(data + fmt_offset + 2);
    sample_rate = read_le32(data + fmt_offset + 4);
    bits_per_sample = read_le16(data + fmt_offset + 14);
    if (audio_format != 1 || channels < 1 || channels > 2 ||
            sample_rate == 0 ||
            (bits_per_sample != 8 && bits_per_sample != 16)) {
        printf("phoneME audio native: unsupported wav fmt=%u channels=%u rate=%u bits=%u\n",
               audio_format, channels, sample_rate, bits_per_sample);
        fflush(stdout);
        return -3;
    }

    source_frames = data_bytes / (channels * (bits_per_sample / 8));
    if (source_frames == 0) {
        return -4;
    }
    out_frames = (unsigned int)(((unsigned long long)source_frames *
            AUDIO_RATE + sample_rate - 1) / sample_rate);
    if (out_frames < 1) {
        out_frames = 1;
    }
    pcm = (short *)calloc((size_t)out_frames * 2, sizeof(short));
    if (pcm == NULL) {
        return -5;
    }

    for (i = 0; i < out_frames; i++) {
        src_frame = (unsigned int)(((unsigned long long)i * sample_rate) /
                AUDIO_RATE);
        if (src_frame >= source_frames) {
            src_frame = source_frames - 1;
        }
        left = 0;
        right = 0;
        for (channel = 0; channel < channels; channel++) {
            src_pos = data_offset + (src_frame * channels + channel) *
                    (bits_per_sample / 8);
            if (bits_per_sample == 8) {
                sample = ((int)data[src_pos] - 128) << 8;
            } else {
                sample = (int)(short)read_le16(data + src_pos);
            }
            if (channel == 0) {
                left = sample;
                right = sample;
            } else {
                right = sample;
            }
        }
        pcm[i * 2] = clip_sample(left);
        pcm[i * 2 + 1] = clip_sample(right);
    }

    SDL_LockAudio();
    if (player->pcm != NULL) {
        free(player->pcm);
    }
    player->pcm = pcm;
    player->frames = out_frames;
    player->pos = 0;
    player->finished = 0;
    player->stop_fade_remaining = 0;
    player->stop_fade_total = 0;
    player->volume = 100;
    SDL_UnlockAudio();

    printf("phoneME audio native: wav rendered bytes=%u frames=%u sourceFrames=%u rate=%u channels=%u bits=%u\n",
           data_size, out_frames, source_frames, sample_rate, channels,
           bits_per_sample);
    fflush(stdout);
    return 0;
}

static struct MidiPlayer *clone_oneshot_player(struct MidiPlayer *source)
{
    struct MidiPlayer *clone;
    size_t bytes;

    if (source == NULL || source->pcm == NULL || source->frames == 0 ||
            source->frames > MIDI_ONESHOT_MAX_FRAMES) {
        return NULL;
    }
    clone = (struct MidiPlayer *)calloc(1, sizeof(struct MidiPlayer));
    if (clone == NULL) {
        return NULL;
    }
    bytes = (size_t)source->frames * 2 * sizeof(short);
    clone->pcm = (short *)malloc(bytes);
    if (clone->pcm == NULL) {
        free(clone);
        return NULL;
    }
    memcpy(clone->pcm, source->pcm, bytes);
    clone->frames = source->frames;
    clone->pos = 0;
    clone->playing = 1;
    clone->finished = 0;
    clone->loop_remaining = 1;
    clone->volume = source->volume;
    clone->auto_close = 1;
    return clone;
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nAudioInit()
{
    KNI_ReturnInt(ensure_audio());
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nMidiCreate()
{
    struct MidiPlayer *player;

    player = (struct MidiPlayer *)calloc(1, sizeof(struct MidiPlayer));
    if (player == NULL) {
        KNI_ReturnInt(0);
    }
    player->volume = 100;
    SDL_LockAudio();
    player->next = players;
    players = player;
    SDL_UnlockAudio();
    KNI_ReturnInt((int)player);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nMidiRealize()
{
    struct MidiPlayer *player;
    jint id;
    unsigned int size;
    unsigned char *data;
    int ret;

    id = KNI_GetParameterAsInt(1);
    if (id == 0) {
        KNI_ReturnInt(-1);
    }
    ret = -2;
    KNI_StartHandles(1);
        KNI_DeclareHandle(buf);
        KNI_GetParameterAsObject(2, buf);
        size = (unsigned int)KNI_GetArrayLength(buf);
        data = (unsigned char *)malloc(size);
        if (data != NULL) {
            KNI_GetRawArrayRegion(buf, 0, (jsize)size, (jbyte *)data);
            player = (struct MidiPlayer *)id;
            ret = render_midi(player, data, size);
            free(data);
        }
    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nWavRealize()
{
    struct MidiPlayer *player;
    jint id;
    unsigned int size;
    unsigned char *data;
    int ret;

    id = KNI_GetParameterAsInt(1);
    if (id == 0) {
        KNI_ReturnInt(-1);
    }
    ret = -2;
    KNI_StartHandles(1);
        KNI_DeclareHandle(buf);
        KNI_GetParameterAsObject(2, buf);
        size = (unsigned int)KNI_GetArrayLength(buf);
        data = (unsigned char *)malloc(size);
        if (data != NULL) {
            KNI_GetRawArrayRegion(buf, 0, (jsize)size, (jbyte *)data);
            player = (struct MidiPlayer *)id;
            ret = render_wav(player, data, size);
            free(data);
        }
    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nPlayTone()
{
    struct MidiPlayer *player;
    jint note;
    jint duration;
    jint volume;
    int ret;

    note = KNI_GetParameterAsInt(1);
    duration = KNI_GetParameterAsInt(2);
    volume = KNI_GetParameterAsInt(3);
    ret = ensure_audio();
    if (ret != 0) {
        KNI_ReturnInt(ret);
    }
    player = (struct MidiPlayer *)calloc(1, sizeof(struct MidiPlayer));
    if (player == NULL) {
        KNI_ReturnInt(-10);
    }
    player->volume = 100;
    player->auto_close = 1;
    ret = render_tone(player, note, duration, volume);
    if (ret != 0) {
        free(player);
        KNI_ReturnInt(ret);
    }
    SDL_LockAudio();
    player->next = players;
    players = player;
    player->loop_remaining = 1;
    player->playing = 1;
    SDL_UnlockAudio();
    printf("phoneME audio native: playTone note=%d duration=%d volume=%d ret=0\n",
           note, duration, volume);
    fflush(stdout);
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nMidiStart()
{
    struct MidiPlayer *player;
    jint id;
    jint loop_count;

    id = KNI_GetParameterAsInt(1);
    loop_count = KNI_GetParameterAsInt(2);
    if (id == 0) {
        KNI_ReturnInt(-1);
    }
    player = (struct MidiPlayer *)id;
    if (player->pcm == NULL || player->frames == 0) {
        KNI_ReturnInt(-2);
    }
    SDL_LockAudio();
    if (player->playing && loop_count == 1 &&
            player->frames <= MIDI_ONESHOT_MAX_FRAMES) {
        struct MidiPlayer *clone;
        clone = clone_oneshot_player(player);
        if (clone != NULL) {
            clone->next = players;
            players = clone;
            SDL_UnlockAudio();
            printf("phoneME audio native: midi oneshot clone id=%d frames=%u\n",
                   id, player->frames);
            fflush(stdout);
            KNI_ReturnInt(0);
        }
    }
    player->pos = 0;
    player->loop_remaining = loop_count;
    player->finished = 0;
    player->stop_fade_remaining = 0;
    player->stop_fade_total = 0;
    player->playing = 1;
    SDL_UnlockAudio();
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_Manager_nMidiSetVolume()
{
    struct MidiPlayer *player;
    jint id;
    jint volume;

    id = KNI_GetParameterAsInt(1);
    volume = KNI_GetParameterAsInt(2);
    if (id != 0) {
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        player = (struct MidiPlayer *)id;
        SDL_LockAudio();
        player->volume = volume;
        SDL_UnlockAudio();
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_Manager_nMidiStop()
{
    struct MidiPlayer *player;
    jint id;

    id = KNI_GetParameterAsInt(1);
    if (id != 0) {
        player = (struct MidiPlayer *)id;
        SDL_LockAudio();
        if (player->playing && player->pcm != NULL && player->frames > 0) {
            player->stop_fade_total = MIDI_STOP_FADE_FRAMES;
            if (player->stop_fade_total > player->frames - player->pos) {
                player->stop_fade_total = player->frames - player->pos;
            }
            if (player->stop_fade_total == 0) {
                player->playing = 0;
                player->finished = 1;
            } else {
                player->stop_fade_remaining = player->stop_fade_total;
            }
        } else {
            player->playing = 0;
            player->finished = 1;
        }
        SDL_UnlockAudio();
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_Manager_nMidiDeallocate()
{
    struct MidiPlayer *player;
    jint id;

    id = KNI_GetParameterAsInt(1);
    if (id != 0) {
        player = (struct MidiPlayer *)id;
        SDL_LockAudio();
        player->playing = 0;
        player->pos = 0;
        player->stop_fade_remaining = 0;
        player->stop_fade_total = 0;
        SDL_UnlockAudio();
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_Manager_nMidiClose()
{
    struct MidiPlayer *player;
    struct MidiPlayer **cursor;
    jint id;

    id = KNI_GetParameterAsInt(1);
    if (id != 0) {
        player = (struct MidiPlayer *)id;
        SDL_LockAudio();
        cursor = &players;
        while (*cursor != NULL) {
            if (*cursor == player) {
                *cursor = player->next;
                break;
            }
            cursor = &((*cursor)->next);
        }
        SDL_UnlockAudio();
        if (player->pcm != NULL) {
            free(player->pcm);
        }
        free(player);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_Manager_nMidiGetMediaTime()
{
    struct MidiPlayer *player;
    jint id;
    long ret;

    id = KNI_GetParameterAsInt(1);
    if (id == 0) {
        KNI_ReturnLong(0);
    }
    player = (struct MidiPlayer *)id;
    ret = (long)(((unsigned long long)player->pos * 1000ULL) / AUDIO_RATE);
    KNI_ReturnLong(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nMidiIsFinished()
{
    struct MidiPlayer *player;
    jint id;

    id = KNI_GetParameterAsInt(1);
    if (id == 0) {
        KNI_ReturnInt(1);
    }
    player = (struct MidiPlayer *)id;
    KNI_ReturnInt(player->finished);
}
