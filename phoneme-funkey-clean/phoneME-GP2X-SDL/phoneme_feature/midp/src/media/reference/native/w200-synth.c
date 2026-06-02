#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w200-synth.h"

#define W200_ENV_MAX 65536
#define W200_ATTACK_FRAMES 220
#define W200_DECAY_FRAMES 3307
#define W200_SUSTAIN_LEVEL 30000
#define W200_RELEASE_FRAMES (W200_SYNTH_OUTPUT_RATE * W200_SYNTH_RELEASE_MS / 1000)
#define W200_MASTER_GAIN 1
#define W200_PITCH_A4_NOTE 69
#define W200_PITCH_A4_VALUE 1152
#define W200_PITCH_BEND_RANGE 2
#define W200_PITCH_PHASE_NUM 1476395008LL
#define W200_PITCH_PHASE_DEN 19845LL

enum W200AudioSignal {
    W200_SIG_MIDI_OPEN = 0x5e44,
    W200_SIG_MIDI_CLOSE = 0x5e45,
    W200_SIG_MIDI_PROGRAM = 0x5e46,
    W200_SIG_MIDI_NOTE_ON_A = 0x5e48,
    W200_SIG_MIDI_NOTE_ON_B = 0x5e49,
    W200_SIG_MIDI_NOTE_OFF = 0x5e4a,
    W200_SIG_MIDI_CONTROL_A = 0x5e4b,
    W200_SIG_MIDI_CONTROL_B = 0x5e4c,
    W200_SIG_MIDI_STREAM_A = 0x5e4d,
    W200_SIG_MIDI_STREAM_B = 0x5e4e,
    W200_SIG_MIDI_STREAM_C = 0x5e4f,
    W200_SIG_MIDI_STREAM_D = 0x5e50,
    W200_SIG_MIDI_STREAM_E = 0x5e51,
    W200_SIG_MIDI_STREAM_F = 0x5e52,
    W200_SIG_MIDI_FLUSH = 0x5e53,
    W200_SIG_MIDI_STATUS = 0x5e54,
    W200_SIG_MIDI_STOP = 0x5e55,
    W200_SIG_MIDI_PAUSE = 0x5e56,
    W200_SIG_AUDIO_QUERY = 0x5e58,
    W200_SIG_MIDI_VOLUME = 0x5e59,
    W200_SIG_MIDI_TEMPO = 0x5e5a
};

struct W200Channel {
    unsigned char program;
    unsigned char bank;
    unsigned char volume;
    unsigned char expression;
    unsigned char pan;
    unsigned char sustain;
    int pitch_bend;
};

struct W200Voice {
    unsigned char active;
    unsigned char released;
    unsigned char sustained;
    unsigned char channel;
    unsigned char note;
    unsigned char velocity;
    unsigned char program;
    unsigned int phase;
    unsigned int phase2;
    unsigned int phase_inc;
    unsigned int phase_inc2;
    unsigned int env;
    unsigned int release_step;
    unsigned int age;
    unsigned int noise;
    int filter_state;
};

struct W200Synth {
    struct W200Channel channel[16];
    struct W200Voice voice[W200_SYNTH_MAX_VOICES];
    unsigned int age;
};

static const unsigned short w200_dsp_pitch_table[128] = {
    22, 23, 24, 26, 27, 29, 31, 32,
    34, 36, 39, 41, 43, 46, 49, 51,
    54, 58, 61, 65, 69, 73, 77, 82,
    86, 91, 97, 103, 109, 115, 122, 129,
    137, 145, 154, 163, 172, 183, 193, 205,
    217, 230, 243, 258, 273, 289, 306, 325,
    344, 364, 386, 409, 433, 458, 486, 514,
    545, 577, 611, 648, 686, 727, 770, 815,
    864, 915, 969, 1026, 1087, 1152, 1220, 1292,
    1369, 1450, 1536, 1627, 1723, 1825, 1933, 2048,
    2169, 2298, 2434, 2578, 2731, 2893, 3064, 3246,
    3438, 3642, 3858, 4086, 4328, 4585, 4857, 5144,
    5449, 5772, 6114, 6476, 6860, 7267, 7697, 8153,
    8636, 9148, 9690, 10264, 10873, 11517, 12199, 12922,
    13688, 14499, 15358, 16268, 17232, 18253, 19334, 20480,
    21694, 22979, 24341, 25783, 27311, 28929, 30643, 32459
};

