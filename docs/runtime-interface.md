# Runtime interface (launcher ↔ runtime)

The runtime (`runtime/`, built into `runMidlet` / `runMidlet.exe`) knows
nothing about game folders, `.jad`/`.jam` files or settings screens. A
launcher (`launchers/<target>/`) turns a game into one command line plus a
set of environment variables, starts `runMidlet` with `bin/` as the current
folder and waits for it. This file is the complete list of what a launcher
can pass; the native side reads every variable through `PhoneMEConfig()`
(`runtime/midp/src/core/phoneme_config/`), whose table must match the
lists below (`tools/check-runtime-interface.py` checks it).

Existing launchers: `launchers/funkey/pm-launch.c` (FunKey S / RG Nano),
`launchers/linux/run.sh` (desktop Linux), `launchers/windows/launcher.c`
(Windows).

## Command line

MIDP game:

    runMidlet [VM options] -classpathext <game.jar> internal <MIDlet class>

i-appli (DoJa) game:

    runMidlet [VM options] =MaxMethodToCompile3000 \
        -Ddoja.app.class=<AppClass> -Ddoja.app.param=<AppParam> \
        -Ddoja.app.ver=<AppVer> -Dmicroedition.encoding=SJIS \
        -Ddoja.scratchpad.path=<game.sp> -Ddoja.spsize=<SPsize> \
        -Ddoja.jam.hex=<the .jam file as hex> -Ddoja.pictures=<folder> \
        -classpathext <game.jar><sep><runtime>/lib/doja-support.jar \
        internal com.funkey.doja.DoJaMIDlet

- `<sep>` is the platform's path separator: `:` on Linux, `;` on Windows.
- Paths must be ASCII on Windows (the VM opens files through the ANSI C
  library): the Windows launcher passes 8.3 short names or copies.
- The MIDlet class is the third field of `MIDlet-1` (`.jad` or manifest).
  `getAppProperty()` reads a `.jad` with the jar's name next to the jar, so
  a launcher makes one from the manifest when the game has none.
- `doja.jam.hex` carries the whole `.jam` (Shift_JIS); the separate
  `doja.app.*` values are only fallbacks, so non-ASCII ones can be left
  empty.

VM options (CLDC flags, `+Name` / `-Name` / `=NameValue`):

| Option | Meaning |
|---|---|
| `-UseVerifier` | skip bytecode verification (launchers pass it by default: modified games often fail it but run) |
| `-UseCompiler` | no JIT, interpreter only |
| `=MaxMethodToCompile3000` | JIT: larger methods (used for i-appli) |

Other Java system properties read by the DoJa layer (all optional):
`doja.draw.area` and `doja.profile.ver` (fallbacks for the `.jam` keys),
`doja.source.url` (fallback for `PackageURL`), `doja.audio.ports` (number
of audio channels), `doja.trace=1`, `doja.trace3d=1`, `doja.c3dtrace`
(set), `doja.stats=1` (logging).

## Exit code

| Code | Meaning |
|---|---|
| 0 or 1 | normal end (1 = `MIDP_SHUTDOWN_STATUS`: window closed, game quit, "Exit" in the runtime menu) |
| -1 / 255 | the suite could not be run |
| -2 / 254 | the suite was not found |
| other | crash (a signal on Linux, an exception code on Windows) |

Errors inside a running game are shown by the runtime itself (its error
screen), so launchers show a message only for the other codes.

## Environment variables

"set" means: any value, also empty, switches it on.

### Files and folders

| Variable | Meaning |
|---|---|
| `MIDP_HOME` | runtime folder with `lib/` and `appdb/` |
| `PHONEME_RMS_HOME` | folder for this game's record stores (saves); created if missing |
| `PHONEME_CONFIG_PATH` | file the runtime menu saves its choices to (see below) |
| `PHONEME_DEBUG_CONFIG` | file with debug switches shared with the launcher (see below) |
| `PHONEME_RESTART_FLAG` | file the runtime creates when the user picks Restart in its menu |
| `PHONEME_SOUNDBANK` | sound bank file for the MIDI synthesizer |

### Memory

