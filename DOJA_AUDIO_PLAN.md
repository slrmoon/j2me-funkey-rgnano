# DoJa Audio Work Plan

Goal: replace MLD/MFi playback with a dedicated live streaming DoJa audio path, while keeping WAV/AMR sample playback on the existing chunk-based path.

## Current Stream Architecture

When launcher audio mode is `Stream`, DoJa MLD/MFi playback does not go through `GenericPlayer` or `MIDIPlayer`.

1. `MediaManager` loads MLD/MFi bytes from scratchpad/JAR and, only when `-Ddoja.audio.streaming=true`, creates `DoJaAudioPlayer`.
2. `DoJaAudioPlayer` calls `MldEventStream.build(...)`, which parses the MLD into a compact `DJA1` event stream: program, volume, pan, pitch, modulation, pitch range, expression, note on/off, loop/end.
3. `DoJaAudioBridge.nOpen(...)` receives `DJA1` in native code and registers the player with the SDL post-mix callback.
4. Native stream playback is live: the post-mix callback advances by playback time, applies due events, and mixes audio into SDL output.
5. The intended sound path is now TSF/soundfont-backed: `DoJaAudioBridge` loads the same `lib/soundfont/default.sf2` used by native MIDI rendering and applies stream events to TinySoundFont. The old oscillator synth remains only as a fallback if the soundfont cannot be loaded.

## Rules

- Do not use `GenericPlayer` or `MIDIPlayer` for DoJa MLD/MFi playback.
- Do not pre-render full MLD/MFi tracks into WAV/PCM chunks for normal playback.
- Keep loop handling inside the DoJa sequencer, not SDL_mixer loop flags.
- Keep OPK builds manual: only build when explicitly requested.
- Keep logs useful: no per-key spam; verbose audio dumps only behind a debug flag/property.

## Checklist

- [x] Create a separate DoJa audio path in Java.
  - `com.nttdocomo.ui.DoJaAudioPlayer` exists in `doja-support.jar`.
  - `MediaManager` can route MLD/MFi to DoJa audio when `-Ddoja.audio.streaming=true` is set.
  - Default route is currently the legacy MLD-to-MIDI/PCM fallback because the streaming path opened streams but produced no audible BGM on device.

- [x] Stop relying on external-JAR native lookup for DoJa audio.
  - Added ROM-side `javax.microedition.media.DoJaAudioBridge`.
  - Added it to `media-sdl/lib.gmk`.
  - Native methods are now visible in `nativeFunctionTable.cpp`.

- [x] Add native streaming audio entry points.
  - Current bridge methods: `nOpen`, `nStart`, `nStop`, `nClose`, `nSetVolume`, `nSetLoopCount`.
  - Current native implementation renders live through SDL post-mix.

- [x] Remove noisy key logs.
  - Removed `DoJa key pressed/released/repeated` prints from DoJa `Canvas`.

- [~] Implement MLD/MFi parser and compact event stream.
  - Current event stream supports program, volume, pan, note on/off, loop start/end, end.
  - `0xdd` MFi loop point control was tested and disabled again because Rockman BGM disappeared.
  - `0xe4` pitch bend, `0xe7` pitch range, and `0xea` modulation depth were tested and disabled again because Rockman BGM still disappeared.
  - Need audit against more MLD/MFi variants and real game files.

- [~] Implement live sequencer.
  - Current sequencer processes events incrementally by playback time.
  - Internal loop marker support is present in native code, but Java parser does not emit loop markers yet.
  - End-of-track repeats now restart the whole stream instead of jumping to the last embedded loop point.
  - Need improve tempo handling and stop/restart edge cases.

- [x] Move live DoJa streaming to soundfont-backed synthesis.
  - `DoJaAudioBridge` now loads the packaged `lib/soundfont/default.sf2` through the same `LoadMIDISoundFont()` path used by native MIDI.
  - Stream events are applied live to TinySoundFont channels in SDL post-mix.
  - The older simple oscillator synth remains as a fallback only when TSF cannot load a soundfont.

- [x] Use one TSF-backed backend for MIDI-like audio paths.
  - Legacy `MIDIPlayer` MIDI/SP-MIDI playback uses `TSF_MIDI`.
  - DoJa `DJA1` live stream events use `TSF_STREAM_EVENTS`.
  - MMAPI tone and tone-sequence chunks are TSF-backed when the soundfont is available.
  - Direct TinySoundFont note/program/controller/pitch calls are centralized behind `TSFBackend*`.
  - WAV/AMR sample playback remains `PCM_DIRECT` and does not enter TSF.
  - Oscillator/timidity paths are retained only as `UNSUPPORTED` emergency fallback paths.