static short clip_sample(int value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (short)value;
}

static unsigned int note_increment(int note, int pitch_bend)
{
    int target_note;
    int bend_den;
    int base_pitch;
    int target_pitch;
    int bent_pitch;
    long long phase_inc;

    if (note < 0) note = 0;
    if (note > 127) note = 127;
    if (pitch_bend < -8192) pitch_bend = -8192;
    if (pitch_bend > 8191) pitch_bend = 8191;

    base_pitch = w200_dsp_pitch_table[note];
    bent_pitch = base_pitch;
    if (pitch_bend != 0) {
        target_note = note;
        if (pitch_bend < 0) {
            target_note -= W200_PITCH_BEND_RANGE;
            if (target_note < 0) target_note = 0;
            target_pitch = w200_dsp_pitch_table[target_note];
            bend_den = 8192;
            bent_pitch = base_pitch -
                    ((base_pitch - target_pitch) * -pitch_bend) / bend_den;
        } else {
            target_note += W200_PITCH_BEND_RANGE;
            if (target_note > 127) target_note = 127;
            target_pitch = w200_dsp_pitch_table[target_note];
            bend_den = 8191;
            bent_pitch = base_pitch +
                    ((target_pitch - base_pitch) * pitch_bend) / bend_den;
        }
    }
    if (bent_pitch < 1) {
        bent_pitch = 1;
    }
    phase_inc = ((long long)bent_pitch * W200_PITCH_PHASE_NUM +
            W200_PITCH_PHASE_DEN / 2) / W200_PITCH_PHASE_DEN;
    if (phase_inc < 1) {
        phase_inc = 1;
    }
    if (phase_inc > 0xffffffffLL) {
        phase_inc = 0xffffffffLL;
    }
    return (unsigned int)phase_inc;
}

static unsigned int next_noise(struct W200Voice *voice)
{
    voice->noise = voice->noise * 1103515245U + 12345U;
    return voice->noise;
}

static unsigned int detuned_increment(unsigned int phase_inc,
                                      unsigned char program,
                                      unsigned int age)
{
    int detune;

    detune = 0;
    if (program >= 16 && program < 24) detune = 5;
    else if (program >= 40 && program < 56) detune = 9;
    else if (program >= 88 && program < 104) detune = 12;
    else if (program >= 24 && program < 32) detune = 3;
    else if (program >= 56 && program < 64) detune = -4;
    if (detune == 0) {
        return phase_inc;
    }
    if (age & 1U) {
        detune = -detune;
    }
    return (unsigned int)((long long)phase_inc *
            (long long)(1024 + detune) / 1024LL);
}

static int triangle_sample(unsigned int phase)
{
    unsigned int p;
    p = phase >> 16;
    if (p < 32768U) {
        return (int)p * 2 - 32768;
    }
    return 98304 - (int)p * 2;
}

static int sineish_sample(unsigned int phase)
{
    int tri;
    int shaped;
    int sign;

    tri = triangle_sample(phase);
    sign = tri < 0 ? -1 : 1;
    if (tri < 0) {
        tri = -tri;
    }
    shaped = (tri * (65536 - tri)) >> 15;
    return sign * shaped;
}

static int saw_sample(unsigned int phase)
{
    return (int)(phase >> 16) - 32768;
}

static int square_sample(unsigned int phase, unsigned int duty)
{
    return (phase >> 16) < duty ? 26000 : -26000;
}

