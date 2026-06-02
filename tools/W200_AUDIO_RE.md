# Sony Ericsson W200 audio reverse engineering

The reproducible scanner is `tools/w200_audio_scan.py`. Run it from the
repository root:

```sh
python3 tools/w200_audio_scan.py \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/W200_R4JA011_MAIN_GENERIC_ME_RED53.mbn \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/W200_R4JA011_FS_RUSSIA_RED53.fbn \
  --out phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan
```

## Confirmed architecture

The MAIN image contains eight MegaStar DSP modules with a shared module
header:

| Module | MAIN file offset |
| --- | ---: |
| `MIDI_PCM_ASR` | `0x002108b6` |
| `Phone_Audio` | `0x003aa592` |
| `Phone_Audio_TTY` | `0x003b54d4` |
| `Phone_Audio_PTT` | `0x003c1d48` |
| `AMR_ASR` | `0x003c93ee` |
| `AAC` | `0x003cfe8e` |
| `MP3` | `0x003d8f30` |
| `Video_H263` | `0x003e24ce` |

The extracted `MIDI_PCM_ASR` module is 36,466 bytes and has SHA-256:

`20700c3578a63575183720f88af6246350d692ee5d796c6ddb68e37f7402e914`

## MegaStar module format

The MegaStar processor in this firmware is consistent with the TI C55x
family. The strongest static confirmation is the module prefix:

- bytes `0x00..0x0f`: common module header
- bytes `0x10..0x10f`: 32 interrupt vectors, 8 bytes each
- byte `1..3` of each vector: little-endian 24-bit C55x ISR address
- byte `0x110` onward: variable-width C55x payload containing code and data

This matches the TI C55x CPU reference guide: C55x has up to 32 interrupt
vectors and each vector is exactly 8 bytes; bytes 1 through 3 encode the
24-bit byte address of the interrupt service routine.

For `MIDI_PCM_ASR`:

- reset vector ISR: `0x5e8002`
- default vector ISR: `0x5e8005`
- vectors `1..30` share the same default handler

Comparison across the eight modules supports a strong static inference that
the payload load VMA is `0x5e8000`. Under that mapping, the `MIDI_PCM_ASR`
reset ISR is payload-relative offset `+0x2`, stored at module file offset
`0x112`; the shared default ISR is payload-relative offset `+0x5`, stored at
module file offset `0x115`. The scanner records these inferred mappings for
every vector. Confirming the mapping still requires loader disassembly or a
runtime trace.

The scanner records raw bytes and decoded ISR addresses for every vector in
all eight modules. The open-source C55x binutils port can produce a first raw
disassembly with:

```sh
tools/build_c55x_binutils.sh
tools/w200_c55x_disasm.sh \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.bin
```

This is a linear raw disassembly. It is useful for locating instruction
sequences, but it does not distinguish code from embedded data and must not be
treated as a recovered control-flow graph.

The first flow-aware CFG pass is reproducible with:

```sh
tools/w200_c55x_cfg.py \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.bin \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.asm \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.entry-0x112.asm \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.entry-0x115.asm \
  --entry 0x112 --entry 0x115 \
  --out phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_DSP_MIDI_PCM_ASR.cfg.json
```

The entry-specific listings are intentional. Because C55x instructions are
variable-width, a linear listing starting at `0x110` does not necessarily
decode `0x112` as a separate instruction boundary. The entry listings force
the raw disassembler to decode from the vector targets.

Current CFG summary:

| Entry | Visited instructions | Unknown instructions | Control instructions | Module range | Truncated |
| ---: | ---: | ---: | ---: | --- | --- |
| `0x112` | 68 | 10 | 11 | `0x112..0x5f75` | no |
| `0x115` | 67 | 10 | 11 | `0x115..0x5f75` | no |

Both entries currently walk through a short bootstrap/default-handler path
that branches to `0x5f4a` and then falls through to `0x5f75`. This is not yet
the MIDI render loop; it is the vector-facing entry path.

The same pass produces a coarse code/data map. The chunks around
`0x6300..0x65ff` contain UTF-16 process labels and are classified as data
despite the linear raw disassembler producing instruction-looking output
there. The pitch table at `0x656a` and the sample-rate table at `0x6790` must
also be treated as embedded data, not C55x code.

An exploratory extra CFG was also generated from repeated raw `CALL` targets:

`W200_R4JA011_DSP_MIDI_PCM_ASR.extra-cfg.json`