- [~] Add named sound profiles.
  - Planned profiles: `generic_doja`, `nokia_s40`, `sony_ericsson`, `generic_gm`.
  - Selection added via system property: `-Ddoja.audio.profile=generic_doja`.
  - Java maps profile names to native profile IDs.
  - Native TSF output shaping now uses the selected profile.
  - Need device-specific tuning after listening tests.

- [~] Improve DoJa/MFi-like timbre.
  - Main timbre should now come from the packaged soundfont, not the simple native oscillator.
  - Live stream events now preserve MLD/MFi bank values into the native TSF program path.
  - Bank-aware TSF mapping handles the observed bank-2 percussion-like channel and maps selected MLD bank-2/bank-3 programs away from plain GM piano.
  - Need tune pitch bend, expression, channel volume, and loop behavior against Rockman/reference recordings.
  - Need compare reference phone/emulator timbre against current soundfont preset mapping; matching legacy MIDI is not enough if legacy MIDI is also musically wrong.

- [~] Improve debug tooling.
  - Keep summary logs by default.
  - Optional event dump exists behind `-Ddoja.audio.dump`.
  - Optional legacy-vs-stream comparison exists behind `-Ddoja.audio.compare=true`
    or launcher env `DOJA_AUDIO_COMPARE=1`.
  - Comparison prints normalized `MLD_EVENT` rows for stream and legacy MIDI,
    then `MLD_DIFF` rows for program, bank, channel, note, velocity,
    controller, pitch bend, and tempo mismatches.
  - Keep extending event dump coverage:
    - `time=0 ch=0 program=12`
    - `time=120 ch=0 note_on 64 vel=80`
    - `time=240 ch=0 note_off 64`
    - `time=240 loop_start`
    - `time=960 loop_end count=0`

- [ ] Compare against reference recordings.
  - Record current output.
  - Compare against original/emulator reference by ear first.
  - Adjust profile mappings incrementally.

- [ ] Keep sample playback unchanged.
  - WAV/AMR samples stay chunk-based through existing sample path.
  - Only MLD/MFi BGM/SFX use DoJa streaming synthesis.

## Next Step

Test the next OPK with launcher audio mode set to `Stream`, compare against the phone/emulator reference, and focus on preset/bank/drum mapping rather than oscillator tuning. If the BGM still sounds like layered piano, inspect the stream logs for bank/program/channel patterns around the background music resources, especially `scratchpad:///0;pos=316476` and nearby larger MLD entries.

## Change Log