| Variable | Meaning |
|---|---|
| `PHONEME_HEAP_MB` | Java heap in MB, 1–256 (Windows, Linux, web: 64; FunKey: 16, i-appli 32) |
| `PHONEME_HEAP_KB` | Java heap in KB (wins over `PHONEME_HEAP_MB`) |

### Screen

| Variable | Meaning |
|---|---|
| `PHONEME_LCD_SIZE` | screen size the game sees, `WxH` (default 240x240; i-appli: the `.jam` `DrawArea`) |
| `PHONEME_SOURCE_SIZE` | part of the game frame that is shown, `WxH` or `full` |
| `PHONEME_SCALE_MODE` | `auto`, `fit`, `fill`, `crop`, `center`, `zoom` or `integer` |
| `PHONEME_SCALE_FILTER` | `sharp` (default), `smooth`, `pixel` or `scale2x` |
| `PHONEME_SCALE_RATIO` | zoom mode: `250`..`4000` (1/1000) or `25`..`400` (percent) |
| `PHONEME_PAN` | zoom/crop: visible part, `x,y` in -1000..1000 |
| `PHONEME_FPS_LIMIT` | `off`, `20`, `25`, `30` (default), `50` or `60` |
| `PHONEME_SHOW_FPS` | set: FPS counter on the screen |
| `J2ME_GP2X_REVERSE` | set: rotate the screen by 90 degrees |

### Desktop window

Without any of these the runtime opens a 240x240 screen (the handheld
behaviour). With them it opens a resizable window; the game frame is
scaled from its own size straight to the window size.

| Variable | Meaning |
|---|---|
| `PHONEME_DESKTOP` | `1`: desktop window even at scale 1 |
| `PHONEME_WINDOW_SCALE` | window = game screen times `1`..`8` (shrunk to fit the monitor) |
| `PHONEME_WINDOW_SIZE` | window size `WxH` (wins over the scale) |
| `PHONEME_FULLSCREEN` | `1`: start in full screen (F11 / Alt+Enter switch) |
| `PHONEME_WINDOW_TITLE` | window title (UTF-8) |

### Keys

Phone key codes (MIDP `keyCode` values the game receives): up -1, down -2,
left -3, right -4, fire -5, soft keys -6/-7 (Motorola -21/-22, older
Motorola 21/22, Siemens -1/-4), clear -8, call -10, end -11, game A–D
-13..-16, `'0'`..`'9'` 48..57, `*` 42, `#` 35.

| Variable | Meaning |
|---|---|
| `PHONEME_PC_KEYS` | PC keyboard map, `sym=code;sym=code;...`: SDL 1.2 key symbol = phone key code; code 1000 opens the runtime menu, 1001 switches full screen. Replaces the built-in keyboard table (arrows, Enter/Space, F1/F2, digits, Esc = menu, F11 = full screen) |
| `PHONEME_ENABLE_GP2X_KEYS` | set: handheld button map (FunKey / RG Nano) |
| `PHONEME_KEY_PROFILE` | handheld key profile: `universal`, `doja`, `nokia-s40`, `sony-ericsson`, `samsung`, `lg`, `motorola`, `soft-21-22`, `motorola-soft`, `siemens-soft` |
| `PHONEME_IGNORE_JOYSTICK` | set: ignore joystick events |
| `J2ME_GP2X_JOYU`, `J2ME_GP2X_JOYUL`, `J2ME_GP2X_JOYL`, `J2ME_GP2X_JOYDL`, `J2ME_GP2X_JOYD`, `J2ME_GP2X_JOYDR`, `J2ME_GP2X_JOYR`, `J2ME_GP2X_JOYUR`, `J2ME_GP2X_JOYC` | handheld stick directions / click → phone key code |
| `J2ME_GP2X_START`, `J2ME_GP2X_SELECT`, `J2ME_GP2X_LEFT`, `J2ME_GP2X_RIGHT`, `J2ME_GP2X_BUTA`, `J2ME_GP2X_BUTB`, `J2ME_GP2X_BUTX`, `J2ME_GP2X_BUTY`, `J2ME_GP2X_VOLU`, `J2ME_GP2X_VOLD` | handheld buttons → phone key code |