static int program_filter_shift(unsigned char program)
{
    if (program == 128) return 1;
    if (program < 8) return 2;
    if (program >= 16 && program < 24) return 3;
    if (program >= 24 && program < 40) return 2;
    if (program >= 40 && program < 56) return 4;
    if (program >= 56 && program < 72) return 2;
    if (program >= 80 && program < 104) return 1;
    return 3;
}

static int filter_voice(struct W200Voice *voice, int sample)
{
    int shift;

    shift = program_filter_shift(voice->program);
    voice->filter_state += (sample - voice->filter_state) >> shift;
    return voice->filter_state;
}

static int voice_output_level(struct W200Voice *voice)
{
    unsigned char program;

    program = voice->program;
    if (program == 128) return 78;
    if (program < 8) return 88;
    if (program >= 16 && program < 24) return 80;
    if (program >= 24 && program < 40) return 82;
    if (program >= 40 && program < 56) return 92;
    if (program >= 56 && program < 72) return 74;
    if (program >= 80 && program < 104) return 68;
    return 84;
}

static unsigned int voice_attack_frames(struct W200Voice *voice)
{
    unsigned char program;

    program = voice->program;
    if (program == 128) return 16;
    if (program < 8) return 50;
    if (program >= 24 && program < 40) return 35;
    if (program >= 40 && program < 56) return 520;
    if (program >= 88 && program < 104) return 720;
    if (program >= 16 && program < 24) return 80;
    return W200_ATTACK_FRAMES;
}

static unsigned int voice_decay_frames(struct W200Voice *voice)
{
    unsigned char program;

    program = voice->program;
    if (program == 128) return 1102;
    if (program < 8) return 2205;
    if (program >= 24 && program < 40) return 1800;
    if (program >= 40 && program < 56) return 5000;
    if (program >= 88 && program < 104) return 7000;
    return W200_DECAY_FRAMES;
}

static unsigned int voice_sustain_level(struct W200Voice *voice)
{
    unsigned char program;

    program = voice->program;
    if (program == 128) return 0;
    if (program < 8) return 18000;
    if (program >= 24 && program < 40) return 22000;
    if (program >= 40 && program < 56) return 46000;
    if (program >= 56 && program < 72) return 36000;
    if (program >= 88 && program < 104) return 50000;
    return W200_SUSTAIN_LEVEL;
}

static unsigned int voice_release_frames(struct W200Voice *voice)
{
    unsigned char program;

    program = voice->program;
    if (program == 128) return 1102;
    if (program < 8) return 6000;
    if (program >= 24 && program < 40) return 4500;
    if (program >= 40 && program < 56) return 12000;
    if (program >= 88 && program < 104) return 16000;
    return W200_RELEASE_FRAMES;
}

static int melodic_wave(struct W200Voice *voice)
{
    unsigned char program;
    int sine;
    int tri;
    int saw;
    int sq;
    int organ;

    program = voice->program;
    sine = sineish_sample(voice->phase);
    tri = triangle_sample(voice->phase);
    saw = saw_sample(voice->phase) / 2;
    sq = square_sample(voice->phase, 32768U) / 3;
    organ = (sine + sineish_sample(voice->phase2 + 0x40000000U) / 2) / 2;

    if (program < 8) {
        return (sine * 5 + tri * 2 + saw) / 8;
    }
    if (program >= 8 && program < 16) {
        return (sine * 3 + tri * 2) / 5;
    }
    if (program >= 16 && program < 24) {
        return (organ * 3 + sineish_sample(voice->phase2) / 3) / 4;
    }
    if (program >= 24 && program < 32) {
        return (tri * 4 + saw + sineish_sample(voice->phase2) / 2 + sq) / 6;
    }
    if (program >= 32 && program < 40) {
        return (saw + sine * 3 + sineish_sample(voice->phase2)) / 5;
    }
    if (program >= 40 && program < 56) {
        return (sine * 3 + tri * 2 + sineish_sample(voice->phase2) * 2) / 7;
    }
    if (program >= 56 && program < 64) {
        return (saw * 2 + sine * 3 + square_sample(voice->phase, 22000U) / 5) / 5;
    }
    if (program >= 64 && program < 80) {
        return (sine * 4 + saw) / 5;
    }
    if (program >= 80 && program < 88) {
        return (sq * 2 + tri + saw) / 4;
    }
    if (program >= 88 && program < 96) {
        return (sine * 4 + sineish_sample(voice->phase2) * 2 + tri) / 7;
    }
    if (program >= 96 && program < 104) {
        return (saw + sineish_sample(voice->phase2 + 0x15555555U)) / 2;
    }
    return (sine * 3 + tri + saw) / 5;
}