The useful outcome is negative. Entries such as `0x312e` and `0x31b8` walk
through dense `.byte` regions and impossible-looking calls, which means the
linear raw listing is decoding embedded tables as control flow. These entries
are rejected as function starts until a better C55x decoder or loader-derived
symbol table confirms them.

Its process list contains:

- `MIDI_Synthesizer`
- `Mixer_Process`
- `Tonegen_Process`
- `pcmif_ul_proc`
- `pcmif_dl_proc`
- `Pcmif_InterruptFunc`
- `MMA_Decoder`
- `crypto_proc`
- `ASR`

The likely phone playback path is:

`Java MMAPI / Nokia Sound API -> ARM player manager -> ARM synthesizer API ->
MegaStar MIDI_Synthesizer -> Mixer_Process -> PCM interface -> output`

## Confirmed MIDI facts

The MIDI DSP module contains:

- a 128-entry quantized pitch table at module offset `0x656a`
- a sample-rate table at module offset `0x6790`:
  `8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000`
- build identity:
  `CRH109693_R4J MIDI_PCM_ASR --- QCHRRAN_ir84094d Thu Jan 26 15:04:08 2006`

The pitch table also appears next to `Tonegen_Process` in the other DSP
modules, and the same rate table appears in six audio modules. These are
shared MegaStar audio primitives, not evidence of MIDI instrument timbres.

The MAIN ARM region contains:

- `DSP_RM_LOADMODULE_MIDI_PCM_ASR`
- `DSP_RM_LOADMODULE_MIDI_72`
- `CCustomMidiBank_imp.c`
- `CPlayerManager_imp.c`
- `CSynthesizer_imp.c`
- `_capMIDI`, `_capCMIDI`, `_capCompoundSound`, `_capStreamSound`
- `DLS`, `dls`, `MID`, `mid`, followed by `MThd` and `MTrk` parser constants

The ARM firmware also retains source-path labels for the surrounding audio
framework:

- `ac_mixer.c`
- `af_mixer.c`
- `af_param.c`
- `af_tonegen.c`
- `ac_apfmngr.c`
- `dsp_manager_functions.c`
- `dsp_manager_state.c`
- `r_af_audio_codecs.c`

This supports the separation between ARM-side audio control, MegaStar module
loading, common mixing/tone generation, and the module-local MIDI process.

## ARM-to-DSP loader protocol

String analysis recovers the first reliable layer of the ARM-side loader state
machine:

`R_Req_AC_DSP_PreBoot -> module download -> R_Req_AC_DSP_PostBoot`

The ARM firmware also exposes these loader states and failure paths:

- `DSP LOADER: ERROR DSP Version responder TIMED OUT!`
- `DSP_Loader: First R_Req_AC_DSP_PreBoot didn't work!`
- `No DSP code found in Flash/failed to download code, releasing MegaStar`
- `DSP Loader: Interrupts disabled`
- `DSP_RM/TryReplace: Failed to replace module!`
- `DSP_RM: DSP_LOADER_ABORT_RESPONSE_TIMEOUT`

The resource manager has slots for phone audio, TTY, PTT, MIDI/PCM/ASR, MP3,
AMR/ASR, AAC, WMA, video, and `MIDI_72`.

Additional retained source labels identify the ARM bridge files:

- `r_dsp_rm_dsp_loader.c`
- `r_af_audio_codecs.c`
- `r_avcr_audio_codecs.c`
- `r_mma_audio_codecs.c`
- `r_sed_audio_codecs.c`

Recovering numeric primitive IDs and message layouts still requires
instruction-level ARM analysis. The MAIN image contains valid ARM and Thumb
code but uses multiple runtime address regions, so raw file offsets must not
be treated as a single linear VMA map.

The ARM-side static probe is reproducible with:

```sh
python3 tools/w200_arm_probe.py \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/W200_R4JA011_MAIN_GENERIC_ME_RED53.mbn \
  --out phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_ARM_PROBE.json
```

Current ARM probe result:

- `CSynthesizer_imp.c`, `CPlayerManager_imp.c`, and `CCustomMidiBank_imp.c`
  occur once each inside a retained component/source-name catalog.
- No aligned exact pointer references to those catalog strings were found
  under the tested VMA bases.
- `AudioControl has received primitive %d from 0x%x unexpectedly.` has aligned
  pointer-looking matches, but the matches land in string/name tables, not in
  ARM or Thumb code. They are not sufficient to recover primitive IDs.