### Sound

| Variable | Meaning |
|---|---|
| `PHONEME_ENABLE_AUDIO` | set: sound on |
| `PHONEME_SOUND_VOLUME` | master volume `0`..`100` |
| `PHONEME_SYNTH` | MIDI synthesizer: `hybrid` (default), `wavetable` or `fm` |
| `PHONEME_AUDIO_QUALITY` | `low` or `high` (default: `low` in the FunKey build, `high` elsewhere). High: sampled sounds and the output (44100 Hz) go through a windowed-sinc resampler, the synthesizer presets are level-matched |
| `PHONEME_MIDI_GAIN` | MIDI synthesizer gain `50`..`100` (dB below 100; default 90, at the high quality level 93 for the wavetable and 84 for the other presets) |
| `PHONEME_AUDIO_BUFFER` | audio buffer in samples, `128`..`8192` (default 1024) |
| `PHONEME_AUDIO_SUM` | `1`/`0`: mix stereo to mono (default: from `/etc/asound.conf`) |
| `PHONEME_MLD_EXACT` | `1`: exact (slower) MLD (i-mode melody) synthesis |
| `PHONEME_MLD_MIDI` | `1`: play MLD melodies through the MIDI synthesizer |

### Logs and debugging

The runtime writes its log to stdout/stderr; launchers redirect it to a
file (or a console window).

| Variable | Meaning |
|---|---|
| `PHONEME_DISPLAY_DEBUG` | set: log display and scaling decisions |
| `PHONEME_KEY_DEBUG` | set: log keys |
| `PHONEME_EVENT_DEBUG` | set: log events sent to the game |
| `PHONEME_AUDIO_TRACE` | set: log the audio device setup and every call a game makes on its players (`snd: <ms> start #3 ...`) |
| `PHONEME_PROFILE` | `1`: sampling CPU profiler, report in the log (Linux) |
| `PHONEME_PROFILE_SECONDS` | profiler report period in seconds (default 10) |

## Files shared with the launcher

Every launcher handles these three files the same way, so the runtime menu
works alike everywhere: FunKey (`/mnt/FunKey/.pm/debug.cfg`, `.pm/keybinds/<game>.cfg`), Linux
(`~/.local/share/phoneme-j2me`: `debug.cfg`, `run/<game>/game.cfg`),
Windows (`data\debug.cfg`, `data\run\<game>\game.cfg`, read back into
its settings when the game ends) and the browser (IndexedDB, `/cfg`).

`PHONEME_CONFIG_PATH` — written by the runtime menu ("Display",
"Controls", "Sound"), `KEY=VALUE` lines; a launcher reads it at the next
start and turns it into the variables above: `SCALE` (zoom preset id),
`VIEW` (source size id), `DISPLAY_MODE`, `FILTER`, `PAN_X`, `PAN_Y`,
`FPS_LIMIT`, `VOLUME`, `SYNTH`, `PROFILE` (key profile id) and the
handheld binds `UP`, `DOWN`, `LEFT`, `RIGHT`, `A`, `B`, `X`, `Y`, `L1`,
`R1`, `START`, `SELECT` (phone key codes).

`PHONEME_DEBUG_CONFIG` — `KEY=0/1` lines: `LOG`, `KEYS`, `EVENTS`,
`DISPLAY`, `AUDIO`, `LAUNCH`, `PROFILE`, `FPS`. The runtime menu
("Debug") edits it; the launcher applies it at the next start (log file
on/off, `PHONEME_KEY_DEBUG`, ...). Only `FPS` takes effect at once.

`PHONEME_RESTART_FLAG` — when the user picks "Restart" in the runtime
menu, the runtime saves `PHONEME_CONFIG_PATH`, creates this file and
exits; a launcher that finds it deletes it and starts the game again.
Without the variable the menu has no Restart entry.

## Adding a setting

1. Read it in the runtime with `PhoneMEConfig("NAME")` (never `getenv`).
2. Add it to the table in `runtime/midp/src/core/phoneme_config/reference/native/phoneme_config.c`.
3. Document it here.
4. `python3 tools/check-runtime-interface.py` must pass.