static int drum_wave(struct W200Voice *voice)
{
    int noise;
    int tonal;

    noise = ((int)((next_noise(voice) >> 16) & 0xffffU) - 32768) / 2;
    tonal = sineish_sample(voice->phase);
    switch (voice->note) {
    case 35:
    case 36:
        return (tonal * 3 + noise / 3) / 4;
    case 38:
    case 40:
        return (noise * 2 + tonal) / 3;
    case 42:
    case 44:
    case 46:
        return noise / 3;
    case 49:
    case 57:
        return (noise * 2 + square_sample(voice->phase, 12000U) / 5) / 3;
    default:
        return (noise + tonal) / 3;
    }
}

static void release_voice(struct W200Voice *voice)
{
    unsigned int frames;

    if (voice->active == 0 || voice->released != 0) {
        return;
    }
    voice->released = 1;
    voice->sustained = 0;
    frames = voice_release_frames(voice);
    voice->release_step = voice->env / (frames > 0 ? frames : 1);
    if (voice->release_step == 0) {
        voice->release_step = 1;
    }
}

static void release_drum_voice(struct W200Voice *voice)
{
    unsigned int frames;

    if (voice->active == 0 || voice->released != 0) {
        return;
    }
    frames = 2205;
    if (voice->note == 35 || voice->note == 36) {
        frames = 4410;
    } else if (voice->note == 42 || voice->note == 44 || voice->note == 46) {
        frames = 735;
    }
    voice->released = 1;
    voice->release_step = voice->env / frames;
    if (voice->release_step == 0) {
        voice->release_step = 1;
    }
}

static void note_off(struct W200Synth *synth, int channel, int note)
{
    int i;

    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (synth->voice[i].active && synth->voice[i].channel == channel &&
                synth->voice[i].note == note) {
            if (synth->channel[channel].sustain) {
                synth->voice[i].sustained = 1;
            } else {
                release_voice(&synth->voice[i]);
            }
        }
    }
}

static struct W200Voice *alloc_voice(struct W200Synth *synth)
{
    struct W200Voice *best;
    unsigned int best_age;
    int i;

    best = NULL;
    best_age = 0xffffffffU;
    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (!synth->voice[i].active) {
            return &synth->voice[i];
        }
        if (synth->voice[i].released && synth->voice[i].age < best_age) {
            best = &synth->voice[i];
            best_age = synth->voice[i].age;
        }
    }
    if (best != NULL) {
        return best;
    }
    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (synth->voice[i].age < best_age) {
            best = &synth->voice[i];
            best_age = synth->voice[i].age;
        }
    }
    return best;
}