- The probe records 39 diagnostic strings containing `primitive`. These are a
  useful search grid for the next ARM pass, but they span many OSE processes
  and must not be interpreted as Java/MMAPI primitive IDs by proximity alone.
- `_capMIDI`, `_capCMIDI`, `_capCompoundSound`, and `_capStreamSound` are in
  the Flash/SWF capability string area, not a confirmed Java audio dispatch
  path.

The second ARM pass adds a coarse code-island map and immediate extractor.
The range `0x1ef000..0x1f02c8` is confirmed as ARM code, but it is a numeric
helper island: the apparent constants `0x7e`, `0x9e`, `0x9f`, `0xfe`, and
`0xff` are used in integer/float conversion style helpers. They are rejected
as audio primitive IDs.

The nearby `0x1f5370..0x1f54d0` area is trace/source metadata for
`ac_mixer.c` and `ac_vincenne2.c`:

- `AUC: Open Audio: Open Channels %d, Freq: %d`
- `AUC: Open Audio: ID: %d. Frequency: %d. Prio: %d Channels: ...`
- `AudioControl has received primitive %d from 0x%x unexpectedly.`
- `AUC ERROR: Timed out while waiting on R_REQ_AC_DSP_POSTBOOT`

The immediate extractor records these ranges in
`W200_R4JA011_ARM_PROBE.json` under `audio_adjacent_code_ranges` and
`audio_adjacent_arm_immediates`. The current result is useful as a reject
list: no numeric primitive IDs are confirmed in the audio-adjacent ARM
helper/trace island.

The process-name list contains `AF_Process`, `AudioControl_Process`,
`VIE_EncoderProcess`, and `CameraServer_Process` around MAIN offset
`0x6e3a9c..0x6e3af7`. No aligned direct pointer references to
`AudioControl_Process` were found under the tested bases, so the OSE process
table likely uses an indirect symbol encoding or a generated registry format
rather than direct string pointers.

The follow-up process-registry probe is reproducible with:

```sh
python3 tools/w200_process_registry_probe.py \
  phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/W200_R4JA011_MAIN_GENERIC_ME_RED53.mbn \
  --out phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/w200-audio-analysis/deep-scan/W200_R4JA011_PROCESS_REGISTRY.json
```

The complete parsed process-name block starts at `0x6e371c` and currently
contains 87 strings. The audio-adjacent entries are:

- `AVCR_Process`: index 55, relative `0x373`, offset `0x6e3a8f`
- `AF_Process`: index 56, relative `0x380`, offset `0x6e3a9c`
- `AudioControl_Process`: index 57, relative `0x38b`, offset `0x6e3aa7`
- `VIE_EncoderProcess`: index 60, relative `0x3c8`, offset `0x6e3ae4`
- `CameraServer_Process`: index 61, relative `0x3db`, offset `0x6e3af7`
- `DSPIF_INT_IntProcess`: index 75, relative `0x4c7`, offset `0x6e3be3`
- `LoadModuleStarterProcess`: index 84, relative `0x552`, offset `0x6e3c6e`

Important correction: an earlier partial string-block view made
`AudioControl_Process` look like relative `0x2a7`. The full parser shows that
the useful process-relative token is `0x38b`.

`AudioControl_Process` relative token `0x38b` appears in both generated symbol
dictionaries and Thumb literal pools. The dense dictionary runs around
`0x2ba0f0..0x2ba120` and `0x2bb4f4..0x2bb548` contain adjacent process-relative
values such as `DebugMux_Process`, `AVCR_Process`, `AF_Process`, and
`AudioControl_Process`; they are data tables, not executable handlers.

Literal-pool hits near Thumb code were found at:

- `0x7c8248` -> Thumb prologue `0x7c8250`
- `0x7f835c` -> Thumb prologue `0x7f836c`
- `0x92a810` -> Thumb prologue `0x92a824`
- `0x97f720` -> Thumb prologue `0x97f72a`
- `0xa3bb4c` -> Thumb prologue `0xa3bb58`
- `0x1219cc4` -> Thumb prologue `0x1219ccc`
- `0x1296884` -> Thumb prologue `0x1296888`
- `0x129721c` -> Thumb prologue `0x129722c`
- `0x1297940` -> Thumb prologue `0x1297948`
- `0x129abac` -> Thumb prologue `0x129abb8`

