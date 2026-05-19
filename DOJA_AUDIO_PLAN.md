# DoJa Audio Work Plan

Goal: replace MLD/MFi playback with a dedicated live streaming DoJa audio path, while keeping WAV/AMR sample playback on the existing chunk-based path.

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

- [~] Implement minimal DoJa synth.
  - Current synth has simple wave families and pan/volume handling.
  - Need envelopes, better instrument mapping, noise/drum behavior, and less “plain beeper” timbre.

- [~] Add named sound profiles.
  - Planned profiles: `generic_doja`, `nokia_s40`, `sony_ericsson`, `generic_gm`.
  - Selection added via system property: `-Ddoja.audio.profile=generic_doja`.
  - Java maps profile names to native profile IDs.
  - Native synth now chooses waveform/envelope/gain per profile.
  - Need device-specific tuning after listening tests.

- [~] Improve DoJa/MFi-like timbre.
  - Added per-profile program family waveform/envelope mapping.
  - Added short attack/decay/release.
  - Pitch bend and modulation-driven vibrato are present in native code, but parser emission is disabled until BGM start behavior is stable.
  - Added first drum/noise mappings.
  - Need tune mappings against Rockman/reference recordings.

- [~] Improve debug tooling.
  - Keep summary logs by default.
  - Optional event dump exists behind `-Ddoja.audio.dump`.
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

Keep the legacy MLD fallback as the default working path, then continue streaming audio work behind `-Ddoja.audio.streaming=true` until it produces audible BGM on device.

## Change Log

- 2026-05-19 21:21: Built runtime and OPK with native streaming profiles, loop/pitch/mod parser events, and SDL post-mix playback. Device result: Rockman BGM disappeared; logs showed `DoJaAudioPlayer.open` but no audible music.
- 2026-05-19 21:28: Disabled `0xdd` loop-marker event emission from the Java MLD event stream and rebuilt OPK. Device result: BGM still absent.
- 2026-05-19 21:32: Disabled `0xe4` pitch bend, `0xe7` pitch range, and `0xea` modulation event emission from the Java MLD event stream and rebuilt OPK. Device result: BGM still absent.
- 2026-05-19 21:38: Changed `MediaManager` so streaming DoJa audio is opt-in through `-Ddoja.audio.streaming=true`; default MLD playback returns to the legacy MIDI/PCM path that previously produced music.
- 2026-05-19 21:39: Rebuilt `doja.opk` after making streaming opt-in and ensuring `unuse()` closes any opt-in DoJa streaming player.
- 2026-05-19 21:45: Device test confirmed sound is back on the default path. Logs show `DoJa MLD converted to MIDI v9` followed by `DoJa MIDI sound play`.
