#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <kni.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <math.h>
#include <pthread.h>
#include "timidity/timidity.h"
#include "tinysoundfont/tsf.h"
#include "tinysoundfont/tml.h"
#include "opencore-amrnb/interf_dec.h"

/*****************************************************************************/

struct MediaSDL_Channel
{ unsigned int   Assigned;
  void         (*Callback)(int);
  void          *Data;
};
static int                      MediaSDL_NumChannel;
static struct MediaSDL_Channel *MediaSDL_Channels;
static int                      MediaSDL_AudioReady;
static int                      MediaSDL_MasterVolume = 100;
static int                      MediaSDL_AudioTrace = -1;

#define DOJA_AUDIO_RATE SAMPLE_FREQ
#define DOJA_MAX_VOICES 32
#define DOJA_PROFILE_GENERIC_DOJA 0
#define DOJA_PROFILE_NOKIA_S40 1
#define DOJA_PROFILE_SONY_ERICSSON 2
#define DOJA_PROFILE_GENERIC_GM 3

#define DOJA_WAVE_SQUARE 0
#define DOJA_WAVE_TRIANGLE 1
#define DOJA_WAVE_SAW 2
#define DOJA_WAVE_SOFT_SQUARE 3
#define DOJA_WAVE_PULSE 4
#define DOJA_WAVE_ORGAN 5
#define DOJA_WAVE_NOISE 6
#define DOJA_WAVE_SINE 7

struct DoJaEvent
{ unsigned int time_ms;
  unsigned char type;
  unsigned char channel;
  unsigned char a;
  unsigned char b;
};

struct DoJaVoice
{ int active;
  int released;
  int channel;
  int note;
  int velocity;
  int program;
  int volume;
  int pan;
  int waveform;
  int attack_ms;
  int decay_ms;
  int sustain;
  int release_ms;
  int duty;
  int brightness;
  unsigned int age_samples;
  unsigned int release_samples;
  int release_level;
  double phase;
  double step;
};

struct DoJaInstrument
{ int waveform;
  int attack_ms;
  int decay_ms;
  int sustain;
  int release_ms;
  int duty;
  int brightness;
  int gain;
};

struct DoJaAudioPlayer
{ struct DoJaEvent *events;
  int event_count;
  int event_pos;
  unsigned int sample_pos;
  int active;
  int loop_count;
  int loops_done;
  int section_loops_done;
  int volume;
  int profile;
  int loop_event_pos;
  unsigned int loop_sample_pos;
  int channel_program[16];
  int channel_volume[16];
  int channel_pan[16];
  int channel_pitch[16];
  int channel_pitch_range[16];
  int channel_modulation[16];
  struct DoJaVoice voices[DOJA_MAX_VOICES];
  struct DoJaAudioPlayer *next;
};

static struct DoJaAudioPlayer *DoJaAudioPlayers;

#define SAMPLE_FREQ	22050
#define SAMPLE_FORMAT_WAV 1
#define SAMPLE_FORMAT_AMR 2
#define AMRNB_FREQ 8000
#define AMRNB_SAMPLES_PER_FRAME 160
#define AMRNB_OUTPUT_SAMPLES_PER_FRAME 441

long long GetTimeMillis();

static int MediaSDL_AudioTraceEnabled()
{
  if (MediaSDL_AudioTrace < 0) {
    MediaSDL_AudioTrace = getenv("PHONEME_AUDIO_TRACE") != NULL;
  }
  return MediaSDL_AudioTrace;
}

#define MEDIA_TRACE(...) \
  do { if (MediaSDL_AudioTraceEnabled()) printf(__VA_ARGS__); } while (0)

static void MediaSDL_LockAudio()
{
  if (MediaSDL_AudioReady) SDL_LockAudio();
}

static void MediaSDL_UnlockAudio()
{
  if (MediaSDL_AudioReady) SDL_UnlockAudio();
}

static int MediaSDL_MixerVolume()
{
  int volume = MediaSDL_MasterVolume;
  if (volume < 0) volume = 0;
  if (volume > 100) volume = 100;
  return (MIX_MAX_VOLUME * volume) / 100;
}

void MediaSDL_SetMasterVolume(int volume)
{
  if (volume < 0) volume = 0;
  if (volume > 100) volume = 100;
  MediaSDL_MasterVolume = volume;
  if (MediaSDL_AudioReady) {
    Mix_Volume(-1, MediaSDL_MixerVolume());
    Mix_VolumeMusic(MediaSDL_MixerVolume());
  }
}

int MediaSDL_GetMasterVolume()
{
  return MediaSDL_MasterVolume;
}

void AudioSubsystemCallback(int chan)
{ if ((chan>=0)&&(chan<MediaSDL_NumChannel))
     if (MediaSDL_Channels[chan].Assigned != 0)
        if (MediaSDL_Channels[chan].Callback != NULL)
           MediaSDL_Channels[chan].Callback(chan);
}

static unsigned int DoJaRead32(const unsigned char *p)
{ return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
         ((unsigned int)p[2] << 8) | (unsigned int)p[3];
}

static int DoJaRead16(const unsigned char *p)
{ return ((int)p[0] << 8) | (int)p[1];
}

static double DoJaMidiFreq(int note)
{ double freq = 440.0;
  int diff = note - 69;
  while (diff > 0) { freq *= 1.0594630943592953; diff--; }
  while (diff < 0) { freq /= 1.0594630943592953; diff++; }
  return freq;
}

static struct DoJaVoice *DoJaAllocVoice(struct DoJaAudioPlayer *p)
{ int i;
  for (i = 0; i < DOJA_MAX_VOICES; i++)
     if (!p->voices[i].active) return &p->voices[i];
  return &p->voices[0];
}

static int DoJaEnvelopeLevel(struct DoJaVoice *v);

static void DoJaSelectInstrument(int profile, int program, int channel,
                                 struct DoJaInstrument *inst)
{ int family = program & 15;
  inst->waveform = DOJA_WAVE_SOFT_SQUARE;
  inst->attack_ms = 3;
  inst->decay_ms = 70;
  inst->sustain = 620;
  inst->release_ms = 70;
  inst->duty = 50;
  inst->brightness = 70;
  inst->gain = 72;

  if (profile == DOJA_PROFILE_GENERIC_GM)
     { if (program < 8)
          { inst->waveform = DOJA_WAVE_SAW;
            inst->attack_ms = 8;
            inst->decay_ms = 90;
            inst->sustain = 680;
            inst->release_ms = 120;
            inst->brightness = 55;
          }
       else if (program < 16)
          { inst->waveform = DOJA_WAVE_TRIANGLE;
            inst->attack_ms = 4;
            inst->decay_ms = 120;
            inst->sustain = 760;
            inst->release_ms = 140;
            inst->brightness = 45;
          }
       else if (program < 32)
          { inst->waveform = DOJA_WAVE_ORGAN;
            inst->attack_ms = 2;
            inst->decay_ms = 30;
            inst->sustain = 850;
            inst->release_ms = 90;
            inst->brightness = 50;
          }
       else
          { inst->waveform = DOJA_WAVE_SOFT_SQUARE;
            inst->brightness = 58;
          }
     }
  else if (profile == DOJA_PROFILE_NOKIA_S40)
     { inst->waveform = (family & 1) ? DOJA_WAVE_SQUARE : DOJA_WAVE_TRIANGLE;
       inst->attack_ms = 1;
       inst->decay_ms = 45;
       inst->sustain = 560;
       inst->release_ms = 45;
       inst->brightness = 82;
       inst->gain = 66;
     }
  else if (profile == DOJA_PROFILE_SONY_ERICSSON)
     { inst->waveform = (family < 4) ? DOJA_WAVE_SAW :
                        (family < 8) ? DOJA_WAVE_SOFT_SQUARE :
                        (family < 12) ? DOJA_WAVE_TRIANGLE : DOJA_WAVE_ORGAN;
       inst->attack_ms = 5;
       inst->decay_ms = 80;
       inst->sustain = 700;
       inst->release_ms = 95;
       inst->brightness = 62;
       inst->gain = 70;
     }
  else
     { switch (family)
          { case 0:
            case 1:
               inst->waveform = DOJA_WAVE_SOFT_SQUARE;
               inst->duty = 44;
               inst->brightness = 58;
               break;
            case 2:
            case 3:
               inst->waveform = DOJA_WAVE_TRIANGLE;
               inst->attack_ms = 4;
               inst->decay_ms = 100;
               inst->sustain = 720;
               inst->release_ms = 120;
               inst->brightness = 40;
               break;
            case 4:
            case 5:
               inst->waveform = DOJA_WAVE_SAW;
               inst->attack_ms = 2;
               inst->decay_ms = 65;
               inst->sustain = 610;
               inst->release_ms = 80;
               inst->brightness = 54;
               break;
            case 6:
            case 7:
               inst->waveform = DOJA_WAVE_ORGAN;
               inst->attack_ms = 2;
               inst->decay_ms = 40;
               inst->sustain = 830;
               inst->release_ms = 70;
               inst->brightness = 48;
               break;
            case 8:
            case 9:
               inst->waveform = DOJA_WAVE_PULSE;
               inst->duty = 25;
               inst->attack_ms = 1;
               inst->decay_ms = 35;
               inst->sustain = 500;
               inst->release_ms = 55;
               inst->brightness = 75;
               break;
            case 10:
            case 11:
               inst->waveform = DOJA_WAVE_SINE;
               inst->attack_ms = 8;
               inst->decay_ms = 110;
               inst->sustain = 760;
               inst->release_ms = 160;
               inst->brightness = 35;
               break;
            case 12:
            case 13:
               inst->waveform = DOJA_WAVE_NOISE;
               inst->attack_ms = 1;
               inst->decay_ms = 28;
               inst->sustain = 180;
               inst->release_ms = 25;
               inst->brightness = 95;
               inst->gain = 52;
               break;
            default:
               inst->waveform = DOJA_WAVE_SQUARE;
               inst->duty = 50;
               inst->brightness = 70;
               break;
          }
     }

  if (channel == 9)
     { inst->waveform = DOJA_WAVE_NOISE;
       inst->attack_ms = 1;
       inst->decay_ms = 18;
       inst->sustain = 120;
       inst->release_ms = 25;
       inst->brightness = 100;
       inst->gain = 60;
     }
}

static void DoJaNoteOff(struct DoJaAudioPlayer *p, int channel, int note)
{ int i;
  for (i = 0; i < DOJA_MAX_VOICES; i++)
     if (p->voices[i].active && p->voices[i].channel == channel &&
         p->voices[i].note == note)
        { p->voices[i].released = 1;
          p->voices[i].release_samples = 0;
          p->voices[i].release_level = DoJaEnvelopeLevel(&p->voices[i]);
        }
}