Manual Thumb disassembly rejects several of these as synth-entry candidates:

- `0x97f6d8` computes four bounded dimensions with divisors `60`, `900`, and
  `1000`; it looks like generic geometry/grid arithmetic.
- `0x1219ccc` allocates buffers and parses packed decimal fields; it is not
  an audio render or DSP primitive path.
- `0x1296888`, `0x129722c`, `0x1297948`, and `0x129abb8` are resource/index
  state helpers. In this region `0x38b` is used as a status literal near
  related literals such as `0x385`, `0x38a`, and `0x38e`, not as an
  `AudioControl_Process` reference.
- `0xa3bb58` serializes a multi-field object through helper calls around
  `0xa36ecc..0xa3738c`; it is still unconfirmed and should be treated as a
  low-priority generic codec/serializer candidate.

The strongest remaining AudioControl object-method cluster is
`0x7c8250..0x7c8514`:

- `0x7c8250` reads an object pointer at `[r0+4]`, reads a channel/id byte at
  object offset `0x1c`, calls `0x9bb124`, and maps returned byte values
  `1..4` into an output byte. It then calls `0x7c7aec` with a source literal.
- `0x7c82dc` checks that the object has an active byte at offset `0x1c` and
  an allocated channel/session pointer at offset `0x20`. If no session exists
  it calls through a vtable at `[obj+0x18]->vtable[0x10]`, stores the returned
  pointer at `[obj+0x20]`, prepares a stack IPC record via `0x9d4e58`, and
  sends a payload through `0x9ba82c`.
- `0x7c83c8` and `0x7c8488` are related send methods. They validate input
  pointers, require the active byte and session pointer, call through the
  vtable at `[obj+0x14]->vtable[0x24]`, and then call `0x9baa78` or
  `0x9bac28` to construct payloads.
- On failure these methods call the release vtable slot at
  `[obj+0x18]->vtable[0x2c]` and clear `[obj+0x20]`.

This cluster is the best current ARM-side candidate for the AudioControl
session/message wrapper. It still does not expose the Java MIDI primitive IDs
directly, but it gives a concrete object layout: active byte at `0x1c`, vtable
or transport pointer at `0x14`, allocator/releaser at `0x18`, and active
session pointer at `0x20`.

`0x7f836c` converts or validates compact message buffers. It accepts message
type `1` with a 16-byte output and type `2` with a 28-byte output, writes
record IDs `2` or `3`, copies a 16-bit field from input offset `0x14`, and
copies either four scalar bytes or a 16-byte tail. Nearby `0x7f8508` dispatches
records with IDs `3..9` through vtable slots at offsets `0x10`, `0x14`, etc.
This is probably a message codec/bridge helper, not the synthesizer itself.

`0x92a824` and its large continuation `0x92a8e4..0x92ac2c` are lower priority.
They compare signed byte identities through helpers around `0x924078`,
`0x927188`, and format many trace/status messages through `0xb20064`. The
literal `0x38b` here is adjacent to other small status literals and is not a
confirmed `AudioControl_Process` reference.

The AudioControl methods call a reusable OSE-message packing layer around
`0x9ba7b8..0x9bb21c`. This layer is now a confirmed bridge target:

- `0xac9818` allocates or prepares an outgoing signal/message. It is called
  with a type-like literal in `r1` and a size-like literal in `r0`.
- `0x9d4f40` copies sender/session metadata from `[wrapper+4]` into the
  allocated message.
- `0xac9820` sends or queues the message using the global pointer loaded from
  the literal at `0x9ba850`/`0x9baa7c`/similar.
- If the wrapper byte `[wrapper+0]` is `1`, the packer returns early after
  enqueueing; otherwise it calls `0xac731c`, stores that result on the stack,
  validates the built signal by magic at `[msg+0]`, and eventually releases
  with `0xac7314`.

Recovered message shapes in this layer:

- Type `0x5e44`, size `0x107c`: session/open-style request. It can carry an
  8-byte header at `[+8]`, a byte flag at `[+0x10]`, and a `0x1068`-byte
  block at `[+0x12]`. The response validator expects `0x5e5b` and extracts a
  status byte, the `0x1068`-byte block, and trailing scalar fields. This is
  the strongest recovered candidate for the MIDI/CMIDI synthesizer
  open/configure primitive.
- Type `0x5e45`, size `0x1074`: channel/session command with byte `[+8] =
  channel/id` and an optional 1-byte payload at `[+0xa]`.