- 2026-05-19 21:21: Built runtime and OPK with native streaming profiles, loop/pitch/mod parser events, and SDL post-mix playback. Device result: Rockman BGM disappeared; logs showed `DoJaAudioPlayer.open` but no audible music.
- 2026-05-19 21:28: Disabled `0xdd` loop-marker event emission from the Java MLD event stream and rebuilt OPK. Device result: BGM still absent.
- 2026-05-19 21:32: Disabled `0xe4` pitch bend, `0xe7` pitch range, and `0xea` modulation event emission from the Java MLD event stream and rebuilt OPK. Device result: BGM still absent.
- 2026-05-19 21:38: Changed `MediaManager` so streaming DoJa audio is opt-in through `-Ddoja.audio.streaming=true`; default MLD playback returns to the legacy MIDI/PCM path that previously produced music.
- 2026-05-19 21:39: Rebuilt `doja.opk` after making streaming opt-in and ensuring `unuse()` closes any opt-in DoJa streaming player.
- 2026-05-19 21:45: Device test confirmed sound is back on the default path. Logs show `DoJa MLD converted to MIDI v9` followed by `DoJa MIDI sound play`.
- 2026-05-19: Changed native DoJa streaming open to call `InitAudioSubsystem()` before registering the post-mix player, so the streaming-only path does not depend on a legacy `Player` having already opened SDL_mixer.
- 2026-05-19: Device log still showed the legacy MIDI path because `-Ddoja.audio.streaming=true` was absent from `runMidlet` argv. Added launcher support for `DOJA_AUDIO_STREAMING=1` and optional `DOJA_AUDIO_PROFILE=...`.
- 2026-05-20: Gated noisy DoJa streaming diagnostics behind `DOJA_AUDIO_DEBUG=1`, added MLD pitch/modulation/pitch-range/expression events to the stream, sorted controller events before same-time notes, and mirrored the legacy `0xba` drum-channel remap.
- 2026-05-20: Reworked native DoJa stream playback to load `default.sf2` with TinySoundFont and apply `DJA1` events directly to TSF channels; the simple oscillator path is now only a no-soundfont fallback.
- 2026-05-20: Reduced live TSF stream cost by sharing one parsed base soundfont with `tsf_copy()` per player and rendering TSF in small cached blocks up to the next event instead of one sample per synth call.
- 2026-05-20: Compared current device recording against the phone/emulator reference. The current stream is still too bright and high-frequency; the reference has much stronger low/low-mid and percussive energy. Corrected stream `0xe1` handling so MFi bank select no longer turns programs 16/17 into GM 80/81 synth leads.
- 2026-05-20: Compared `PC.wav` reference against latest `rg.wav`. RG still had much less 100-500 Hz energy and much more 2-8 kHz energy. Added a `generic_doja` TSF post-filter that boosts the low/low-mid component and attenuates the high component, and made the launcher set `DOJA_AUDIO_PROFILE=generic_doja` by default for DoJa runs.
- 2026-05-21: Compared the next `pc.wav`/`rg.wav` pair. The first phone-shaping filter over-boosted 0-100 Hz while still missing 100-500 Hz. Replaced it with a DC/sub high-pass plus a separate low-mid band boost so the filter targets roughly 100-500 Hz instead of all low frequencies from zero.
- 2026-05-21: Unified MIDI-like native playback around the TSF-backed backend. Legacy MIDI, DoJa stream events, and tone chunks now log `TSF_MIDI` or `TSF_STREAM_EVENTS`; WAV/AMR logs `PCM_DIRECT`; oscillator/timidity paths are explicit `UNSUPPORTED` fallbacks.
- 2026-05-21: Added `doja.audio.compare` diagnostic mode. It builds both the live `DJA1` stream and the legacy MLD-to-MIDI output from the same MLD bytes, prints comparable normalized event rows, and summarizes likely mapping differences before changing synthesis.
- 2026-05-21: Device comparison logs showed stream programs such as `program=17 bank_lsb=3` where legacy emitted `program=81`. Updated live `MldEventStream` `0xe0/0xe1` handling to mirror legacy: bank bit 0 becomes program bit 6, so stream receives the same GM program number legacy used.
- 2026-05-21: Bounded `doja.audio.compare` output after a device log reached thousands of lines and ended mid-`MLD_DIFF` during a large BGM. Compare now prints limited event/diff rows, reason counts, and skips duplicate resource comparisons.
- 2026-05-22: Device test with `doja.audio.streaming=true` confirmed `TSF_STREAM_EVENTS` is active, but the stream still sounds close to legacy MIDI and too piano-like compared with phone/emulator audio. Added music-like DoJa stream exclusivity so long BGM streams stop older long BGM streams instead of stacking, while short SFX can still overlap.
- 2026-05-22: Preserved stream bank values into native TSF program changes and added bank-aware preset mapping. Channel 15 with bank 2/program 0 is treated as percussion-like TSF drum-bank output, and selected bank 2/3 programs are remapped toward synth lead-style GM presets instead of defaulting to plain piano.
- 2026-05-22: Checked `vavi-sound` as an MFi parser reference. It confirms `0xe0` voice/program and `0xe1` voice/bank parsing, the low bank bit becoming GM program bit 6, and percussion pitch using `note + 35` instead of melodic `note + 45`. Updated both live stream and legacy MLD paths to apply the percussion pitch offset, and added `DoJaAudioPlayer.open id=... name=...` logging so native `DOJA_MAP` rows can be tied back to the original scratchpad resource.
- 2026-05-22: Corrected MFi delta/gate timing to match `vavi-sound` tempo semantics. Live stream now converts MFi ticks with a fixed 48-tick base (`ticks * 60000 / (48 * tempo)`), and the legacy MIDI converter now maps MFi ticks to MIDI resolution 96 as `ticks * 2`. This avoids shortening notes whenever an MLD track changes `timeBase` to 96/192/etc.
- 2026-05-22: Device log after the timing fix showed better rhythm but native bank-aware preset mapping was too aggressive: several distinct BGM programs (`34`, `59`, `65`, `62`) were collapsed to TSF programs `38/80/81`. Removed that second native remap so melodic channels preserve the already-correct Java/vavi-derived program number; only TSF drum-bank selection remains special-cased.