static void DoJaNoteOn(struct DoJaAudioPlayer *p, int channel, int note, int velocity)
{ struct DoJaVoice *v;
  struct DoJaInstrument inst;
  if (velocity <= 0) { DoJaNoteOff(p, channel, note); return; }
  DoJaSelectInstrument(p->profile, p->channel_program[channel & 15], channel, &inst);
  v = DoJaAllocVoice(p);
  v->active = 1;
  v->released = 0;
  v->channel = channel;
  v->note = note;
  v->velocity = velocity;
  v->program = p->channel_program[channel & 15];
  v->volume = p->channel_volume[channel & 15];
  v->pan = p->channel_pan[channel & 15];
  v->waveform = inst.waveform;
  v->attack_ms = inst.attack_ms;
  v->decay_ms = inst.decay_ms;
  v->sustain = inst.sustain;
  v->release_ms = inst.release_ms;
  v->duty = inst.duty;
  v->brightness = inst.brightness;
  v->age_samples = 0;
  v->release_samples = 0;
  v->release_level = 1024;
  v->velocity = velocity * inst.gain / 64;
  if (v->velocity > 127) v->velocity = 127;
  v->phase = 0.0;
  v->step = DoJaMidiFreq(note) / (double)DOJA_AUDIO_RATE;
}

static void DoJaHandleEvent(struct DoJaAudioPlayer *p, struct DoJaEvent *e)
{ int ch = e->channel & 15;
  switch (e->type)
     { case 1:
          p->channel_program[ch] = e->a;
          break;
       case 2:
          p->channel_volume[ch] = e->a;
          break;
       case 3:
          p->channel_pan[ch] = e->a;
          break;
       case 4:
          DoJaNoteOn(p, ch, e->a, e->b);
          break;
       case 5:
          DoJaNoteOff(p, ch, e->a);
          break;
       case 6:
          p->loop_event_pos = p->event_pos;
          p->loop_sample_pos = p->sample_pos;
          p->section_loops_done = 0;
          break;
       case 7:
          if (p->loop_event_pos >= 0 && (e->a == 255 ||
              p->section_loops_done < e->a))
             { p->section_loops_done++;
               p->event_pos = p->loop_event_pos;
               p->sample_pos = p->loop_sample_pos;
             }
          break;
       case 8:
          if (p->loop_count < 0 || p->loops_done + 1 < p->loop_count)
             { p->loops_done++;
               p->event_pos = 0;
               p->sample_pos = 0;
               p->loop_event_pos = -1;
               p->section_loops_done = 0;
             }
          else p->active = 0;
          break;
       case 9:
          p->channel_pitch[ch] = e->a;
          break;
       case 10:
          p->channel_modulation[ch] = e->a;
          break;
       case 11:
          p->channel_pitch_range[ch] = e->a;
          break;
     }
}

static double DoJaTriangle(double phase)
{ return phase < 0.5 ? phase * 4.0 - 1.0 : 3.0 - phase * 4.0;
}

static double DoJaSineApprox(double phase)
{ double tri = DoJaTriangle(phase);
  return tri * (1.35 - 0.35 * (tri < 0.0 ? -tri : tri));
}

static double DoJaWave(struct DoJaVoice *v, int sample)
{ double phase = v->phase;
  double tri;
  double base;
  int noise;
  switch (v->waveform)
     { case DOJA_WAVE_SQUARE:
          base = phase < 0.5 ? 0.82 : -0.82;
          break;
       case DOJA_WAVE_TRIANGLE:
          base = DoJaTriangle(phase) * 0.78;
          break;
       case DOJA_WAVE_SAW:
          base = (1.0 - phase * 2.0) * 0.72;
          break;
       case DOJA_WAVE_SOFT_SQUARE:
          tri = DoJaTriangle(phase);
          base = (phase < ((double)v->duty / 100.0) ? 0.62 : -0.62) +
                 tri * (0.18 + (double)(100 - v->brightness) / 500.0);
          break;
       case DOJA_WAVE_PULSE:
          base = phase < ((double)v->duty / 100.0) ? 0.90 : -0.34;
          break;
       case DOJA_WAVE_ORGAN:
          base = DoJaSineApprox(phase) * 0.62 +
                 DoJaSineApprox(phase * 2.0 - (int)(phase * 2.0)) * 0.22 +
                 DoJaSineApprox(phase * 3.0 - (int)(phase * 3.0)) * 0.10;
          break;
       case DOJA_WAVE_NOISE:
          noise = ((sample * 1103515245 + v->program * 12345 +
                    v->note * 31337) >> 16) & 255;
          base = (noise - 128) / 150.0;
          break;
       case DOJA_WAVE_SINE:
          base = DoJaSineApprox(phase) * 0.74;
          break;
       default:
          base = phase < 0.5 ? 0.70 : -0.70;
          break;
     }
  return base * (0.55 + (double)v->brightness / 220.0);
}

static int DoJaMsToSamples(int ms)
{ if (ms <= 0) return 1;
  return (ms * DOJA_AUDIO_RATE) / 1000 + 1;
}

static int DoJaEnvelopeLevel(struct DoJaVoice *v)
{ int attack = DoJaMsToSamples(v->attack_ms);
  int decay = DoJaMsToSamples(v->decay_ms);
  int release = DoJaMsToSamples(v->release_ms);
  int level;
  int pos;
  if (v->released)
     { if ((int)v->release_samples >= release) return 0;
       return v->release_level * (release - (int)v->release_samples) / release;
     }
  if ((int)v->age_samples < attack)
     return ((int)v->age_samples * 1024) / attack;
  if ((int)v->age_samples < attack + decay)
     { pos = (int)v->age_samples - attack;
       level = 1024 - ((1024 - v->sustain) * pos) / decay;
       return level;
     }
  return v->sustain;
}

static int DoJaClamp16(int v)
{ if (v > 32767) return 32767;
  if (v < -32768) return -32768;
  return v;
}

static void DoJaAudioPostMix(void *udata, Uint8 *stream, int len)
{ int frames = len / 4;
  short *out = (short *)stream;
  int frame, i;
  struct DoJaAudioPlayer *p;
  struct DoJaVoice *v;
  unsigned int time_ms;
  int env;
  double wave;
  int amp;
  int vol;
  int pan;
  int bend;
  int range;
  int mod;
  int period;
  double step;
  double lfo;
  (void)udata;
  for (frame = 0; frame < frames; frame++)
     { int mix_l = 0;
       int mix_r = 0;
       p = DoJaAudioPlayers;
       while (p != NULL)
          { if (p->active)
               { time_ms = (p->sample_pos * 1000U) / DOJA_AUDIO_RATE;
                 while (p->event_pos < p->event_count &&
                        p->events[p->event_pos].time_ms <= time_ms)
                    { p->event_pos++;
                      DoJaHandleEvent(p, &p->events[p->event_pos - 1]);
                    }
                 for (i = 0; i < DOJA_MAX_VOICES; i++)
                    if (p->voices[i].active)
                       { v = &p->voices[i];
                         env = DoJaEnvelopeLevel(v);
                         vol = v->volume <= 0 ? 100 : v->volume;
                         pan = v->pan <= 0 ? 64 : v->pan;
                         if (env <= 0)
                            { v->active = 0;
                              continue;
                            }
                         wave = DoJaWave(v, (int)p->sample_pos);
                         amp = (int)(wave * (double)(v->velocity * 34));
                         amp = amp * env / 1024;
                         amp = amp * vol * p->volume / 10000;
                         mix_l += amp * (127 - pan) / 127;
                         mix_r += amp * pan / 127;
                         bend = p->channel_pitch[v->channel & 15] - 32;
                         range = p->channel_pitch_range[v->channel & 15];
                         mod = p->channel_modulation[v->channel & 15];
                         step = v->step * (1.0 + (double)(bend * range) / 3840.0);
                         if (mod > 0)
                            { period = DOJA_AUDIO_RATE / 5;
                              if (period <= 0) period = 1;
                              lfo = DoJaTriangle((double)(p->sample_pos % period) /
                                                  (double)period);
                              step *= 1.0 + lfo * (double)mod / 3000.0;
                            }
                         v->phase += step;
                         if (v->phase >= 1.0) v->phase -= (int)v->phase;
                         v->age_samples++;
                         if (v->released) v->release_samples++;
                       }
                 p->sample_pos++;
               }
            p = p->next;
          }
       out[frame * 2] = (short)DoJaClamp16((int)out[frame * 2] + mix_l);
       out[frame * 2 + 1] = (short)DoJaClamp16((int)out[frame * 2 + 1] + mix_r);
     }
}

int InitAudioSubsystem()
{ int chan;
  char *volumeText;
  if (MediaSDL_AudioReady) return(0);
  volumeText = getenv("PHONEME_SOUND_VOLUME");
  if (volumeText != NULL && volumeText[0] != '\0') {
    MediaSDL_SetMasterVolume(atoi(volumeText));
  }
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return(-1);
  if (Mix_OpenAudio(SAMPLE_FREQ, AUDIO_S16SYS, 2, 4096) != 0)
     return(-2);
  if (Timidity_Init() < 0) return(-3);
  MediaSDL_NumChannel = Mix_AllocateChannels(32);
  if (MediaSDL_NumChannel < 1) return(-4);
  MediaSDL_Channels = (struct MediaSDL_Channel *)malloc(sizeof(struct MediaSDL_Channel) * MediaSDL_NumChannel);
  if (MediaSDL_Channels == NULL) return(-5);
  for(chan=0; chan<MediaSDL_NumChannel; chan++)
     { MediaSDL_Channels[chan].Assigned = 0;
       MediaSDL_Channels[chan].Callback = NULL;
       MediaSDL_Channels[chan].Data = NULL;
     }
  Mix_ChannelFinished(AudioSubsystemCallback);
  Mix_SetPostMix(DoJaAudioPostMix, NULL);
  MediaSDL_AudioReady = 1;
  MediaSDL_SetMasterVolume(MediaSDL_MasterVolume);
  return(0);
}

