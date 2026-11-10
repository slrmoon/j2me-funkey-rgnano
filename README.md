# phoneME J2ME for FunKey S / RG Nano

> If a specific game doesn't work, reach out on Telegram: [@slrmo0n](https://t.me/slrmo0n)

A Java ME runtime for the **FunKey S** and **Anbernic RG Nano** handhelds:
phone games (MIDP 2.0 / CLDC 1.1) and Japanese i-mode games (i-appli,
NTT DoCoMo DoJa), packed as one `pm.opk` with a game browser.

It is based on the phoneME VM from
<https://github.com/j2me-preservation/phoneME-GP2X-SDL>, with a new screen
scaler, sound engine, 3D renderer and DoJa runtime written for this device.
The same runtime also builds for desktop Linux, Windows and the browser
(see [Other platforms](#other-platforms)), mostly to test games and to play
them on a PC.

## Screenshots

### Launcher

<p float="left">
  <img src="screenshots/IMG_0018.PNG" width="240" />
  <img src="screenshots/IMG_0033.PNG" width="240" />
  <img src="screenshots/IMG_0035.PNG" width="240" />
  <img src="screenshots/IMG_0024.PNG" width="240" />
</p>

### Games

<p float="left">
  <img src="screenshots/IMG_0012.PNG" width="240" />
  <img src="screenshots/IMG_0013.PNG" width="240" />
  <img src="screenshots/IMG_0023.PNG" width="240" />
</p>

### i-mode games

Rockman The Puzzle Battle, THE Battle Cats (にゃんこ大戦争), Bomberman '08,
Bio Miracle Bokutte Upa, Battle City, Flame Firefighter, Cafe Far Niente,
CR Evangelion 4 (pachinko), Tetra. See [i-mode games (i-appli)](#i-mode-games-i-appli).

<p float="left">
  <img src="screenshots/iappli_rockman.png" width="240" />
  <img src="screenshots/iappli_battlecats.png" width="240" />
  <img src="screenshots/iappli_bomberman.png" width="240" />
</p>

<p float="left">
  <img src="screenshots/iappli_upa.png" width="240" />
  <img src="screenshots/iappli_battlecity.png" width="240" />
  <img src="screenshots/iappli_firefighter.png" width="240" />
</p>

<p float="left">
  <img src="screenshots/iappli_cafe.png" width="240" />
  <img src="screenshots/iappli_eva.png" width="240" />
  <img src="screenshots/iappli_tetra.png" width="240" />
</p>

## Installation (FunKey S / RG Nano)

1. Download `pm.opk` from the [latest release](https://github.com/slrmoon/j2me-funkey-rgnano/releases/latest)
2. Place it in the `emulators` folder on your FunKey/RG Nano SD card
3. Create a `java` folder in the root of the SD card
4. Put your `.jar` files into that `java/` folder
5. Japanese i-mode games (**i-appli**) go into the same `java/` folder: keep
   the `.jam` next to the `.jar` (same name, or the only `.jam` in its
   folder), optionally with the `.sp` scratchpad. They show up in the list
   with an `i` mark. See [i-mode games (i-appli)](#i-mode-games-i-appli).

In the game list: **D-pad up/down** moves, **◀▶** flips a page (the page
number is shown top right), **L1/R1** jump to the first/last game, **A** runs,
**SELECT** adds to favorites, **START** opens the game's settings, **Y** the
debug options. There is no limit on the number of games in a folder.

Saves (record stores) are kept in `/mnt/FunKey/.pm/rms/<game>/`; an i-appli
keeps its scratchpad as `<game>.sp` next to the `.jar`.

## Features

- **MIDP 2.0 / CLDC 1.1** runtime (phoneME, JIT for ARM) with a game browser
  launcher.
- **i-mode games (i-appli)** (DoJa 1.x–5.x) from the same launcher, with
  i-mode MLD music on a Yamaha MA-3 FM synthesizer and i-appli 3D.
  See [i-mode games (i-appli)](#i-mode-games-i-appli).
- **3D: JSR 184 (M3G)** on a software OpenGL ES 1.x renderer written for the
  FunKey (single Cortex-A7, RGB565): integer rasterizer with a NEON span
  loop, texture cache, fog, lighting, sprites, render-to-texture.
  **MascotCapsule Micro3D v3** (`com.mascotcapsule.micro3d.v3`) on the same
  renderer. See [3D (M3G)](#3d-m3g).
- **Sound: MMAPI (JSR 135)** on a new audio engine with the handset MIDI synth
  (Sonivox EAS), all common phone formats and the vendor sound APIs.
  See [Sound](#sound).
- **Scaling** for every phone resolution, with sharp / smooth filtering and a
  movable view for cropped modes. See [Scaling](#scaling).
- **In-game menu** (Power button) that freezes the game while open: display,
  controls, sound and debug settings per game, restart, exit.
- **Debug options** in the launcher and the in-game menu: log file, detailed
  logs, FPS counter, CPU profiler and an on-device log viewer.
  See [Debugging](#debugging).
- **Error screens** instead of frozen black/white screens: when a game thread
  dies (OutOfMemoryError, a missing class, an exception in the game loop) and
  the game stops drawing, a "Game error" screen names the error (A: restart,
  B: exit); a game that crashes the runtime gets a "Game crashed" screen in
  the launcher with a shortcut to the log.
- **Exact timing**: `Thread.sleep()` lasts what the game asks, so games that
  pace themselves (most Gameloft titles run a fixed 15 fps step) run at the
  speed they were made for.

## In-game menu

Press **Power** during a game (**Esc** on a PC). The game and its sound are
**frozen** while the menu is open and continue exactly where they were when
it closes. The menu is translucent, so the frozen picture stays visible
behind it.

| Page | What is there |
|---|---|
| **Resume** | back to the game |
| **Display** | Screen, Zoom, Size, Filter, FPS, Adjust view — see [Scaling](#scaling) |
| **Controls** | key profile and per-button bindings |
| **Sound** | Volume, MIDI synth — see [Sound](#sound) |
| **Debug** | log switches and FPS counter — see [Debugging](#debugging) |
| **Restart game** | quit and start the game again (applies a new Size and debug settings) |
| **Exit** | save and quit to the launcher |

D-pad moves and changes values, **A** selects, **B** goes back.
All settings are saved **per game** (`/mnt/FunKey/.pm/keybinds/<game>.cfg`)
and are also available from the launcher: press **START** on a game. The
desktop and browser versions keep the same files and apply them the same
way ([docs/runtime-interface.md](docs/runtime-interface.md)).

## Contents

- [Scaling](#scaling) · [Sound](#sound) · [i-mode games (i-appli)](#i-mode-games-i-appli) · [3D (M3G)](#3d-m3g) · [Debugging](#debugging)
- [Other platforms](#other-platforms): Linux, Windows, browser
- [Building](#building) · [Repository structure](#repository-structure) · [Known issues](#known-issues)

## Scaling

The FunKey screen is 240×240. J2ME games were made for many phone screens,
so the picture usually has to be scaled, cropped or both.

### 1. Size — the game's own resolution

Pick the screen size the game was made for. Everything else depends on it.
**Restart the game after changing it** (in-game menu → **Restart game**).

| Size | Typical phones |
|---|---|
| Auto | let the runtime guess from what the game draws |
| 128×128 | very old Nokia / Siemens |
| 128×160 | old Samsung / LG / Motorola |
| 176×208 | Nokia S60 |
| 176×220 | Sony Ericsson, Siemens, most "middle" phones |
| 208×208 | Nokia S60 square |
| Tall 240×320 | most later portrait phones |
| Wide 320×240 | landscape phones / games |
| Full buffer | the whole drawing buffer, no detection |

If the picture is stretched, cut off, too small or the UI is outside the
screen, the size is most likely wrong.

### 2. Screen — how the picture is placed

| Screen | What it does | Use it for |
|---|---|---|
| **Auto** | Picks one of the modes below by a rule (see next table) | a good default |
| **Fit all** | whole picture, proportions kept, black bars | text, HUD and menus must be fully visible |
| **Crop fill** | fills the screen, proportions kept, edges cut off | bars bother you; move the view with *Adjust view* |
| **Stretch** | forced 240×240, proportions lost | last resort |
| **Center** | 1:1, no scaling; larger games are cut, smaller get bars | games close to 240×240 |
| **Manual zoom** | your own zoom factor (see *Zoom*), around the area the game actually draws | fine tuning (launcher default) |
| **Integer** | largest whole multiple that fits (2×, 3×…), else 1:1 | perfectly even pixels for small games (≤120 px) |

**Auto** decides like this:

| Game size | Auto result |
|---|---|
| 240×240 | 1:1 |
| ≤ 120 px (a whole multiple fills ≥ 90% of the screen) | Integer |
| filling the screen cuts ≤ 16% of the picture (128×128, 176×208, 208×208) | Crop fill |
| anything else (176×220, 128×160, 240×320, 320×240) | Fit all |

### 3. Zoom

Only for **Manual zoom**. The presets depend on *Size* (for example
150–200% for 128×128, 100–120% for 176×208, "Safe 70%" … "Full width" for
240×320). In other modes the menu shows a hint that Zoom does nothing.

### 4. Filter — how pixels are scaled

| Filter | Look | Notes |
|---|---|---|
| **Sharp** (default) | crisp pixels of even width | at non-integer factors (e.g. 176→240 = 1.36×) each game pixel stays a clean block, only its edge is blended over one screen pixel — no "uneven pixel" shimmer when scrolling |
| **Smooth** | soft, bilinear | photos / pre-rendered art; text gets softer |
| **Pixel** | plain nearest neighbour (the old behaviour) | uneven pixel widths, lines may disappear when shrinking |

When the picture is **shrunk** (240×320 → 180×240), Sharp and Smooth average
the source pixels, so thin lines and small text are kept instead of dropped.

### 5. Adjust view — move the picture when it is cropped

In every mode that cuts something off (Crop fill, Auto when it crops,
Center/Integer with big games, Manual zoom) you can choose which part is
visible — for example to bring back a score line or soft-key labels.

Display → **Adjust view** shows the frozen game full screen with a hint bar:

- **D-pad** — move the picture (10% steps)
- **L1 / R1** — zoom in Manual zoom mode
- **B** — back to the center
- **A** (or Power) — done, the position is saved for this game

If the whole picture already fits, the hint says *"Whole picture fits:
nothing to move"*.

### 6. FPS

Frame limit (Off / 20 / 25 / 30 / 50 / 60). 30 is the default.

Most phone games keep their own pace (they measure time), but some simple
ones move one step per frame and were tuned for 15–20 FPS handsets: without
a limit they run too fast. The limit pauses the whole game, not just the
picture, so such games keep their normal speed.

- Game too fast → 20 or 25.
- Game keeps its own pace (speed does not change with the limit) → 30 or Off.

Frames are released on a fixed 1/FPS schedule: a slow frame is made up by
the following fast ones, so a game that can reach the limit really gets it
even when frames are uneven (typical for 3D). A lower limit does not make a
slow game faster.

### Quick recipes

| Game | Size | Screen | Filter |
|---|---|---|---|
| Nokia S40 128×128 | 128×128 | Auto / Crop fill | Sharp |
| S60 176×208 | 176×208 | Auto (crop) + Adjust view if the soft-key line is cut | Sharp |
| Sony Ericsson 176×220 | 176×220 | Auto (fit) or Crop fill + Adjust view | Sharp |
| Portrait 240×320 | Tall 240×320 | Fit all for menus/text, Crop fill + Adjust view for action | Sharp |
| Landscape 320×240 | Wide 320×240 | Fit all | Sharp |

## Sound

Sound is played by **fkaudio**, a small engine written for this device:
one 22 kHz mono mixer (the FunKey has one speaker), its own parsers for
every phone music format, and the **Sonivox EAS** synthesizer — the MIDI
engine that shipped on many handsets and in Android — for the authentic
phone sound.

**Formats** (detected from the data; the content type a game declares is
only a hint, games often get it wrong):

| Kind | Formats |
|---|---|
| Music | MIDI (SMF 0/1/2, RMID), SMAF / MMF (Yamaha, incl. ADPCM voices), iMelody, Nokia OTA / Smart Messaging, RTTTL, JSR 135 tone sequences, i-mode MLD (Yamaha MA-3 FM synth) |
| Sampled | WAV (8/16/24/32-bit, float, A-law, µ-law, IMA and MS ADPCM), AU, AMR-NB, MP3 |

**APIs**: MMAPI 1.1 (`Manager`, `Player`, `PlayerListener`, Volume / MIDI /
Tempo / Rate / Pitch / StopTime / Tone controls, `Manager.playTone`,
`device://tone`, `device://midi`) and the vendor APIs: Nokia
`com.nokia.mid.sound.Sound`, Samsung `com.samsung.util.AudioClip`, Siemens
`com.siemens.mp.game.Sound / Melody / MelodyComposer` and
`com.siemens.mp.media`, Motorola `com.motorola.midi.MidiPlayer`.

Playback details: instant start (nothing is pre-rendered), exact seeking,
gapless loops (also for MP3), many sounds at once, a soft limiter instead
of clipping.

**In-game menu → Sound**

- **Volume** — 0–100%.
- **MIDI synth** — the instrument set, applied immediately:
  - **Hybrid** (default) — FM + small wavetable at 22 kHz, the typical phone sound
  - **Wavetable** — sampled instruments, fuller
  - **FM** — pure FM, the oldest phones

Advanced (environment variables in `r.sh`):

| Variable | Meaning |
|---|---|
| `PHONEME_SYNTH` | `hybrid` / `wavetable` / `fm` (normally set from the menu) |
| `PHONEME_SOUNDBANK` | path to your own DLS or SF2 bank; `lib/soundbank.dls` / `.sf2` in the runtime folder is picked up automatically |
| `PHONEME_MIDI_GAIN` | MIDI loudness, 50–100 (default 90) |
| `PHONEME_AUDIO_BUFFER` | output buffer in samples (default 1024 ≈ 46 ms); lower = less latency, more risk of crackles |
| `PHONEME_AUDIO_TRACE` | print the audio device setup to the log |

## i-mode games (i-appli)

i-appli are the Java games of Japanese NTT DoCoMo i-mode phones (2001-2010s),
written for the DoJa profile (`com.nttdocomo.*`) instead of MIDP. A game is a
`.jar` (often without a manifest) plus a `.jam` descriptor (Shift-JIS text
with `AppClass`, `DrawArea`, `SPsize`...) and often a `.sp` scratchpad with
its data (many games download their data into it on first start; their
servers are gone, so keep the `.sp` you have).

- **Install:** put the `.jar`, `.jam` and (if you have it) `.sp` into `java/`.
  A `.jar` counts as i-appli when its `.jam` has the same name or is the only
  `.jam` in the folder and its `AppClass` is in the jar. The game reads and
  saves its scratchpad in place (`<game>.sp` next to the `.jar`, created when
  missing); nothing is copied elsewhere.
- **Screen:** the game's `DrawArea` (240x240 when missing) becomes the screen
  size, the usual scaling options apply. Keys use the `doja` profile
  (D-pad, A = select, soft keys, 0-9, `*`, `#`).
- **Runtime:** DoJa 1.x-5.x UI (Canvas with lock/unlock, Graphics incl.
  `opt.ui` render modes, Panel and components, Dialog, Sprite, ShortTimer,
  paletted images), MediaManager images (GIF/PNG/JPEG/BMP), scratchpad, file
  storage, `JarInflater`, Shift-JIS text with Japanese bitmap fonts
  (Shinonome).
- **3D:** MascotCapsule v3 (`opt.ui.j3d`: MBAC models, MTRA animations,
  textures, lighting, toon shading, primitives, command lists), OpenGL ES
  (`ui.ogl`) and the C3D API (`opt.ui.j3d2`), all on the M3G renderer.
  C3D figures and the DoJa 5 `ui.graphics3d` API are not done yet.
- **Sound:** i-mode **MLD** melodies are played by a C port of the Keitai Wiki
  MLD sequencer and Yamaha MA-3 FM synthesizer, with the fixes from openDoJa
  (presets, timing, repeats, cue-point loops, vibrato). By default it runs in a
  fast mode (half the chip rate, 16 voices) to keep the music in real time on
  the FunKey; `PHONEME_MLD_EXACT=1` gives the bit-exact output. SMAF, MIDI and
  WAV sounds go through the normal audio engine.
- **Tested** (with keys; a fuller log is in [docs/doja-status.md](docs/doja-status.md)):

  | Game | State |
  |---|---|
  | Rockman The Puzzle Battle | playable, music |
  | THE Battle Cats (にゃんこ大戦争) | menus, stage map, 3D battles |
  | FIFA World Cup 2006 | 3D matches (~19 fps on the FunKey) |
  | Bomberman '08 | menus, save slots |
  | Bio Miracle Bokutte Upa | playable |
  | Battle City (patched) | playable |
  | Flame Firefighter | playable |
  | Cafe Far Niente (offline) | menus, puzzle |
  | CR Evangelion 4 (pachinko, research mode) | playable (slow) |
  | Tetra | playable |
  | VIRUS | story scenes |
  | ( `Д`)MT!, 10 Seconds, Chobi Hige, Avatar Bakyun, BakeGyamon Quiz 2, Comic Elliot x Alice, BreakDown, 30 Second Rocket, Buta-san | start, menus or gameplay |
  | BATTLE GEAR D | C3D course preview and menus; the race not reached |
  | FF7 Dirge of Cerberus Lost Episode | menus |
  | games that need their server (online versions) | network is not available |

- **Not yet:** C3D figures, `ui.graphics3d`, ADPCM voice samples inside MLD,
  network (`HttpConnection`), text input (IME returns the initial text).
  Unimplemented calls are logged as `DoJa: unimplemented ...`.

## 3D (M3G)

JSR 184 games run on a software renderer made for this device (no GPU):
the Nokia M3G core on a custom OpenGL ES 1.x rasterizer that draws straight
into the 16-bit screen buffer. It supports textures (with a per-image cache
and mipmaps), lighting, fog, sprites, alpha blending, depth test, culling,
`.m3g` files, animation and rendering into images. Native objects are
released when the game drops them (finalizers), so long sessions do not
run out of memory.

Speed. The FunKey has one Cortex-A7 core and no GPU, so every pixel is
drawn by the CPU. The renderer is tuned for that:

- a span fast path for nearly every state (up to two textures, mipmaps,
  nearest/bilinear, all texture modes, fog, alpha blending, depth test),
  with a NEON loop that draws 4 pixels at a time for the simplest one;
- vertex cache, combined model-view-projection per draw, clip codes per
  vertex, off-screen triangles rejected early;
- built for Cortex-A7 (`-march=armv7ve -mcpu=cortex-a7 -mfpu=neon-vfpv4`,
  hardware divide).

In Rally Master Pro (240×320) these changes raised the frame rate in races
by about 1.5–2×. Heavy scenes are still CPU-bound: if a game is slow, use the
*Pixel* filter (cheapest scaling) and send a [profiler](#profiler-output)
report.

### MascotCapsule Micro3D v3

The MIDP MascotCapsule API (`com.mascotcapsule.micro3d.v3`: Graphics3D,
Figure, ActionTable, Texture, FigureLayout, Effect3D, Light, AffineTrans,
Vector3D, Util3D, including the deprecated names older games call) is built
in. It shares its engine (`com.funkey.mc3`) with the i-appli MascotCapsule
support: MBAC models v2–5 with bones and patterns, MTRA animations, 8-bit BMP
textures with the color key, lighting, toon shading, sphere maps,
semi-transparent / additive / subtractive materials, primitives, point sprites
and command lists, all drawn through the M3G renderer above.

- The 3D screen is the clip rectangle of the bound `Graphics`.
- `renderFigure`, `renderPrimitives` and `drawCommandList` are drawn at once
  and share one depth buffer until `flush()`; 2D drawn between them and
  `flush()` therefore ends up under the 3D instead of over it.
- Test: `tools/mc3-test` (generated model, animation and textures).

## Debugging

Debug options are **global** (the same for every game) and stored in
`/mnt/FunKey/.pm/debug.cfg`. The launcher and the in-game menu edit the same
file. On a PC the in-game menu (Esc) has the same Debug page; the Windows
launcher shows the options in its window.

### In the launcher — press **Y**

<p float="left">
  <img src="screenshots/debug_launcher.png" width="240" />
  <img src="screenshots/debug_log_viewer.png" width="240" />
</p>

**Left — Debug screen.** A or ◀▶ toggles an option; the two lines below the
list explain the selected one.

| Option | Default | What it writes |
|---|---|---|
| **Log file** | On | game output to `.pm/runmidlet.log`; the run before is kept as `runmidlet.prev.log`. Off: nothing is written to the SD card |
| **Key log** | Off | every button press and the phone key it maps to |
| **Event log** | Off | key events delivered to the game |
| **Display log** | Off | screen set-up and the chosen scaling layout |
| **Audio log** | Off | audio device rate, channels and buffer |
| **Launch log** | Off | all settings the launcher passes to the game |
| **Profiler** | Off | every 10 s: CPU load, time per area (Java, 3D, 2D, display, audio, GC…) and the top functions / Java methods — see below |
| **FPS counter** | Off | frames per second on screen (no log needed) |

The logs need **Log file** on; otherwise they show as `On (no log)`.
**Clear logs** deletes both log files.

**Right — View log**: the log of the last run, read on the device, opened at
the end. D-pad scrolls, ◀▶ by page, **L1/R1** jump to the start/end,
**X** switches to the previous run.

### Profiler output

With **Profiler** and **Log file** on, the log gets a report every 10 seconds:

```text
[profile] 10.0 s, CPU 97%, 980 samples
[profile] by area: Java (JIT) 41%, 3D (M3G) 27%, display output 9%, audio 8%, ...
[profile]  12.3%  ngl_sw_raster_tri_fast
[profile]   6.1%  e.a
...
```

`CPU` is how busy the processor was; the areas and the list below show where
that time went. Java methods appear as `class.method` (obfuscated names in most
games). Send this report when a game is slow — it shows what to optimize.

### In a game — Power → **Debug**

<p float="left">
  <img src="screenshots/debug_ingame_menu.png" width="240" />
  <img src="screenshots/debug_ingame_page.png" width="240" />
  <img src="screenshots/debug_fps_counter.png" width="240" />
</p>

**Left — Restart game** (main menu) quits and starts the game again with the
saved settings. Use it after changing the logs or **Size**.

**Middle — Debug page**: the same options as in the launcher. The log
options are read when the game starts, so the yellow reminder stays until you
restart (the **Restart now** row does it from here).

**Right — FPS counter**: switches on and off right away, no restart needed.
It shows how many frames the game draws per second, after the FPS limit.

## Other platforms

The runtime does not know about game folders or settings screens: a
launcher starts it with a command line and environment variables
([docs/runtime-interface.md](docs/runtime-interface.md)). Besides the
FunKey launcher there are three more, all with the same in-game menu
(Esc), restart, per-game settings and debug switches.

| | Linux (x86) | Windows | Browser |
|---|---|---|---|
| Get it | `phoneme-j2me-linux-x86.tar.gz` (releases) | `phoneme-j2me-win32.zip` (releases) | <https://slrmoon.github.io/j2me-funkey-rgnano/> |
| Start a game | `./phoneME`: game list and settings, or `./run.sh game.jar` | `phoneME.exe`: game list and settings window, or drop a jar on it | choose or drop the `.jar` (+ `.jam`, `.sp`) |
| VM | JIT | JIT | interpreter (WebAssembly) |
| Window | resizable, `SCALE=`, full screen (F11) | window size, full screen, filters, Scale2x | fills the page, full screen (F11) |
| Saves | `~/.local/share/phoneme-j2me/rms/<game>` | `data\rms\<game>` | browser storage (IndexedDB), export / import as `.zip` |

Keys on a PC: arrows, Enter/Space = fire, F1/F2 = soft keys, 0-9, `*`, `#`,
Esc = in-game menu, F11 = full screen. The Windows launcher can remap them.

**Linux.** A 32-bit x86 program with an SDL window; it needs the glibc of
the build machine or newer (build on Ubuntu 20.04 to run on 20.04 and
later). `SCALE=4` makes the window bigger, `LCD=176x208` sets the screen
size the game sees (240x320 by default on Linux, Windows and in the browser;
240x240, the FunKey's screen, only on the FunKey); the packed `README.txt`
lists all options.

**Windows.** A native 32-bit build (Windows 7 or newer, also Windows 11 on
ARM), cross-compiled with MinGW-w64: no WSL, no Java on the PC.
`phoneME.exe` has the game list, display (window size, full screen,
filters), PC key map and log options. See `launchers/windows/README.txt`.

**Browser.** The same runtime compiled to WebAssembly with Emscripten; the
files stay on your computer. The page remembers the games you opened
(Recent games), so after a reload one click starts them again, and keeps
their saves in the browser; **Export saves** gives the same files as
`rms/<game>` on the FunKey, so saves move both ways. It runs the VM in
slave mode with a C interpreter; hot methods are compiled to WebAssembly
by its own JIT (`runtime/cldc/src/vm/cpu/c/WasmJit_c.hpp`), so Java-heavy
games run several times faster than interpreted, still slower than the
desktop builds. **Buttons** shows
on-screen keys (soft keys, D-pad, keypad, menu; on by default on phones and
tablets), **Keys** remaps the keyboard (several keys per phone key, soft key
type per game). Page options: `?fps=1` (FPS counter), `?limit=off`,
`?lcd=240x320`, `?jit=0` (interpreter only, if a game misbehaves with the
JIT).

## Repository structure

```text
build.sh                 ./build.sh <funkey|linux|windows|web> [package]
runtime/                 the runtime: phoneME CLDC VM, MIDP (SDL port), JSRs,
                         M3G, fkaudio, our native code (upstream phoneME tree)
java/doja/               i-appli (DoJa) API and runtime classes, all targets
launchers/
  funkey/                FunKey S / RG Nano: launcher pm-launch.c, OPK packaging
  linux/                 desktop Linux: run.sh, test-games.py, test-doja.py
  windows/               Windows: phoneME.exe (settings window), packaging
  web/                   the browser page (index.html)
scripts/                 per-target build scripts used by build.sh,
                         publish-web.sh (gh-pages)
docs/                    building, packaging, runtime interface, DoJa status
tools/                   test MIDlets, reference renderers, cloud test helpers
.github/workflows/       web.yml: builds the browser version, publishes gh-pages
out/                     build outputs (not in git): out/<target>, out/dist
```

`tools/check-runtime-interface.py` checks that every setting the runtime
reads is listed in [docs/runtime-interface.md](docs/runtime-interface.md).

Not in the repository: the FunKey SDK, the JDK, `out/`, `.toolchains/`,
prebuilt OPKs and test games.

## Required dependency versions

1. FunKey SDK (FunKey target only)
   Version: `2.3.0`
   Official docs: <https://doc.funkey-project.com/developer_guide/tutorials/build_system/build_program_using_sdk/>
   Release download used by this project:
   `https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz`

2. JDK (all targets)
   Version: `Azul Zulu JDK 7`
   Exact archive used by this project:
   `zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`
   Direct download:
   `https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`

3. Emscripten (browser target only)
   Version: `3.1.6`, the `emscripten` package of Ubuntu 24.04 / Debian.

Pass them as `JDK_DIR=...` and `FUNKEY_SDK_DIR=...`, or unpack them in the
repository root (`zulu7*-linux_x64/`, `FunKey-sdk-2.3.0/`), where the
scripts look by default.

## Host build requirements

Linux amd64 host (Ubuntu 24.04 and 26 tested): `bash`, `make`, `gcc`,
`g++`, `gcc-multilib`, `g++-multilib`, `libc6-dev-i386`, `file`,
`mksquashfs` (OPK). The VM's build tools are 32-bit Linux programs for
every target.

- Linux x86 target: `zlib1g-dev:i386 libsdl1.2-dev:i386
  libsdl-image1.2-dev:i386 libsdl-gfx1.2-dev:i386`
  (`sudo dpkg --add-architecture i386` first)
- Windows target: `mingw-w64` and `git` (SDL 1.2, SDL_gfx and zlib are
  built from GitHub sources into `out/win32/sysroot` the first time)
- Browser target: `emscripten` and `git` (SDL_gfx and zlib are built from
  GitHub sources with `emcc` the first time)

Ubuntu 24.04 only: its static i386 libm has no `fmod`, so the host tools
need a shim, see [tools/cloud-test/README.md](tools/cloud-test/README.md):
`HOST_LINK_FLAGS="-m32 -pthread /path/to/fmodshim.o"` (FunKey, Linux) or
`HOST_LINK_EXTRA=/path/to/fmodshim.o` (Windows, browser).

## Building

```bash
JDK_DIR=/path/to/zulu7... FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 ./build.sh funkey package
JDK_DIR=/path/to/zulu7... ./build.sh linux package
JDK_DIR=/path/to/zulu7... ./build.sh windows package
JDK_DIR=/path/to/zulu7... ./build.sh web
```

| Target | Runtime | Package |
|---|---|---|
| `funkey` | `out/funkey/midp/bin/arm/runMidlet` | `out/funkey/opk/pm.opk` |
| `linux` | `out/linux-x86/midp/bin/i386/runMidlet` | `out/dist/phoneme-j2me-linux-x86.tar.gz` |
| `windows` | `out/win32/midp/bin/i386/runMidlet.exe` | `out/dist/phoneme-j2me-win32.zip` |
| `web` | `out/web/midp/bin/i386/runMidlet.wasm` | `out/web/site/` (serve over http) |

The browser version is built and published to GitHub Pages by
`.github/workflows/web.yml` on every push to `main` that touches the
runtime or the page; `scripts/publish-web.sh` does the same by hand.

Compiler flags are not tracked by make: after changing them (or pulling a
commit that does) delete `out/<target>` and build again.

More: [docs/building.md](docs/building.md) (runtime build details),
[docs/packaging.md](docs/packaging.md) (OPK options, bundling one MIDlet).

### FunKey OPK

`./build.sh funkey package` builds a self-contained `pm.opk` with the
runtime, the SDL launcher, desktop entry and icon (plus a tiny built-in
`HelloMidlet` as a test). To bundle one game instead:
`JDK_DIR=... FUNKEY_SDK_DIR=... FUNKEY_BUNDLE_MIDLET=1 launchers/funkey/package.sh /path/to/game.jar [game.jad]`
(`FUNKEY_MIDLET_MAIN` sets the main class).

## Known issues

- **Controls** — button mapping is still incomplete on FunKey S / RG Nano; some games may need a different key profile (in-game menu → Controls, or `PHONEME_KEY_PROFILE` in `r.sh`).
- **3D speed** — complex M3G scenes are still CPU-bound on the single
  Cortex-A7 (no GPU). Games that use filtered or mipmapped textures take the
  slower general path. Perspective is corrected every 16 pixels, which can
  bend textures slightly on large surfaces seen at a steep angle.
- **MascotCapsule (MIDP)** — 2D drawn between `renderFigure` and `flush()`
  ends up under the 3D instead of over it.
- **Browser** — no JIT (slower than the other builds, most of all for games
  that draw their 3D in Java); no gamepad yet; the page keeps only what a
  game saved itself (no save states).
- **Not supported**: Motorola `com.motorola.game` (GameScreen API), Mobile XMF music, video, camera/recording, network.
- **Optional JSRs without the hardware** (JSR 75 FileConnection/PIM, JSR 82
  Bluetooth/OBEX, JSR 179 Location, JSR 226 SVG, JSR 239 OpenGL ES) are
  present as "not available": games that refer to them load, and the entry
  points refuse the way a handset without the feature does (no file roots,
  Bluetooth off, no location provider, SVG not loadable, EGL without a
  display); their version properties stay unset. Games that need one of
  them to play still cannot. Test: `tools/optional-stubs-test`.

## Third-party components

| Component | License | Used for |
|---|---|---|
| [phoneME](https://github.com/j2me-preservation/phoneME-GP2X-SDL) (CLDC VM, MIDP) | GPL 2.0 | the runtime |
| [Sonivox EAS](https://github.com/pedrolcl/sonivox) | Apache 2.0 | MIDI synthesizer |
| [minimp3](https://github.com/lieff/minimp3) | CC0 | MP3 decoding |
| Keitai Wiki MLD / MA-3 music implementation (Guy Perfect, SquirrelJME) | Unlicense / MPL 2.0 | MLD sequencer and MA-3 FM synth (C port) |
| fdlibm 5.3 | Sun (free use with notice) | log/pow in the MLD synth |
| Shinonome bitmap fonts | Public domain | DoJa Japanese text |
| opencore-amr | Apache 2.0 | AMR-NB decoding |
| Nokia M3G core (from phoneME) | EPL 1.0 | 3D scene graph |
| [stb_image](https://github.com/nothings/stb) | Public domain / MIT | JPEG on Windows and in the browser |
| SDL 1.2, [SDL_gfx](https://github.com/ferzkopp/SDL_gfx), zlib | LGPL 2.1 / zlib | Windows and browser builds (built from source) |
| [Emscripten](https://emscripten.org) | MIT / UIUC | browser build |
