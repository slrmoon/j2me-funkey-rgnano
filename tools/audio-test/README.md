# Audio tests

How the runtime's sound compares with a reference, without a device.

- `build.sh [dir]` builds `fkrender`: the audio engine as a desktop program.
  `fkrender in.mid out.wav [seconds] [synth 1 wavetable|2 fm|3 hybrid] [volume] [loops]`
  prints the detected type, the duration and every event with its time.
  `PHONEME_AUDIO_QUALITY=low` renders as the FunKey build does, the default
  on a PC is the high level; `PHONEME_MIDI_GAIN` as in the runtime.
- `wavs.py game...` - every WAV of `media/<game>/` against a reference
  decode (sox): length, level, time offset, correlation and how much energy
  sits above the sound's own band (images of the resampler).
- `mids.py game...` - every MIDI file in the three synth presets against
  FluidSynth + FluidR3_GM: the duration against the file's own length, the
  time of END_OF_MEDIA, RMS and peak level, spectral distance.

Both scripts expect `tool/fkrender` and `media/<game>/` (sound files taken
out of a jar) under the current directory, and need numpy, sox and
fluidsynth (`apt-get install sox fluidsynth fluid-soundfont-gm`).

What a game does with its players: run it with `PHONEME_AUDIO_TRACE=1`
(browser: `?env=PHONEME_AUDIO_TRACE=1`, lines in the console) - every
create, prefetch, start, stop, seek, volume, loop count and event, with the
time in ms: `snd: 2026 start #3 at 0 ms`.