void FinalizeAudioSubsystem()
{ if (!MediaSDL_AudioReady) return;
  Timidity_Exit();
  SDL_CloseAudio();
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
  if (MediaSDL_Channels != NULL)
     { free(MediaSDL_Channels);
       MediaSDL_Channels = NULL;
     }
  MediaSDL_NumChannel = 0;
  MediaSDL_AudioReady = 0;
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_Manager_nInitAudioSubsystem()
{ KNI_ReturnInt(InitAudioSubsystem());
}

static int ReserveChannel()
{ int chan;
  MediaSDL_LockAudio();
  for(chan=0; chan<MediaSDL_NumChannel; chan++)
     if (MediaSDL_Channels[chan].Assigned == 0)
        { MediaSDL_Channels[chan].Assigned = 1;
          MediaSDL_UnlockAudio();
          return(chan);
        }
  MediaSDL_UnlockAudio();
  return(-1);
}

static void FreeChannelUnlocked(int chan)
{ if ((chan>=0)&&(chan<MediaSDL_NumChannel))
     { MediaSDL_Channels[chan].Assigned = 0;
       MediaSDL_Channels[chan].Callback = NULL;
       MediaSDL_Channels[chan].Data = NULL;
     }
}

static void FreeChannel(int chan)
{ if ((chan>=0)&&(chan<MediaSDL_NumChannel))
     { MediaSDL_LockAudio();
       FreeChannelUnlocked(chan);
       MediaSDL_UnlockAudio();
     }
}

/*****************************************************************************/

static unsigned short MidiNotes[128] = {0x0008,0x0008,0x0009,0x0009,0x000A,0x000A,0x000B,0x000C,
                                        0x000C,0x000D,0x000E,0x000F,0x0010,0x0011,0x0012,0x0013,
                                        0x0014,0x0015,0x0017,0x0018,0x0019,0x001B,0x001D,0x001E,
                                        0x0020,0x0022,0x0024,0x0026,0x0029,0x002B,0x002E,0x0031,
                                        0x0033,0x0037,0x003A,0x003D,0x0041,0x0045,0x0049,0x004D,
                                        0x0052,0x0057,0x005C,0x0062,0x0067,0x006E,0x0074,0x007B,
                                        0x0082,0x008A,0x0092,0x009B,0x00A4,0x00AE,0x00B9,0x00C4,
                                        0x00CF,0x00DC,0x00E9,0x00F6,0x0105,0x0115,0x0125,0x0137,
                                        0x0149,0x015D,0x0172,0x0188,0x019F,0x01B8,0x01D2,0x01ED,
                                        0x020B,0x022A,0x024B,0x026E,0x0293,0x02BA,0x02E4,0x0310,
                                        0x033E,0x0370,0x03A4,0x03DB,0x0416,0x0454,0x0496,0x04DC,
                                        0x0526,0x0574,0x05C8,0x0620,0x067D,0x06E0,0x0748,0x07B7,
                                        0x082D,0x08A9,0x092D,0x09B9,0x0A4D,0x0AE9,0x0B90,0x0C40,
                                        0x0CFA,0x0DC0,0x0E91,0x0F6F,0x105A,0x1153,0x125A,0x1372,
                                        0x149A,0x15D3,0x1720,0x1880,0x19F5,0x1B80,0x1D22,0x1EDE,
                                        0x20B4,0x22A6,0x24B5,0x26E4,0x2934,0x2BA7,0x2E40,0x3100};

struct NativeTonePlayer
{ Mix_Chunk MC;
  int       Chan;
};

struct NativeTonePlayer *CreateToneChunk(int Note, int Volume)
{ struct NativeTonePlayer *Ret;
  short Value, *Buf;
  unsigned int i, m;
  if ((Note<0)||(Note>127)) return(NULL);
  Ret = (struct NativeTonePlayer *)malloc(sizeof(struct NativeTonePlayer));
  if (Ret == NULL) return(NULL);
  Ret->MC.allocated = 0;
  Ret->MC.volume = MIX_MAX_VOLUME;
  Ret->MC.alen = SAMPLE_FREQ / MidiNotes[Note];
  Ret->MC.abuf = malloc(Ret->MC.alen * 2);
  if (Ret->MC.abuf == NULL)
     { free(Ret);
       return(NULL);
     }
  Value = (Volume * 32767) / 100;
  m = (Ret->MC.alen >> 1);
  Buf = (short *)Ret->MC.abuf;
  for(i=0; i<Ret->MC.alen; i++)
     Buf[i] = (i<m) ? Value : -Value;
  Ret->MC.alen <<= 1;
  Ret->Chan = -1;
  return(Ret);
}

void FreeToneChunk(struct NativeTonePlayer *NTP)
{ if (NTP != NULL)
     { if (NTP->MC.abuf != NULL) free(NTP->MC.abuf);
       free(NTP);
     }  
}

void TonePlayerCallback(int chan)
{ struct NativeTonePlayer *NTP;
  NTP = (struct NativeTonePlayer *)MediaSDL_Channels[chan].Data;
  FreeChannelUnlocked(chan);
  FreeToneChunk(NTP);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_Manager_nPlayTone()
{ struct NativeTonePlayer *NTP;
  jint Note = KNI_GetParameterAsInt(1);
  jint Duration = KNI_GetParameterAsInt(2);
  jint Volume = KNI_GetParameterAsInt(3);
  printf("nPlayTone(%d, %d, %d)\n", Note, Duration, Volume); 
  NTP = CreateToneChunk(Note, Volume);
  if (NTP != NULL)
     { NTP->Chan = ReserveChannel();
       if (NTP->Chan == -1) FreeToneChunk(NTP);
       else { MediaSDL_Channels[NTP->Chan].Data = NTP;
              MediaSDL_Channels[NTP->Chan].Callback = TonePlayerCallback;
              Mix_Volume(NTP->Chan, MediaSDL_MixerVolume());
              Mix_PlayChannelTimed(NTP->Chan, &NTP->MC, -1, Duration);
            }
     }
  KNI_ReturnVoid();
}

/*****************************************************************************/

struct NativeSamplePlayer
{ Mix_Chunk *Chunk;
  int Chan;
  long long LastTime;
  int CheckEOM;
  int VolumeLevel;
  int Stopped;
};

static const int AmrNbFrameSizes[16] =
{ 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

static Mix_Chunk *LoadAMRNBChunk(const unsigned char *Buffer, unsigned int BufferSize)
{ void *Decoder;
  unsigned int Pos;
  unsigned int OutSize;
  unsigned int OutCapacity;
  unsigned char *OutBuffer;
  Mix_Chunk *Chunk;
  if (BufferSize < 6) return(NULL);
  if (memcmp(Buffer, "#!AMR\n", 6) != 0) return(NULL);
  Decoder = Decoder_Interface_init();
  if (Decoder == NULL) return(NULL);
  Pos = 6;
  OutSize = 0;
  OutCapacity = AMRNB_OUTPUT_SAMPLES_PER_FRAME * 2 * sizeof(short) * 16;
  OutBuffer = (unsigned char *)malloc(OutCapacity);
  if (OutBuffer == NULL)
     { Decoder_Interface_exit(Decoder);
       return(NULL);
     }
  while (Pos < BufferSize)
     { unsigned char Toc;
       int Mode;
       int FrameSize;
       short Pcm[AMRNB_SAMPLES_PER_FRAME];
       unsigned int Needed;
       short *Dst;
       int i;
       Toc = Buffer[Pos];
       Mode = (Toc >> 3) & 0x0f;
       FrameSize = AmrNbFrameSizes[Mode];
       if (FrameSize <= 0) break;
       if (Pos + 1 + FrameSize > BufferSize) break;
       Needed = OutSize + AMRNB_OUTPUT_SAMPLES_PER_FRAME * 2 * sizeof(short);
       if (Needed > OutCapacity)
          { unsigned char *NewBuffer;
            OutCapacity *= 2;
            while (Needed > OutCapacity) OutCapacity *= 2;
            NewBuffer = (unsigned char *)realloc(OutBuffer, OutCapacity);
            if (NewBuffer == NULL)
               { free(OutBuffer);
                 Decoder_Interface_exit(Decoder);
                 return(NULL);
               }
            OutBuffer = NewBuffer;
          }
       Decoder_Interface_Decode(Decoder, Buffer + Pos, Pcm, 0);
       Dst = (short *)(OutBuffer + OutSize);
       for(i=0; i<AMRNB_OUTPUT_SAMPLES_PER_FRAME; i++)
          { int SrcIndex;
            short Sample;
            SrcIndex = (i * AMRNB_FREQ) / SAMPLE_FREQ;
            if (SrcIndex >= AMRNB_SAMPLES_PER_FRAME) SrcIndex = AMRNB_SAMPLES_PER_FRAME - 1;
            Sample = Pcm[SrcIndex];
            Dst[i * 2] = Sample;
            Dst[i * 2 + 1] = Sample;
          }
       OutSize = Needed;
       Pos += 1 + FrameSize;
     }
  Decoder_Interface_exit(Decoder);
  if (OutSize == 0)
     { free(OutBuffer);
       return(NULL);
     }
  Chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
  if (Chunk == NULL)
     { free(OutBuffer);
       return(NULL);
     }
  Chunk->allocated = 1;
  Chunk->abuf = OutBuffer;
  Chunk->alen = OutSize;
  Chunk->volume = MIX_MAX_VOLUME;
  printf("AMRNB decoded bytes=%u pcmBytes=%u frames=%u\n", BufferSize, OutSize,
         OutSize / (AMRNB_OUTPUT_SAMPLES_PER_FRAME * 2 * sizeof(short)));
  return(Chunk);
}

void SamplePlayerCallback(int chan)
{ struct NativeSamplePlayer *NSP = (struct NativeSamplePlayer *)MediaSDL_Channels[chan].Data;
  if (NSP == NULL) return;
  NSP->CheckEOM = 1;
  NSP->Stopped = 1;
  FreeChannelUnlocked(chan);
  NSP->Chan = -1;
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_GenericPlayer_nSamplePlayerInit()
{ struct NativeSamplePlayer *Ret;
  Ret = malloc(sizeof(struct NativeSamplePlayer));
  if (Ret != NULL)
     { Ret->Chunk = NULL;
       Ret->Chan = -1;
       Ret->LastTime = 0;
       Ret->CheckEOM = 0;
       Ret->VolumeLevel = 100;
       Ret->Stopped = 1;
     }
  KNI_ReturnInt((int)Ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_GenericPlayer_nSamplePlayerRealize()
{ struct NativeSamplePlayer *NSP;
  SDL_RWops *RW;
  unsigned char *Buffer;
  unsigned int BufferSize;
  int ret=0;
  jint id = KNI_GetParameterAsInt(1);
  jint Format = KNI_GetParameterAsInt(3);
  KNI_StartHandles(1);
  KNI_DeclareHandle(buf);
  KNI_GetParameterAsObject(2, buf);
  NSP = (struct NativeSamplePlayer *)id;
  BufferSize = KNI_GetArrayLength(buf);
  Buffer = malloc(BufferSize);
  if (Buffer == NULL) ret = -1;
  else { KNI_GetRawArrayRegion(buf, 0, (jsize)BufferSize, (jbyte*)Buffer);
         RW = SDL_RWFromMem(Buffer, BufferSize);
         if (RW == NULL)
            { free(Buffer);
              ret = -2;
            }
         else { if (Format == SAMPLE_FORMAT_AMR)
                   { SDL_RWclose(RW);
                     NSP->Chunk = LoadAMRNBChunk(Buffer, BufferSize);
                   }
                else NSP->Chunk = Mix_LoadWAV_RW(RW, 1);
                free(Buffer);
                if (NSP->Chunk == NULL) ret = -3;
              }
       }
  KNI_EndHandles();
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_GenericPlayer_nSamplePlayerPrefetch()
{ struct NativeSamplePlayer *NSP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnInt(-1);
  NSP = (struct NativeSamplePlayer *)id;
  NSP->CheckEOM = 0;
  NSP->Stopped = 1;
  printf("Sample native prefetch id=%d chan=%d\n", id, NSP->Chan);
  KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_GenericPlayer_nSamplePlayerStart()
{ struct NativeSamplePlayer *NSP;
  jint id = KNI_GetParameterAsInt(1);
  int ret=0;
  int play_chan;
  if (id == 0) KNI_ReturnInt(-3);
  NSP = (struct NativeSamplePlayer *)id;
  if (NSP->Chunk == NULL) KNI_ReturnInt(-4);
  if (NSP->Chan < 0)
     { NSP->Chan = ReserveChannel();
       if (NSP->Chan == -1) KNI_ReturnInt(-2);
       MediaSDL_Channels[NSP->Chan].Data = NSP;
       MediaSDL_Channels[NSP->Chan].Callback = SamplePlayerCallback;
     }
  NSP->LastTime = GetTimeMillis();
  NSP->CheckEOM = 0;
  NSP->Stopped = 0;
  Mix_Volume(NSP->Chan, MediaSDL_MixerVolume());
  play_chan = Mix_PlayChannel(NSP->Chan, NSP->Chunk, 0);
  if (play_chan == -1)
     { FreeChannel(NSP->Chan);
       NSP->Chan = -1;
       NSP->Stopped = 1;
       ret = -1;
     }
  printf("Sample native start id=%d ret=%d chan=%d playChan=%d len=%u\n",
         id, ret, NSP->Chan, play_chan, NSP->Chunk->alen);
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_GenericPlayer_nSamplePlayerStop()
{ struct NativeSamplePlayer *NSP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NSP = (struct NativeSamplePlayer *)id;
  MediaSDL_LockAudio();
  NSP->Stopped = 1;
  chan = NSP->Chan;
  if (NSP->Chan != -1)
     { FreeChannelUnlocked(NSP->Chan);
       NSP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_GenericPlayer_nSamplePlayerDeallocate()
{ struct NativeSamplePlayer *NSP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NSP = (struct NativeSamplePlayer *)id;
  MediaSDL_LockAudio();
  NSP->Stopped = 1;
  chan = NSP->Chan;
  if (NSP->Chan != -1)
     { FreeChannelUnlocked(NSP->Chan);
       NSP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_GenericPlayer_nSamplePlayerClose()
{ struct NativeSamplePlayer *NSP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NSP = (struct NativeSamplePlayer *)id;
  MediaSDL_LockAudio();
  NSP->Stopped = 1;
  chan = NSP->Chan;
  if (NSP->Chan != -1)
     { FreeChannelUnlocked(NSP->Chan);
       NSP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  if (NSP->Chunk != NULL) Mix_FreeChunk(NSP->Chunk);
  free(NSP);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_GenericPlayer_nSampleGetMediaTime()
{ struct NativeSamplePlayer *NSP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnLong(0);
  NSP = (struct NativeSamplePlayer *)id;
  if (NSP->Stopped) KNI_ReturnLong(0);
  KNI_ReturnLong((long)(GetTimeMillis() - NSP->LastTime));
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_GenericPlayer_nSampleCheckEOM()
{ struct NativeSamplePlayer *NSP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnInt(1);
  NSP = (struct NativeSamplePlayer *)id;
  KNI_ReturnInt(NSP->CheckEOM);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_GenericPlayer_nSampleSetVolumeLevel()
{ struct NativeSamplePlayer *NSP;
  jint id = KNI_GetParameterAsInt(1);
  jint value = KNI_GetParameterAsInt(2);
  if (id == 0) KNI_ReturnVoid();
  NSP = (struct NativeSamplePlayer *)id;
  NSP->VolumeLevel = value;
  if (NSP->Chunk != NULL)
     Mix_VolumeChunk(NSP->Chunk, (MIX_MAX_VOLUME * value) / 100);
  KNI_ReturnVoid();
}

/*****************************************************************************/

#define MIDI_CHUNK_SIZE	16384
#define MIDI_TSF_BLOCK_SAMPLES 512
#define MIDI_TSF_MAX_MS 300000

struct NativeMIDIPlayer
{ MidiSong      *Song;
  unsigned char *RawMidi;
  unsigned int   RawMidiSize;
  Mix_Chunk      MC;
  int            Chan;
  long           MediaSample;
  long long      LastTime;
  int            CheckEOM;
  int            VolumeLevel;
  int            Stopped;
};

long long GetTimeMillis()
{ struct timeval TV;
  long long Ret;
  gettimeofday(&TV, NULL);
  Ret = (long long)TV.tv_sec * 1000;
  Ret += (long long)TV.tv_usec / 1000;
  return(Ret);
}

extern const char* midpGetHomeDir(void);

void MidiPlayerCallback(int chan)
{ struct NativeMIDIPlayer *NMP = (struct NativeMIDIPlayer *)MediaSDL_Channels[chan].Data;
  if (NMP == NULL) return;
  NMP->CheckEOM = 1;
  NMP->Stopped = 1;
  FreeChannelUnlocked(chan);
  NMP->Chan = -1;
}

static unsigned int ReadBE32(const unsigned char *p)
{ return(((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
         ((unsigned int)p[2] << 8) | (unsigned int)p[3]);
}

static unsigned int ReadBE16(const unsigned char *p)
{ return(((unsigned int)p[0] << 8) | (unsigned int)p[1]);
}

static unsigned int ReadVarLen(const unsigned char *buf, unsigned int size, unsigned int *pos)
{ unsigned int value = 0;
  unsigned char c;
  do { if (*pos >= size) return(value);
       c = buf[(*pos)++];
       value = (value << 7) | (c & 0x7f);
     } while (c & 0x80);
  return(value);
}

static int EnsureSimpleMIDICapacity(short **pcm, unsigned int *capacity, unsigned int samples)
{ short *new_pcm;
  unsigned int new_capacity;
  if (samples <= *capacity) return(0);
  new_capacity = (*capacity == 0) ? SAMPLE_FREQ : *capacity;
  while (samples > new_capacity) new_capacity *= 2;
  if (new_capacity > SAMPLE_FREQ * 180) return(-1);
  new_pcm = (short *)realloc(*pcm, new_capacity * 2 * sizeof(short));
  if (new_pcm == NULL) return(-2);
  memset(new_pcm + (*capacity * 2), 0, (new_capacity - *capacity) * 2 * sizeof(short));
  *pcm = new_pcm;
  *capacity = new_capacity;
  return(0);
}

static void MixSimpleMIDINote(short *pcm, unsigned int capacity, unsigned int start,
                              unsigned int end, int note, int velocity)
{ double freq;
  double phase;
  double step;
  int amp;
  unsigned int i;
  if (end <= start || start >= capacity) return;
  if (end > capacity) end = capacity;
  freq = 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
  step = freq / (double)SAMPLE_FREQ;
  phase = 0.0;
  amp = 900 + velocity * 36;
  for (i=start; i<end; i++)
     { int sample = (phase < 0.5) ? amp : -amp;
       int left = pcm[i * 2] + sample;
       int right = pcm[i * 2 + 1] + sample;
       if (left > 32767) left = 32767;
       if (left < -32768) left = -32768;
       if (right > 32767) right = 32767;
       if (right < -32768) right = -32768;
       pcm[i * 2] = (short)left;
       pcm[i * 2 + 1] = (short)right;
       phase += step;
       if (phase >= 1.0) phase -= 1.0;
     }
}

static int RenderSimpleMIDIPlayer(struct NativeMIDIPlayer *NMP)
{ const unsigned char *buf;
  unsigned int size;
  unsigned int pos;
  unsigned int tracks;
  unsigned int division;
  unsigned int trk;
  unsigned int max_sample;
  unsigned int capacity;
  short *pcm;
  buf = NMP->RawMidi;
  size = NMP->RawMidiSize;
  if (buf == NULL || size < 14) return(-1);
  if (memcmp(buf, "MThd", 4) != 0) return(-2);
  tracks = ReadBE16(buf + 10);
  division = ReadBE16(buf + 12);
  if (division == 0 || division > 0x7fff) division = 96;
  pos = 8 + ReadBE32(buf + 4);
  if (pos > size) return(-3);
  pcm = NULL;
  capacity = 0;
  max_sample = 0;
  if (EnsureSimpleMIDICapacity(&pcm, &capacity, SAMPLE_FREQ / 4) != 0) return(-4);
  for (trk=0; trk<tracks && pos + 8 <= size; trk++)
     { unsigned int end;
       unsigned int tick;
       unsigned int tempo;
       unsigned int status;
       unsigned int active_tick[16][128];
       unsigned char active_vel[16][128];
       if (memcmp(buf + pos, "MTrk", 4) != 0) break;
       end = pos + 8 + ReadBE32(buf + pos + 4);
       pos += 8;
       if (end > size) end = size;
       tick = 0;
       tempo = 500000;
       status = 0;
       memset(active_tick, 0, sizeof(active_tick));
       memset(active_vel, 0, sizeof(active_vel));
       while (pos < end)
          { unsigned int delta;
            unsigned int ev;
            unsigned int channel;
            unsigned int sample_pos;
            delta = ReadVarLen(buf, end, &pos);
            tick += delta;
            if (pos >= end) break;
            ev = buf[pos++];
            if (ev < 0x80)
               { if (status == 0) break;
                 pos--;
                 ev = status;
               }
            else if (ev < 0xf0) status = ev;
            if (ev == 0xff)
               { unsigned int type;
                 unsigned int len;
                 if (pos >= end) break;
                 type = buf[pos++];
                 len = ReadVarLen(buf, end, &pos);
                 if (type == 0x51 && len >= 3 && pos + 3 <= end)
                    tempo = ((unsigned int)buf[pos] << 16) | ((unsigned int)buf[pos+1] << 8) | buf[pos+2];
                 if (type == 0x2f) { pos += len; break; }
                 pos += len;
                 continue;
               }
            if (ev == 0xf0 || ev == 0xf7)
               { unsigned int len = ReadVarLen(buf, end, &pos);
                 pos += len;
                 continue;
               }
            channel = ev & 0x0f;
            sample_pos = (unsigned int)(((double)tick * (double)tempo * (double)SAMPLE_FREQ) /
                                        ((double)division * 1000000.0));
            if ((ev & 0xf0) == 0x90 || (ev & 0xf0) == 0x80)
               { unsigned int note;
                 unsigned int vel;
                 if (pos + 2 > end) break;
                 note = buf[pos++] & 0x7f;
                 vel = buf[pos++] & 0x7f;
                 if ((ev & 0xf0) == 0x90 && vel != 0)
                    { active_tick[channel][note] = sample_pos;
                      active_vel[channel][note] = (unsigned char)vel;
                    }
                 else if (active_vel[channel][note] != 0)
                    { unsigned int start = active_tick[channel][note];
                      unsigned int stop = sample_pos;
                      if (stop <= start) stop = start + SAMPLE_FREQ / 12;
                      if (EnsureSimpleMIDICapacity(&pcm, &capacity, stop + SAMPLE_FREQ / 8) == 0)
                         MixSimpleMIDINote(pcm, capacity, start, stop, note, active_vel[channel][note]);
                      if (stop > max_sample) max_sample = stop;
                      active_vel[channel][note] = 0;
                    }
               }
            else if ((ev & 0xf0) == 0xc0 || (ev & 0xf0) == 0xd0)
               { if (pos < end) pos++;
               }
            else
               { if (pos + 2 > end) break;
                 pos += 2;
               }
          }
       pos = end;
     }
  if (max_sample < SAMPLE_FREQ / 4) max_sample = SAMPLE_FREQ / 4;
  NMP->MC.abuf = (Uint8 *)pcm;
  NMP->MC.alen = max_sample * 2 * sizeof(short);
  NMP->MC.allocated = 1;
  NMP->MC.volume = (MIX_MAX_VOLUME * NMP->VolumeLevel) / 100;
  MEDIA_TRACE("MIDI simple rendered rawBytes=%u pcmBytes=%u\n", size, NMP->MC.alen);
  return(0);
}

static tsf *LoadMIDISoundFont(void)
{ char path[1024];
  const char *env;
  const char *midp_home;
  tsf *font;
  env = getenv("PHONEME_SOUNDFONT");
  if (env != NULL && *env != '\0')
     { font = tsf_load_filename(env);
       if (font != NULL)
          { MEDIA_TRACE("MIDI tsf soundfont=%s presets=%d\n", env, tsf_get_presetcount(font));
            return(font);
          }
       MEDIA_TRACE("MIDI tsf soundfont failed=%s\n", env);
     }
  font = tsf_load_filename("/mnt/sd/midi/default.sf2");
  if (font != NULL)
     { MEDIA_TRACE("MIDI tsf soundfont=/mnt/sd/midi/default.sf2 presets=%d\n", tsf_get_presetcount(font));
       return(font);
     }
  font = tsf_load_filename("/mnt/java/default.sf2");
  if (font != NULL)
     { MEDIA_TRACE("MIDI tsf soundfont=/mnt/java/default.sf2 presets=%d\n", tsf_get_presetcount(font));
       return(font);
     }
  midp_home = midpGetHomeDir();
  if (midp_home != NULL && *midp_home != '\0')
     { snprintf(path, sizeof(path), "%s/lib/soundfont/default.sf2", midp_home);
       font = tsf_load_filename(path);
       if (font != NULL)
          { MEDIA_TRACE("MIDI tsf soundfont=%s presets=%d\n", path, tsf_get_presetcount(font));
            return(font);
          }
       MEDIA_TRACE("MIDI tsf soundfont failed=%s\n", path);
     }
  return(NULL);
}

static void ApplyTSFMIDIMessage(tsf *font, tml_message *msg)
{ int channel;
  channel = msg->channel & 0x0f;
  switch(msg->type)
     { case TML_PROGRAM_CHANGE:
              tsf_channel_set_presetnumber(font, channel, msg->program & 0x7f, channel == 9);
              break;
       case TML_NOTE_ON:
              tsf_channel_note_on(font, channel, msg->key & 0x7f, (float)(msg->velocity & 0x7f) / 127.0f);
              break;
       case TML_NOTE_OFF:
              tsf_channel_note_off(font, channel, msg->key & 0x7f);
              break;
       case TML_PITCH_BEND:
              tsf_channel_set_pitchwheel(font, channel, msg->pitch_bend);
              break;
       case TML_CONTROL_CHANGE:
              tsf_channel_midi_control(font, channel, msg->control & 0x7f, msg->control_value & 0x7f);
              break;
     }
}

static int RenderTSFMIDIPlayer(struct NativeMIDIPlayer *NMP)
{ tsf *font;
  tml_message *midi;
  tml_message *msg;
  short *pcm;
  unsigned int length_ms;
  unsigned int total_samples;
  unsigned int rendered;
  double msec;
  int used_channels;
  int used_programs;
  int total_notes;
  unsigned int first_note;
  int i;
  if (NMP->RawMidi == NULL || NMP->RawMidiSize == 0) return(-1);
  midi = tml_load_memory(NMP->RawMidi, (int)NMP->RawMidiSize);
  if (midi == NULL) return(-2);
  font = LoadMIDISoundFont();
  if (font == NULL)
     { tml_free(midi);
       return(-3);
     }
  tml_get_info(midi, &used_channels, &used_programs, &total_notes, &first_note, &length_ms);
  if (length_ms < 250) length_ms = 250;
  length_ms += 2000;
  if (length_ms > MIDI_TSF_MAX_MS) length_ms = MIDI_TSF_MAX_MS;
  total_samples = (unsigned int)(((double)length_ms * (double)SAMPLE_FREQ) / 1000.0);
  pcm = (short *)calloc(total_samples * 2, sizeof(short));
  if (pcm == NULL)
     { tsf_close(font);
       tml_free(midi);
       return(-4);
     }
  tsf_set_output(font, TSF_STEREO_INTERLEAVED, SAMPLE_FREQ, -12.0f);
  tsf_set_max_voices(font, 64);
  for (i=0; i<16; i++)
     tsf_channel_set_presetnumber(font, i, 0, i == 9);
  tsf_channel_set_bank_preset(font, 9, 128, 0);
  msg = midi;
  msec = 0.0;
  rendered = 0;
  while (rendered < total_samples)
     { unsigned int block;
       block = total_samples - rendered;
       if (block > MIDI_TSF_BLOCK_SAMPLES) block = MIDI_TSF_BLOCK_SAMPLES;
       msec += (double)block * 1000.0 / (double)SAMPLE_FREQ;
       while (msg != NULL && msec >= (double)msg->time)
          { ApplyTSFMIDIMessage(font, msg);
            msg = msg->next;
          }
       tsf_render_short(font, pcm + rendered * 2, (int)block, 0);
       rendered += block;
       if (msg == NULL && tsf_active_voice_count(font) == 0 && rendered > SAMPLE_FREQ / 4)
          break;
     }
  NMP->MC.abuf = (Uint8 *)pcm;
  NMP->MC.alen = rendered * 2 * sizeof(short);
  NMP->MC.allocated = 1;
  NMP->MC.volume = (MIX_MAX_VOLUME * NMP->VolumeLevel) / 100;
  MEDIA_TRACE("MIDI tsf rendered rawBytes=%u pcmBytes=%u notes=%d channels=%d programs=%d\n",
              NMP->RawMidiSize, NMP->MC.alen, total_notes, used_channels, used_programs);
  tsf_close(font);
  tml_free(midi);
  return(0);
}

static unsigned int ReadToneBits(const unsigned char *buf, unsigned int size,
                                 unsigned int *bitpos, unsigned int bits, int *ok)
{ unsigned int value = 0;
  unsigned int i;
  if (bits == 0) return(0);
  if (*bitpos + bits > size * 8)
     { *ok = 0;
       return(0);
     }
  for (i=0; i<bits; i++)
     { unsigned int byte_pos = (*bitpos + i) >> 3;
       unsigned int bit = 7 - ((*bitpos + i) & 7);
       value = (value << 1) | ((buf[byte_pos] >> bit) & 1);
     }
  *bitpos += bits;
  return(value);
}

static void SkipToneBits(unsigned int *bitpos, unsigned int bits)
{ *bitpos += bits;
}

static unsigned int NokiaToneDurationSamples(unsigned int duration, unsigned int spec,
                                             unsigned int bpm)
{ double quarter_ms;
  double ms;
  static const unsigned int denom[8] = { 1, 2, 4, 8, 16, 32, 32, 32 };
  if (bpm < 20) bpm = 120;
  quarter_ms = 60000.0 / (double)bpm;
  ms = quarter_ms * 4.0 / (double)denom[duration & 7];
  if (spec == 1) ms *= 1.5;
  else if (spec == 2) ms *= 1.75;
  else if (spec == 3) ms *= 0.6666667;
  if (ms < 20.0) ms = 20.0;
  return((unsigned int)((ms * (double)SAMPLE_FREQ) / 1000.0));
}

static int NokiaToneNoteToMidi(unsigned int note, unsigned int scale)
{ static const int semitone[16] = { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1, -1, -1 };
  int base;
  if (note == 0 || note >= 16 || semitone[note] < 0) return(-1);
  base = 60 + ((int)scale * 12);
  return(base + semitone[note]);
}

static int RenderToneSamples(tsf *font, short **pcm, unsigned int *capacity,
                             unsigned int *sample_pos, unsigned int samples)
{ int ret;
  if (samples == 0) return(0);
  ret = EnsureSimpleMIDICapacity(pcm, capacity, *sample_pos + samples + SAMPLE_FREQ / 8);
  if (ret != 0) return(ret);
  if (font != NULL)
     tsf_render_short(font, *pcm + (*sample_pos * 2), (int)samples, 0);
  *sample_pos += samples;
  return(0);
}

static int RenderNokiaTonePlayer(struct NativeMIDIPlayer *NMP)
{ const unsigned char *buf;
  unsigned int size;
  unsigned int bitpos;
  unsigned int char_width;
  unsigned int song_type;
  unsigned int patterns;
  unsigned int p;
  unsigned int sample_pos;
  unsigned int max_sample;
  unsigned int capacity;
  unsigned int bpm;
  unsigned int scale;
  unsigned int volume;
  unsigned int style;
  unsigned int notes;
  short *pcm;
  tsf *font;
  int ok = 1;
  int used_tsf;
  static const unsigned int tempo_table[32] =
     { 25, 28, 31, 35, 40, 45, 50, 56, 63, 70, 80, 90, 100, 112, 125, 140,
       160, 180, 200, 225, 250, 285, 320, 355, 400, 400, 400, 400, 400, 400, 400, 400 };

  buf = NMP->RawMidi;
  size = NMP->RawMidiSize;
  if (buf == NULL || size < 4) return(-1);
  if (size >= 4 && memcmp(buf, "MThd", 4) == 0) return(-2);

  bitpos = 0;
  SkipToneBits(&bitpos, 8);
  SkipToneBits(&bitpos, 8);
  if (ReadToneBits(buf, size, &bitpos, 7, &ok) == 0x22)
     { char_width = 16;
       ReadToneBits(buf, size, &bitpos, 1, &ok);
       SkipToneBits(&bitpos, 7);
     }
  else char_width = 8;
  if (!ok) return(-3);

  song_type = ReadToneBits(buf, size, &bitpos, 3, &ok);
  if (song_type == 1)
     { unsigned int len = ReadToneBits(buf, size, &bitpos, 4, &ok);
       SkipToneBits(&bitpos, len * char_width);
     }
  else if (song_type != 2) return(-4);
  if (!ok) return(-5);

  patterns = ReadToneBits(buf, size, &bitpos, 8, &ok);
  if (!ok || patterns == 0 || patterns > 32) return(-6);

  pcm = NULL;
  capacity = 0;
  sample_pos = 0;
  max_sample = 0;
  bpm = 125;
  scale = 1;
  volume = 100;
  style = 0;
  notes = 0;
  font = LoadMIDISoundFont();
  used_tsf = 0;
  if (font != NULL)
     { tsf_set_output(font, TSF_STEREO_INTERLEAVED, SAMPLE_FREQ, -12.0f);
       tsf_set_max_voices(font, 16);
       tsf_channel_set_presetnumber(font, 0, 39, 0);
       tsf_channel_midi_control(font, 0, 7, 110);
       used_tsf = 1;
     }
  if (EnsureSimpleMIDICapacity(&pcm, &capacity, SAMPLE_FREQ) != 0)
     { if (font != NULL) tsf_close(font);
       return(-7);
     }

  for (p=0; p<patterns && ok; p++)
     { unsigned int pattern_len;
       unsigned int loop_value;
       unsigned int repeat;
       unsigned int e;
       SkipToneBits(&bitpos, 3);
       SkipToneBits(&bitpos, 2);
       loop_value = ReadToneBits(buf, size, &bitpos, 4, &ok);
       pattern_len = ReadToneBits(buf, size, &bitpos, 8, &ok);
       if (pattern_len == 0) continue;
       repeat = (loop_value == 0) ? 1 : loop_value + 1;
       if (repeat > 16) repeat = 16;
       for (; repeat > 0 && ok; repeat--)
          { unsigned int event_bitpos = bitpos;
            for (e=0; e<pattern_len && ok; e++)
               { unsigned int cmd = ReadToneBits(buf, size, &event_bitpos, 3, &ok);
                 if (!ok) break;
                 if (cmd == 1)
                    { unsigned int note = ReadToneBits(buf, size, &event_bitpos, 4, &ok);
                      unsigned int duration = ReadToneBits(buf, size, &event_bitpos, 3, &ok);
                      unsigned int spec = ReadToneBits(buf, size, &event_bitpos, 2, &ok);
                      unsigned int samples = NokiaToneDurationSamples(duration, spec, bpm);
                      int midi_note = NokiaToneNoteToMidi(note, scale);
                      if (midi_note >= 0)
                         { unsigned int play_samples;
                           play_samples = samples;
                           if (style == 0) play_samples = (samples * 7) / 8;
                           else if (style == 2) play_samples = samples / 2;
                           if (play_samples == 0) play_samples = samples;
                           if (font != NULL)
                              { tsf_channel_note_on(font, 0, midi_note & 0x7f, (float)volume / 127.0f);
                                if (RenderToneSamples(font, &pcm, &capacity, &sample_pos, play_samples) != 0)
                                   ok = 0;
                                tsf_channel_note_off(font, 0, midi_note & 0x7f);
                                if (ok && samples > play_samples)
                                   { if (RenderToneSamples(font, &pcm, &capacity, &sample_pos,
                                                           samples - play_samples) != 0)
                                        ok = 0;
                                   }
                              }
                           else
                              { if (EnsureSimpleMIDICapacity(&pcm, &capacity, sample_pos + samples + SAMPLE_FREQ / 8) == 0)
                                   MixSimpleMIDINote(pcm, capacity, sample_pos,
                                                     sample_pos + play_samples, midi_note, volume);
                                sample_pos += samples;
                              }
                           notes++;
                         }
                      else
                         { if (font != NULL)
                              { if (RenderToneSamples(font, &pcm, &capacity, &sample_pos, samples) != 0)
                                   ok = 0;
                              }
                           else sample_pos += samples;
                         }
                      if (sample_pos > max_sample) max_sample = sample_pos;
                    }
                 else if (cmd == 2)
                    { scale = ReadToneBits(buf, size, &event_bitpos, 2, &ok);
                    }
                 else if (cmd == 3)
                    { style = ReadToneBits(buf, size, &event_bitpos, 2, &ok);
                    }
                 else if (cmd == 4)
                    { unsigned int tempo = ReadToneBits(buf, size, &event_bitpos, 5, &ok);
                      bpm = tempo_table[tempo & 31];
                    }
                 else if (cmd == 5)
                    { volume = 24 + ReadToneBits(buf, size, &event_bitpos, 4, &ok) * 7;
                      if (volume > 127) volume = 127;
                    }
                 else break;
               }
          }
       for (e=0; e<pattern_len && ok; e++)
          { unsigned int cmd = ReadToneBits(buf, size, &bitpos, 3, &ok);
            if (cmd == 1) SkipToneBits(&bitpos, 9);
            else if (cmd == 2 || cmd == 3) SkipToneBits(&bitpos, 2);
            else if (cmd == 4) SkipToneBits(&bitpos, 5);
            else if (cmd == 5) SkipToneBits(&bitpos, 4);
            else break;
          }
     }

  if (!ok || notes == 0 || max_sample == 0)
     { if (pcm != NULL) free(pcm);
       if (font != NULL) tsf_close(font);
       return(-8);
     }
  NMP->MC.abuf = (Uint8 *)pcm;
  NMP->MC.alen = max_sample * 2 * sizeof(short);
  NMP->MC.allocated = 1;
  NMP->MC.volume = (MIX_MAX_VOLUME * NMP->VolumeLevel) / 100;
  MEDIA_TRACE("Nokia tone rendered rawBytes=%u pcmBytes=%u notes=%u synth=%s\n",
              size, NMP->MC.alen, notes, used_tsf ? "tsf" : "simple");
  if (font != NULL) tsf_close(font);
  return(0);
}

static int RenderMIDIPlayer(struct NativeMIDIPlayer *NMP)
{ unsigned char *Tmp;
  unsigned char *Out;
  unsigned int OutSize;
  unsigned int OutCapacity;
  SDL_AudioSpec audio;
  SDL_RWops *RW;
  int Len;
  if (NMP->MC.abuf != NULL && NMP->MC.alen > 0) return(0);
  if (RenderTSFMIDIPlayer(NMP) == 0) return(0);
  if (RenderNokiaTonePlayer(NMP) == 0) return(0);
  if (RenderSimpleMIDIPlayer(NMP) == 0) return(0);
  if (NMP->Song == NULL)
     { if (NMP->RawMidi == NULL || NMP->RawMidiSize == 0) return(-1);
       RW = SDL_RWFromMem(NMP->RawMidi, NMP->RawMidiSize);
       if (RW == NULL) return(-6);
       audio.freq = 22050;
       audio.format = AUDIO_S16SYS;
       audio.channels = 2;
       audio.samples = MIDI_CHUNK_SIZE >> 2;
       audio.callback = NULL;
       NMP->Song = Timidity_LoadSong(RW, &audio);
       SDL_RWclose(RW);
       MEDIA_TRACE("MIDI native load-on-start rawBytes=%u song=%p\n", NMP->RawMidiSize, NMP->Song);
       if (NMP->Song == NULL) return(-7);
     }
  Tmp = (unsigned char *)malloc(MIDI_CHUNK_SIZE);
  if (Tmp == NULL) return(-2);
  OutCapacity = MIDI_CHUNK_SIZE * 4;
  Out = (unsigned char *)malloc(OutCapacity);
  if (Out == NULL)
     { free(Tmp);
       return(-3);
     }
  OutSize = 0;
  Timidity_Start(NMP->Song);
  while ((Len = Timidity_PlaySome(NMP->Song, Tmp, MIDI_CHUNK_SIZE)) > 0)
     { if (OutSize + Len > OutCapacity)
          { unsigned char *NewOut;
            while (OutSize + Len > OutCapacity) OutCapacity *= 2;
            NewOut = (unsigned char *)realloc(Out, OutCapacity);
            if (NewOut == NULL)
               { free(Out);
                 free(Tmp);
                 return(-4);
               }
            Out = NewOut;
          }
       memcpy(Out + OutSize, Tmp, Len);
       OutSize += Len;
     }
  free(Tmp);
  if (OutSize == 0)
     { free(Out);
       return(-5);
     }
  NMP->MC.abuf = Out;
  NMP->MC.alen = OutSize;
  NMP->MC.allocated = 1;
  NMP->MC.volume = (MIX_MAX_VOLUME * NMP->VolumeLevel) / 100;
  MEDIA_TRACE("MIDI native rendered pcmBytes=%u\n", OutSize);
  return(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_MIDIPlayer_nMidiPlayerInit()
{ struct NativeMIDIPlayer *Ret;
  Ret = malloc(sizeof(struct NativeMIDIPlayer));
  if (Ret != NULL) 
     { Ret->CheckEOM = 0;
       Ret->VolumeLevel = 100;
	       Ret->Chan = -1;
	       Ret->Song = NULL;
	       Ret->RawMidi = NULL;
	       Ret->RawMidiSize = 0;
	       Ret->Stopped = 1;
       Ret->MediaSample = 0;
       Ret->MC.allocated = 0;
       Ret->MC.volume = MIX_MAX_VOLUME;
       Ret->MC.alen = 0;
       Ret->MC.abuf = NULL;
     }
  KNI_ReturnInt((int)Ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_MIDIPlayer_nMidiPlayerRealize()
{ struct NativeMIDIPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  unsigned char *Buffer;
  unsigned int BufferSize;
  int ret=0;
  KNI_StartHandles(1);
  KNI_DeclareHandle(buf); 
  KNI_GetParameterAsObject(2, buf);
  NMP = (struct NativeMIDIPlayer *)id;
	  if (NMP->MC.abuf != NULL)
	     { free(NMP->MC.abuf);
	       NMP->MC.abuf = NULL;
	       NMP->MC.alen = 0;
	     }
	  if (NMP->Song != NULL)
	     { Timidity_FreeSong(NMP->Song);
	       NMP->Song = NULL;
	     }
	  if (NMP->RawMidi != NULL)
	     { free(NMP->RawMidi);
	       NMP->RawMidi = NULL;
	       NMP->RawMidiSize = 0;
	     }
	  BufferSize = KNI_GetArrayLength(buf);  
	  MEDIA_TRACE("MIDI native realize id=%d bytes=%u\n", id, BufferSize);
	  Buffer = malloc(BufferSize);
	  if (Buffer == NULL) ret = -1;
	  else { KNI_GetRawArrayRegion(buf, 0, (jsize)BufferSize, (jbyte*)Buffer);
	         NMP->RawMidi = Buffer;
	         NMP->RawMidiSize = BufferSize;
	       }
	  MEDIA_TRACE("MIDI native realize ret=%d rawBytes=%u song=%p\n", ret, NMP->RawMidiSize, NMP->Song);
  KNI_EndHandles();
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_MIDIPlayer_nMidiPlayerPrefetch()
{ struct NativeMIDIPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnInt(-1);
  NMP = (struct NativeMIDIPlayer *)id;
  NMP->CheckEOM = 0;
  NMP->Stopped = 1;
  MEDIA_TRACE("MIDI native prefetch id=%d chan=%d\n", id, NMP->Chan);
  KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_MIDIPlayer_nMidiPlayerStart()
{ struct NativeMIDIPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  int ret=0;
  int render_ret;
  int play_chan;
  NMP = (struct NativeMIDIPlayer *)id;
  NMP->LastTime = GetTimeMillis();
  NMP->CheckEOM = 0;
  NMP->Stopped = 0;
  if (NMP->Song == NULL && (NMP->RawMidi == NULL || NMP->RawMidiSize == 0))
     { MEDIA_TRACE("MIDI native start id=%d ret=-2 song=NULL rawBytes=%u chan=%d\n",
                   id, NMP->RawMidiSize, NMP->Chan);
       KNI_ReturnInt(-2);
     }
  if (NMP->Chan < 0)
     { NMP->Chan = ReserveChannel();
       if (NMP->Chan == -1)
          { MEDIA_TRACE("MIDI native start id=%d ret=-3 song=%p chan=%d\n", id, NMP->Song, NMP->Chan);
            KNI_ReturnInt(-3);
          }
       MediaSDL_Channels[NMP->Chan].Data = NMP;
       MediaSDL_Channels[NMP->Chan].Callback = MidiPlayerCallback;
     }
  render_ret = RenderMIDIPlayer(NMP);
  if (render_ret != 0)
     { FreeChannel(NMP->Chan);
       NMP->Chan = -1;
       MEDIA_TRACE("MIDI native start id=%d renderRet=%d\n", id, render_ret);
       KNI_ReturnInt(-4);
     }
  Mix_Volume(NMP->Chan, MediaSDL_MixerVolume());
  play_chan = Mix_PlayChannel(NMP->Chan, &NMP->MC, 0);
  if (play_chan == -1)
     { FreeChannel(NMP->Chan);
       NMP->Chan = -1;
       ret = -5;
     }
  MEDIA_TRACE("MIDI native start id=%d ret=%d chan=%d pcmLen=%d playChan=%d\n",
              id, ret, NMP->Chan, NMP->MC.alen, play_chan);
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_MIDIPlayer_nMidiPlayerStop()
{ struct NativeMIDIPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeMIDIPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan != -1)
     { FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_MIDIPlayer_nMidiPlayerDeallocate()
{ struct NativeMIDIPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeMIDIPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan != -1)
     { FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_MIDIPlayer_nMidiPlayerClose()
{ struct NativeMIDIPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeMIDIPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan != -1)
     {
       FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  if (NMP->Song != NULL) Timidity_FreeSong(NMP->Song);
  if (NMP->MC.abuf != NULL) free(NMP->MC.abuf);
  if (NMP->RawMidi != NULL) free(NMP->RawMidi);
  free(NMP);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_MIDIPlayer_nGetMediaTime()
{ struct NativeMIDIPlayer *NMP;
  long Ret;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnLong(0);
  NMP = (struct NativeMIDIPlayer *)id;
  if (NMP->Stopped) KNI_ReturnLong(0);
  Ret = (NMP->MediaSample * 1000) / SAMPLE_FREQ;
  Ret += (long)(GetTimeMillis() - NMP->LastTime);
  KNI_ReturnLong(Ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_MIDIPlayer_nCheckEOM()
{ struct NativeMIDIPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnInt(1);
  NMP = (struct NativeMIDIPlayer *)id;
  KNI_ReturnInt(NMP->CheckEOM);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_MIDIPlayer_nSetVolumeLevel()
{ struct NativeMIDIPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  jint value = KNI_GetParameterAsInt(2);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeMIDIPlayer *)id;
  NMP->VolumeLevel = value;
  NMP->MC.volume = (MIX_MAX_VOLUME * value) / 100;
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_DoJaAudioBridge_nOpen()
{ struct DoJaAudioPlayer *p = NULL;
  unsigned char *buffer = NULL;
  unsigned int size;
  int count;
  int i;
  int ret = 0;
  int profile = KNI_GetParameterAsInt(2);
  unsigned char *e;
  KNI_StartHandles(1);
  KNI_DeclareHandle(buf);
  KNI_GetParameterAsObject(1, buf);
  size = KNI_GetArrayLength(buf);
  if (size < 8) goto done;
  buffer = (unsigned char *)malloc(size);
  if (buffer == NULL) goto done;
  KNI_GetRawArrayRegion(buf, 0, (jsize)size, (jbyte*)buffer);
  if (buffer[0] != 'D' || buffer[1] != 'J' ||
      buffer[2] != 'A' || buffer[3] != '1') goto done;
  count = DoJaRead16(buffer + 4);
  if (count <= 0 || 8 + count * 8 > (int)size) goto done;
  p = (struct DoJaAudioPlayer *)malloc(sizeof(struct DoJaAudioPlayer));
  if (p == NULL) goto done;
  memset(p, 0, sizeof(*p));
  p->events = (struct DoJaEvent *)malloc(sizeof(struct DoJaEvent) * count);
  if (p->events == NULL)
     { free(p);
       p = NULL;
       goto done;
     }
  p->event_count = count;
  p->volume = 100;
  if (profile < DOJA_PROFILE_GENERIC_DOJA || profile > DOJA_PROFILE_GENERIC_GM)
     profile = DOJA_PROFILE_GENERIC_DOJA;
  p->profile = profile;
  p->loop_count = 1;
  p->loop_event_pos = -1;
  for (i = 0; i < 16; i++)
     { p->channel_volume[i] = 100;
       p->channel_pan[i] = 64;
       p->channel_pitch[i] = 32;
       p->channel_pitch_range[i] = 2;
       p->channel_modulation[i] = 0;
     }
  for (i = 0; i < count; i++)
     { e = buffer + 8 + i * 8;
       p->events[i].time_ms = DoJaRead32(e);
       p->events[i].type = e[4];
       p->events[i].channel = e[5];
       p->events[i].a = e[6];
       p->events[i].b = e[7];
     }
  MediaSDL_LockAudio();
  p->next = DoJaAudioPlayers;
  DoJaAudioPlayers = p;
  MediaSDL_UnlockAudio();
  ret = (int)p;
  MEDIA_TRACE("DoJa native audio open id=%d events=%d profile=%d\n", ret, count, profile);
done:
  if (buffer != NULL) free(buffer);
  KNI_EndHandles();
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_DoJaAudioBridge_nStart()
{ struct DoJaAudioPlayer *p = (struct DoJaAudioPlayer *)KNI_GetParameterAsInt(1);
  int i;
  if (p != NULL)
     { MediaSDL_LockAudio();
       p->event_pos = 0;
       p->sample_pos = 0;
       p->loops_done = 0;
       p->section_loops_done = 0;
       for (i = 0; i < DOJA_MAX_VOICES; i++) p->voices[i].active = 0;
       p->active = 1;
       MediaSDL_UnlockAudio();
       MEDIA_TRACE("DoJa native audio start id=%d\n", (int)p);
     }
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_DoJaAudioBridge_nStop()
{ struct DoJaAudioPlayer *p = (struct DoJaAudioPlayer *)KNI_GetParameterAsInt(1);
  int i;
  if (p != NULL)
     { MediaSDL_LockAudio();
       p->active = 0;
       for (i = 0; i < DOJA_MAX_VOICES; i++) p->voices[i].active = 0;
       MediaSDL_UnlockAudio();
       MEDIA_TRACE("DoJa native audio stop id=%d\n", (int)p);
     }
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_DoJaAudioBridge_nClose()
{ struct DoJaAudioPlayer *p = (struct DoJaAudioPlayer *)KNI_GetParameterAsInt(1);
  struct DoJaAudioPlayer **cur;
  if (p != NULL)
     { MediaSDL_LockAudio();
       cur = &DoJaAudioPlayers;
       while (*cur != NULL)
          { if (*cur == p)
               { *cur = p->next;
                 break;
               }
            cur = &((*cur)->next);
          }
       MediaSDL_UnlockAudio();
       MEDIA_TRACE("DoJa native audio close id=%d\n", (int)p);
       if (p->events != NULL) free(p->events);
       free(p);
     }
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_DoJaAudioBridge_nSetVolume()
{ struct DoJaAudioPlayer *p = (struct DoJaAudioPlayer *)KNI_GetParameterAsInt(1);
  int volume = KNI_GetParameterAsInt(2);
  if (volume < 0) volume = 0;
  if (volume > 100) volume = 100;
  if (p != NULL) p->volume = volume;
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_DoJaAudioBridge_nSetLoopCount()
{ struct DoJaAudioPlayer *p = (struct DoJaAudioPlayer *)KNI_GetParameterAsInt(1);
  int loop_count = KNI_GetParameterAsInt(2);
  if (p != NULL) p->loop_count = loop_count;
  KNI_ReturnVoid();
}

/*****************************************************************************/

struct ToneSequenceEvent
{ signed char Type;
  signed char Value;
};

struct ToneSequenceBlock
{ int Start;
  int Stop;
  int Size;
};

struct ToneSequenceLoaded
{ char                      Tempo;
  char                      Resolution;
  struct ToneSequenceBlock  TS_Blocks[256];
  struct ToneSequenceBlock  TS_Main;
  char                     *Buffer;
  int                       BufSize;
  struct ToneSequenceEvent *Sequence;
  int                       SeqSize;
};

int EvalBlockSize(struct ToneSequenceEvent *Events, struct ToneSequenceBlock *Block, struct ToneSequenceBlock *Blocks, struct ToneSequenceEvent *Result)
{ struct ToneSequenceEvent *evt;
  int i, start, stop, ret, pw, j;
  start = Block->Start;
  stop = Block->Stop + 1;
  for(ret=0,pw=0,i=start; i<stop; i++)
     { evt = &Events[i];
       if (evt->Type >= -1) // PLAY_NOTE or SILENCE
          { ret++;
            if (Result != NULL) 
               { Result[pw].Type = evt->Type;
                 Result[pw].Value = evt->Value;
                 pw++;  
               }
          }
       else switch(evt->Type)
                  { case -7: // PLAY_BLOCK
                             ret += Blocks[(unsigned char)evt->Value].Size;
                             if (Result != NULL) 
                                { j = EvalBlockSize(Events, &Blocks[(unsigned char)evt->Value], Blocks, &Result[pw]);
                                  if (j < 0) return(-1);
                                  pw += j;
                                }
                             break;
                    case -8: // SET_VOLUME
                             ret++;
                             if (Result != NULL) 
                                { Result[pw].Type = evt->Type;
                                  Result[pw].Value = evt->Value;
                                  pw++;  
                                }
                             break;
                    case -9: // REPEAT
                             ret += evt->Value;
                             i++;
                             if (Result != NULL)
                                { for(j=0; j<evt->Value; j++,pw++)
                                     { Result[pw].Type = Events[i].Type;
                                       Result[pw].Value = Events[i].Value;
                                     }
                                }
                             break;
                    
                  }
     }
  return(ret);
}

int EvalBlocksSize(struct ToneSequenceLoaded *TSL)
{ int i, ret;
  for(i=0; i<256; i++)
     if (TSL->TS_Blocks[i].Start > -1)
        { ret = EvalBlockSize((struct ToneSequenceEvent *)TSL->Buffer, &TSL->TS_Blocks[i], TSL->TS_Blocks, NULL);
          if (ret < 0) return(-1);
          TSL->TS_Blocks[i].Size = ret;
        } 
  for(i=0; i<256; i++)
     if (TSL->TS_Blocks[i].Start > -1)
        { ret = EvalBlockSize((struct ToneSequenceEvent *)TSL->Buffer, &TSL->TS_Blocks[i], TSL->TS_Blocks, NULL);
          if (ret < 0) return(-1);
          TSL->TS_Blocks[i].Size = ret;
        } 
  return(0);
}

struct ToneSequenceLoaded *InitToneSequence(char *Buffer, unsigned int BufSize)
{ struct ToneSequenceLoaded *Ret;
  struct ToneSequenceEvent *Events;
  int i, ss, inBlock, inMain;
  if ((BufSize & 0x01) != 0) return(NULL);
  Ret = (struct ToneSequenceLoaded *)malloc(sizeof(struct ToneSequenceLoaded));
  if (Ret == NULL) return(NULL);
  Ret->Tempo = 30;
  Ret->Resolution = 64;
  Ret->Buffer = Buffer;
  Ret->BufSize = BufSize;
  Events = (struct ToneSequenceEvent *)Buffer;
  if ((Events[0].Type != -2) || (Events[0].Value != 1)) // VERSION = 1
     { free(Ret);
       return(NULL);
     }
  for(i=0; i<256; i++) 
     { Ret->TS_Blocks[i].Start = -1;
       Ret->TS_Blocks[i].Stop = -1;
       Ret->TS_Blocks[i].Size = 0;
     }
  Ret->TS_Main.Start = -1;
  Ret->TS_Main.Stop = -1;
  Ret->TS_Main.Size = 0;
  ss = Ret->BufSize >> 1;
  inBlock = inMain = 0;
  for(i=1; i<ss; i++)
     { switch(Events[i].Type)
             { case -3: // TEMPO
                        Ret->Tempo = Events[i].Value;
                        break;
               case -4: // RESOLUTION
                        Ret->Resolution = Events[i].Value;
                        break;
               case -5: // BLOCK_START
                        if ((Ret->TS_Blocks[(unsigned char)Events[i].Value].Start != -1) || (inBlock == 1) || (inMain == 1))
                           { free(Ret);
                             return(NULL);
                           }
                        Ret->TS_Blocks[(unsigned char)Events[i].Value].Start = i+1;
                        inBlock = 1;
                        break;
               case -6: // BLOCK_END
                        if ((Ret->TS_Blocks[(unsigned char)Events[i].Value].Start == -1) || (Ret->TS_Blocks[(unsigned char)Events[i].Value].Stop != -1) || (inBlock == 0))
                           { free(Ret);
                             return(NULL);
                           }
                        Ret->TS_Blocks[(unsigned char)Events[i].Value].Stop = i-1;
                        inBlock = 0;
                        break;
               case -7: // PLAY_BLOCK
                        if ((inBlock == 0) && (inMain == 0)) 
                           { inMain = 1;
                             Ret->TS_Main.Start = i;
                           }
                        break;
               case -8: // SET_VOLUME
                        if ((inBlock == 0) && (inMain == 0)) 
                           { inMain = 1;
                             Ret->TS_Main.Start = i;
                           }
                        break;
               case -9: // REPEAT
                        if ((inBlock == 0) && (inMain == 0)) 
                           { inMain = 1;
                             Ret->TS_Main.Start = i;
                           }
                        break;
               default: if (Events[i].Type >= -1) // PLAY_NOTE or SILENCE
                           { if ((inBlock == 0) && (inMain == 0)) 
                              { inMain = 1;
                                Ret->TS_Main.Start = i;
                              }
                           }
                        else { // Invalid
                               free(Ret);
                               return(NULL);
                             }
             }
     }
  Ret->TS_Main.Stop = ss-1;
  if (EvalBlocksSize(Ret) != 0)
     { free(Ret);
       return(NULL);
     }
  Ret->SeqSize = EvalBlockSize(Events, &Ret->TS_Main, Ret->TS_Blocks, NULL);
  if (Ret->SeqSize == -1)
     { free(Ret);
       return(NULL);
     }
  Ret->Sequence = (struct ToneSequenceEvent *)malloc(sizeof(struct ToneSequenceEvent) * Ret->SeqSize);
  if (Ret->Sequence == NULL)
     { free(Ret);
       return(NULL);
     }
  EvalBlockSize(Events, &Ret->TS_Main, Ret->TS_Blocks, Ret->Sequence);
  return(Ret);
}

#define TS_CHUNK_SIZE	16384

struct NativeTSPlayer
{ struct ToneSequenceLoaded *Loaded;
  struct NativeTonePlayer   *NTP;
  int                        Chan;
  long                       TimeSampled;
  long long                  LastTime;
  int                        CheckEOM;
  int                        TimeMult;
  int                        ActualNote;
  int                        ActualDuration;
  int                        ActualVolume;
  int                        Stopped;
};

int PlayNextTone(struct NativeTSPlayer *NMP)
{ int Type=0;
  if (NMP->NTP != NULL) 
     { FreeToneChunk(NMP->NTP);
       NMP->NTP = NULL;
     }
  do { NMP->ActualNote++;
       if (NMP->ActualNote >= NMP->Loaded->SeqSize) return(-1);
       Type = NMP->Loaded->Sequence[NMP->ActualNote].Type;
       if (Type == -8) NMP->ActualVolume = NMP->Loaded->Sequence[NMP->ActualNote].Value;
     } while(Type == -8); 
  if (Type == -1) NMP->NTP = CreateToneChunk(0x40, 0);
  else NMP->NTP = CreateToneChunk(Type, NMP->ActualVolume);
  if (NMP->NTP == NULL) return(-1);
  NMP->ActualDuration = NMP->Loaded->Sequence[NMP->ActualNote].Value * NMP->TimeMult;
  NMP->LastTime = GetTimeMillis();
  Mix_Volume(NMP->Chan, MediaSDL_MixerVolume());
  Mix_PlayChannelTimed(NMP->Chan, &NMP->NTP->MC, -1, NMP->ActualDuration);
  return(0);
}

void TSPlayerCallback(int chan)
{ struct NativeTSPlayer *NMP = (struct NativeTSPlayer *)MediaSDL_Channels[chan].Data;
  if (NMP == NULL) return;
  NMP->TimeSampled += NMP->ActualDuration;
  if (NMP->Stopped) return;
  if (PlayNextTone(NMP) != 0) NMP->CheckEOM = 1;
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerInit()
{ struct NativeTSPlayer *Ret;
  Ret = malloc(sizeof(struct NativeTSPlayer));
  if (Ret != NULL) 
     { Ret->CheckEOM = 0;
       Ret->Chan = -1;
       Ret->TimeSampled = 0;
       Ret->NTP = NULL;
       Ret->Loaded = NULL;
       Ret->Stopped = 1;
     }
  KNI_ReturnInt((int)Ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerRealize()
{ struct NativeTSPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  int ret=0, seqsize;
  char *sequence;
  KNI_StartHandles(1);
  KNI_DeclareHandle(buf); 
  KNI_GetParameterAsObject(2, buf);
  NMP = (struct NativeTSPlayer *)id;
  seqsize = KNI_GetArrayLength(buf);  
  sequence = malloc(seqsize);
  if (sequence == NULL) ret = -1;
  else { KNI_GetRawArrayRegion(buf, 0, (jsize)seqsize, (jbyte*)sequence);
         NMP->Loaded = InitToneSequence(sequence, seqsize);
         if (NMP->Loaded == NULL) { free(sequence); ret = -1;}
         else { NMP->TimeMult = 240000 / (NMP->Loaded->Resolution * NMP->Loaded->Tempo * 4);
                printf("\nToneSequence: Load OK! (Tempo=%d  Resolution=%d  TimeMult=%d)\n", NMP->Loaded->Tempo, NMP->Loaded->Resolution, NMP->TimeMult);
              }   
       }
  KNI_EndHandles();
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerStart()
{ struct NativeTSPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  int ret=0;
  NMP = (struct NativeTSPlayer *)id;
  if (NMP->Loaded == NULL) KNI_ReturnInt(-2);
  NMP->Chan = ReserveChannel();
  if (NMP->Chan == -1) KNI_ReturnInt(-1);
  MediaSDL_Channels[NMP->Chan].Data = NMP;
  MediaSDL_Channels[NMP->Chan].Callback = TSPlayerCallback;
  NMP->ActualNote = 0;
  NMP->ActualVolume = 100;
  NMP->LastTime = GetTimeMillis();
  NMP->CheckEOM = 0;
  NMP->TimeSampled = 0;
  NMP->Stopped = 0;
  if (PlayNextTone(NMP) != 0)
     { FreeChannel(NMP->Chan);
       NMP->Chan = -1;
       ret=-1;
     }
  KNI_ReturnInt(ret);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerStop()
{ struct NativeTSPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeTSPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan >= 0)
     { FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan >= 0) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerDeallocate()
{ struct NativeTSPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeTSPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan >= 0) 
     { FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan >= 0) Mix_HaltChannel(chan);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_javax_microedition_media_ToneSequencePlayer_nTSPlayerClose()
{ struct NativeTSPlayer *NMP;
  int chan;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnVoid();
  NMP = (struct NativeTSPlayer *)id;
  MediaSDL_LockAudio();
  NMP->Stopped = 1;
  chan = NMP->Chan;
  if (NMP->Chan != -1)
     { FreeChannelUnlocked(NMP->Chan);
       NMP->Chan = -1;
     }
  MediaSDL_UnlockAudio();
  if (chan != -1) Mix_HaltChannel(chan);
  if (NMP->Loaded != NULL)
     { free(NMP->Loaded->Buffer);
       free(NMP->Loaded->Sequence);
       free(NMP->Loaded);
     }
  if (NMP->NTP != NULL) FreeToneChunk(NMP->NTP);
  free(NMP);
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG Java_javax_microedition_media_ToneSequencePlayer_nTSGetMediaTime()
{ struct NativeTSPlayer *NMP;
  long Ret;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnLong(0);
  NMP = (struct NativeTSPlayer *)id;
  Ret = NMP->TimeSampled;
  Ret += (long)(GetTimeMillis() - NMP->LastTime);
  KNI_ReturnLong(Ret);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_javax_microedition_media_ToneSequencePlayer_nTSCheckEOM()
{ struct NativeTSPlayer *NMP;
  jint id = KNI_GetParameterAsInt(1);
  if (id == 0) KNI_ReturnInt(1);
  NMP = (struct NativeTSPlayer *)id;
  KNI_ReturnInt(NMP->CheckEOM);
}