- Type `0x5e46`, size `0x1074`: channel/session command with the same large
  message class as `0x5e45`; it is called from the earlier AudioControl
  session cluster and is a likely program/state setter.
- Type `0x5e47`, size `0x1074`: adjacent large channel command. No direct
  high-confidence AudioControl caller has been named yet.
- Type `0x5e48`, size `0x1270`: large stream/note command called from
  `0x7c5a64` and `0x7c5f9c`. Its larger payload class matches MIDI/CMIDI
  event buffer transfer better than the small scalar controls.
- Type `0x5e49`, size `0x1270`: sibling large stream/note command called from
  `0x7c5924` and `0x7c5d52`.
- Type `0x5e4a`, size `0x1074`: adjacent channel command; currently mapped as
  the best note-off/flush sibling candidate by sequence position.
- Type `0x5e4b`, size `0x1074`: byte `[+8] = channel/id`, byte `[+9] =
  sub-id`, optional 5-byte payload copied to `[+0xa]`, then sent.
- Type `0x5e4c`, size `0x000c`: byte `[+8] = channel/id`, no large payload.
- Type `0x5e4d`, size `0x1074`: large channel command in the same family as
  `0x5e4b` and `0x5e4e`.
- Type `0x5e4e`, size `0x1074`: byte `[+8] = channel/id`, optional 1-byte
  payload copied to `[+0xa]`, extra byte stored at `[+0x1072]`.
- Type `0x5e4f`, size `0x1074`: large channel command that is entered through
  a tail-style call from `0x7c84e6 -> 0x9bac28`.
- Type `0x5e50`, size `0x1074`: byte `[+8] = channel/id`, optional 3-byte
  payload copied to `[+0xa]`, extra byte stored at `[+0x1072]`.
- Type `0x5e51`, size `0x1274`: large stream command with an extended tail
  block, sibling to `0x5e52`.
- Type `0x5e52`, size `0x1274`: byte `[+8] = channel/id`, optional 3-byte
  payload copied to `[+0xa]`, extra byte stored at `[+0x1270]`, and an
  additional bounded block (`strlen < 0x80`) copied to `[+0x1072]`.
- Type `0x5e53`, size `0x1074`: large channel command.
- Type `0x5e54`, size `0x1074`: large channel/status command.
- Type `0x5e55`, size `0x000c`: small scalar transport command.
- Type `0x5e56`, size `0x000c`: small scalar transport command.
- Type `0x5e57`, size `0x000c`: stores a 32-bit value at `[+8]`.
- Type `0x5e58`, size `0x0008`: request/response helper used by
  `0x7c8250`; it returns a byte result through the caller-provided stack
  buffer.
- Type `0x5e59`, size `0x000c`: small scalar AudioControl command, called
  from `0x7c7612`/`0x7c7d66` through the `0x9bb1b4` packer.
- Type `0x5e5a`, size `0x000c`: small scalar AudioControl command, called
  from `0x7c7dfc` through the `0x9bb200` packer.

Current callsite map for the confirmed packer family:

| Signal | Packer start | Size | Confirmed callers / entry callers | Working role |
| ---: | ---: | ---: | --- | --- |
| `0x5e44` | `0x9ba130` | `0x107c` | tail response at `0x9ba210` | MIDI/CMIDI open/configure |
| `0x5e45` | `0x9ba270` | `0x1074` | tail callers around `0x9ba2e8` | channel/session command |
| `0x5e46` | `0x9ba334` | `0x1074` | `0x7c6094 -> 0x9ba338` | program/state setter candidate |
| `0x5e47` | `0x9ba528` | `0x1074` | no direct caller named | channel command |
| `0x5e48` | `0x9ba430` | `0x1270` | `0x7c5a64`, `0x7c5f9c` | large MIDI event transfer candidate |
| `0x5e49` | `0x9ba5e0` | `0x1270` | `0x7c5924`, `0x7c5d52` | large MIDI event transfer candidate |
| `0x5e4a` | `0x9ba6d8` | `0x1074` | tail callers around `0x9ba6c0` | note-off/flush sibling candidate |
| `0x5e4b` | `0x9ba7b8` | `0x1074` | tail callers around `0x9ba760`/`0x9ba788` | control/buffer command |
| `0x5e4c` | `0x9ba880` | `0x000c` | `0x7c8352 -> 0x9ba82c` | small AudioControl send |
| `0x5e4d` | `0x9ba924` | `0x1074` | no direct caller named | stream/status command |
| `0x5e4e` | `0x9ba9e4` | `0x1074` | tail callers around `0x9baa78` | stream/status command |
| `0x5e4f` | `0x9bac60` | `0x1074` | `0x7c84e6 -> 0x9bac28` | stream/status command |
| `0x5e50` | `0x9baaac` | `0x1074` | tail callers around `0x9baa78` | stream/status command |
| `0x5e51` | `0x9bad20` | `0x1274` | no direct caller named | extended stream command |
| `0x5e52` | `0x9bab70` | `0x1274` | tail callers around `0x9bac28` | extended stream command |
| `0x5e53` | `0x9bae10` | `0x1074` | no direct caller named | channel command |
| `0x5e54` | `0x9baed4` | `0x1074` | `0x7c77c8 -> 0x9baea0` | status/channel command |
| `0x5e55` | `0x9baf98` | `0x000c` | `0x7c74e2 -> 0x9baf20` | transport scalar |
| `0x5e56` | `0x9bb018` | `0x000c` | no direct caller named | transport scalar |
| `0x5e57` | `0x9bb0a0` | `0x000c` | `0x7c4646`, `0x7c495a`, `0x7c6490`, `0x7c870a` | scalar setter |
| `0x5e58` | `0x9bb124` | `0x0008` | `0x7c826a` | AudioControl state query |
| `0x5e59` | `0x9bb1a4` | `0x000c` | `0x7c7612`, `0x7c7d66` | scalar setter |
| `0x5e5a` | `0x9bb21c` | `0x000c` | `0x7c7dfc` | scalar setter |

The repeated message-type literals continue at least through `0x5e70`. These
are strong primitive-family candidates, but they are OSE signal IDs for the
AudioControl bridge rather than confirmed Java MMAPI method IDs. The next
reverse-engineering step is to map which `0x5eXX` signal corresponds to
MIDI/CMIDI note, program, bank, volume, and render/session commands.

The phoneME port now models the confirmed W200 constraints directly:
`22050 Hz`, `72` voices, channel/program/control/note state, percussive
channel handling, and native PCM generation. This replaces the TinySoundFont
renderer while keeping the existing MIDI parser as the Java/MMAPI input layer.

The supplied MAIN and FS images contain no standalone RIFF-DLS or SF2 bank:

- no `sfbk`
- no `DLS ` RIFF form
- no `wvpl`, `ptbl`, `lins`, or `lrgn`

Instrument data is therefore encoded in proprietary data/code, loaded at
runtime through `CCustomMidiBank`, or both.

## Filesystem findings

The FS image contains 31 valid standalone SMF MIDI files and five embedded
Java archive central directories.

Two game archives contain `sound.bin`. These are game-local WAV bundles, not
the system MIDI bank:

- one bundle contains an `8000 Hz` mono IMA ADPCM WAV-like record
- the `quadra_pop` bundle contains four `8000 Hz`, mono, 8-bit PCM WAV records

The `(c) MicroSound 2004` strings occur inside SMF metadata. They identify
authored MIDI resources, not a synthesizer bank.

## Porting boundary

The current phoneME backend can immediately use the confirmed 72-voice
profile and supported DSP rates as compatibility constraints. The pitch
table is useful when reproducing the shared W200 tone generator.

A bit-identical W200 renderer still requires:

1. recovering C55x code/data boundaries and a control-flow graph with the
   open-source raw disassembler, TI C5500 CGT, or both
2. recovering the ARM-to-DSP numeric primitives used by `CSynthesizer` and
   `CCustomMidiBank`
3. extracting or reconstructing instrument data passed at runtime
4. validating output against PCM captures from a physical W200

The extracted module and generated `deep-scan.json` are the starting corpus
for that work.

## Primary references

- TI C55x CPU Reference Guide:
  <https://www.ti.com/lit/pdf/swpu073>
- TI C55x Mnemonic Instruction Set Reference Guide:
  <https://www.ti.com/lit/pdf/swpu067>
- TI C55x Algebraic Instruction Set Reference Guide:
  <https://www.ti.com/lit/pdf/swpu068>
- TI C5500 Code Generation Tools archive:
  <https://software-dl.ti.com/codegen/non-esd/downloads/download_archive.htm#C5500>
- TMS320C55x binutils port:
  <https://sourceforge.net/projects/c55x-binutils/>