static void note_on(struct W200Synth *synth, int channel, int note, int velocity)
{
    struct W200Voice *voice;
    int drum_note;

    if (velocity <= 0) {
        note_off(synth, channel, note);
        return;
    }
    voice = alloc_voice(synth);
    if (voice == NULL) {
        return;
    }
    memset(voice, 0, sizeof(*voice));
    voice->active = 1;
    voice->channel = (unsigned char)channel;
    voice->note = (unsigned char)note;
    voice->velocity = (unsigned char)velocity;
    voice->program = synth->channel[channel].program;
    voice->age = ++synth->age;
    voice->noise = 0x1234abcdU ^ ((unsigned int)channel << 24) ^
            ((unsigned int)note << 8) ^ (unsigned int)velocity;

    drum_note = note;
    if (channel == 9) {
        if (drum_note == 35 || drum_note == 36) note = 36;
        else if (drum_note == 38 || drum_note == 40) note = 60;
        else if (drum_note == 42 || drum_note == 44 || drum_note == 46) note = 84;
        else note = 64;
        voice->program = 128;
    }
    voice->phase_inc = note_increment(note, synth->channel[channel].pitch_bend);
    voice->phase = (voice->noise & 0xffffU) << 16;
    voice->phase2 = voice->phase ^ 0x40000000U ^ (voice->age << 17);
    voice->phase_inc2 = detuned_increment(voice->phase_inc, voice->program,
                                          voice->age);
}

static void all_notes(struct W200Synth *synth, int channel, int immediate)
{
    int i;

    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (synth->voice[i].active && synth->voice[i].channel == channel) {
            if (immediate) {
                synth->voice[i].active = 0;
            } else {
                release_voice(&synth->voice[i]);
            }
        }
    }
}

static void update_channel_pitch(struct W200Synth *synth, int channel)
{
    int i;

    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (synth->voice[i].active && synth->voice[i].channel == channel &&
                synth->voice[i].program != 128) {
            synth->voice[i].phase_inc = note_increment(synth->voice[i].note,
                    synth->channel[channel].pitch_bend);
            synth->voice[i].phase_inc2 = detuned_increment(
                    synth->voice[i].phase_inc, synth->voice[i].program,
                    synth->voice[i].age);
        }
    }
}

struct W200Synth *w200_synth_open(void)
{
    struct W200Synth *synth;
    int channel;

    synth = (struct W200Synth *)calloc(1, sizeof(struct W200Synth));
    if (synth == NULL) {
        return NULL;
    }
    for (channel = 0; channel < 16; channel++) {
        synth->channel[channel].volume = 100;
        synth->channel[channel].expression = 127;
        synth->channel[channel].pan = 64;
        synth->channel[channel].pitch_bend = 0;
    }
    printf("phoneME audio native: w200 native synth ready signal=0x%x voices=%d\n",
           W200_SIG_MIDI_OPEN, W200_SYNTH_MAX_VOICES);
    return synth;
}

void w200_synth_close(struct W200Synth *synth)
{
    if (synth == NULL) {
        return;
    }
    free(synth);
}

void w200_synth_apply_midi(struct W200Synth *synth, tml_message *message)
{
    int channel;
    int value;

    if (synth == NULL || message == NULL) {
        return;
    }
    channel = message->channel & 0x0f;
    switch (message->type) {
    case TML_PROGRAM_CHANGE:
        synth->channel[channel].program = (unsigned char)(message->program & 0x7f);
        break;
    case TML_NOTE_ON:
        note_on(synth, channel, message->key & 0x7f, message->velocity & 0x7f);
        break;
    case TML_NOTE_OFF:
        note_off(synth, channel, message->key & 0x7f);
        break;
    case TML_PITCH_BEND:
        synth->channel[channel].pitch_bend = message->pitch_bend - 8192;
        update_channel_pitch(synth, channel);
        break;
    case TML_CONTROL_CHANGE:
        value = message->control_value & 0x7f;
        switch (message->control & 0x7f) {
        case 0:
            synth->channel[channel].bank = (unsigned char)value;
            break;
        case 7:
            synth->channel[channel].volume = (unsigned char)value;
            break;
        case 10:
            synth->channel[channel].pan = (unsigned char)value;
            break;
        case 11:
            synth->channel[channel].expression = (unsigned char)value;
            break;
        case 64:
            synth->channel[channel].sustain = (unsigned char)(value >= 64);
            if (value < 64) {
                int i;
                for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
                    if (synth->voice[i].active &&
                            synth->voice[i].channel == channel &&
                            synth->voice[i].sustained) {
                        release_voice(&synth->voice[i]);
                    }
                }
            }
            break;
        case 120:
            all_notes(synth, channel, 1);
            break;
        case 123:
            all_notes(synth, channel, 0);
            break;
        }
        break;
    }
}

void w200_synth_render(struct W200Synth *synth, short *pcm, int frames)
{
    int frame;
    int index;
    int sample;
    int left;
    int right;
    int volume;
    int pan;
    int env_step;
    int mixed;
    int mix_divisor;
    unsigned int attack_frames;
    unsigned int decay_frames;
    unsigned int sustain_level;
    struct W200Voice *voice;

    if (synth == NULL || pcm == NULL || frames <= 0) {
        return;
    }
    memset(pcm, 0, (size_t)frames * 2 * sizeof(short));
    for (frame = 0; frame < frames; frame++) {
        left = 0;
        right = 0;
        mixed = 0;
        for (index = 0; index < W200_SYNTH_MAX_VOICES; index++) {
            voice = &synth->voice[index];
            if (!voice->active) {
                continue;
            }
            if (voice->released) {
                if (voice->env <= voice->release_step) {
                    voice->active = 0;
                    continue;
                }
                voice->env -= voice->release_step;
            } else if (voice->env < W200_ENV_MAX) {
                attack_frames = voice_attack_frames(voice);
                if (attack_frames == 0) attack_frames = 1;
                env_step = W200_ENV_MAX / (int)attack_frames;
                if (env_step < 1) env_step = 1;
                voice->env += (unsigned int)env_step;
                if (voice->env > W200_ENV_MAX) {
                    voice->env = W200_ENV_MAX;
                }
            } else {
                sustain_level = voice_sustain_level(voice);
                if (voice->env > sustain_level) {
                    decay_frames = voice_decay_frames(voice);
                    if (decay_frames == 0) decay_frames = 1;
                    env_step = (W200_ENV_MAX - sustain_level) /
                            (int)decay_frames;
                    if (env_step < 1) env_step = 1;
                    voice->env -= (unsigned int)env_step;
                    if (voice->env < sustain_level) {
                        voice->env = sustain_level;
                    }
                }
            }

            if (voice->program == 128) {
                sample = drum_wave(voice);
                if (!voice->released && voice->env >= W200_ENV_MAX / 2) {
                    release_drum_voice(voice);
                }
            } else {
                sample = melodic_wave(voice);
            }
            sample = filter_voice(voice, sample);
            volume = (int)voice->velocity *
                    (int)synth->channel[voice->channel].volume *
                    (int)synth->channel[voice->channel].expression;
            sample = (int)(((long long)sample * (long long)voice->env *
                    (long long)volume) /
                    (127LL * 127LL * 127LL * (long long)W200_ENV_MAX));
            sample = (sample * voice_output_level(voice)) / 100;
            pan = synth->channel[voice->channel].pan;
            left += (sample * (127 - pan)) / 127;
            right += (sample * pan) / 127;
            mixed++;
            voice->phase += voice->phase_inc;
            voice->phase2 += voice->phase_inc2;
        }
        if (mixed > 8) {
            mix_divisor = 8 + ((mixed - 8) / 3);
            if (mix_divisor > 8) {
                left = (left * 8) / mix_divisor;
                right = (right * 8) / mix_divisor;
            }
        }
        pcm[frame * 2] = clip_sample(left * W200_MASTER_GAIN);
        pcm[frame * 2 + 1] = clip_sample(right * W200_MASTER_GAIN);
    }
}

int w200_synth_active_voice_count(struct W200Synth *synth)
{
    int count;
    int i;

    if (synth == NULL) {
        return 0;
    }
    count = 0;
    for (i = 0; i < W200_SYNTH_MAX_VOICES; i++) {
        if (synth->voice[i].active) {
            count++;
        }
    }
    return count;
}

const char *w200_synth_backend_name(void)
{
    return "w200-midi72-native-stage3-dsp-pitch";
}
