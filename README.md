# phoneME FunKey J2ME Runtime

> If a specific game doesn't work, feel free to reach out on Telegram: [@slr_moon](https://t.me/slr_moon)

A `phoneME`-based Java ME runtime and `OPK` wrapper for `FunKey S / RG Nano`.

Based on the upstream phoneME-GP2X-SDL source at:  
<https://github.com/j2me-preservation/phoneME-GP2X-SDL>

## Screenshots

### Launcher

<p float="left">
  <img src="screenshots/IMG_0018.PNG" width="240" />
  <img src="screenshots/IMG_0019.PNG" width="240" />
  <img src="screenshots/IMG_0010.PNG" width="240" />
</p>

### Games

<p float="left">
  <img src="screenshots/IMG_0007.PNG" width="240" />
  <img src="screenshots/IMG_0023.PNG" width="240" />
  <img src="screenshots/IMG_0028.PNG" width="240" />
  <img src="screenshots/IMG_0025.PNG" width="240" />
  <img src="screenshots/IMG_0026.PNG" width="240" />
  <img src="screenshots/IMG_0027.PNG" width="240" />
</p>

## Installation

1. Download `pm.opk` from the [latest release](https://github.com/slrmoon/j2me-funkey-rgnano/releases/latest)
2. Place it in the `emulators` folder on your FunKey/RG Nano SD card
3. Create a `java` folder (lowercase) in the root of the SD card
4. Put your `.jar` files into that `java/` folder

## Overview

This export is split into two parts:

- `phoneme-funkey-clean`
  Cleaned runtime source tree with FunKey-specific build scripts.
- `phoneme-funkey-opk-wrapper`
  `OPK` packaging layer, launcher, icon, Nokia compatibility stubs, and JAR/JAD wrapper logic.

Intentionally not included:

- `FunKey-sdk-2.3.0`
- `Zulu JDK 7`
- `build_output*`
- `.toolchains`
- prebuilt `OPK` files
- local test JARs and older backup/dev folders

## Required dependency versions

1. FunKey SDK
   Version: `2.3.0`
   Official docs: <https://doc.funkey-project.com/developer_guide/tutorials/build_system/build_program_using_sdk/>
   Release download used by this project:
   `https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz`

2. JDK
   Version: `Azul Zulu JDK 7`
   Exact archive used by this project:
   `zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`
   Archive index:
   <https://cdn.azul.com/zulu/bin/zulu>
   Direct download:
   `https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`

## Host build requirements

Confirmed working on **Ubuntu 26** (amd64 host).

- `bash`
- `make`
- `gcc`
- `g++`
- `find`
- `sed`
- `grep`
- `file`
- `mksquashfs`
- `gcc-multilib`
- `g++-multilib`
- `libc6-dev-i386`

## Expected layout

```text
repo/
  phoneme-funkey-clean/
  phoneme-funkey-opk-wrapper/
```

Recommended approach: keep the SDK and JDK outside the repository and pass them explicitly via env vars.

If you prefer local unpacked copies, these are the paths the scripts know how to use by default:

- `phoneme-funkey-clean/FunKey-sdk-2.3.0`
- `phoneme-funkey-clean/zulu7.24.0.1-ca-jdk7.0.191-linux_x64`
- `phoneme-funkey-clean/zulu7.24.0.1-jdk7.0.191-linux_x64`

## Build runtime

```bash
cd phoneme-funkey-clean
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
./build-funkey-s.sh
```

Main runtime outputs:

- `phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/midp/bin/arm/runMidlet`
- `phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/cldc/linux_arm_vfp/dist/bin/cldc_vm`

## Build OPK (launcher)

The wrapper builds a self-contained `pm.opk` with the full runtime, SDL launcher, desktop entry and icon.

**Launcher build** (no external JAR required):
If no MIDlet is passed, the script still builds the launcher `OPK`. In that mode it also bundles a tiny built-in `HelloMidlet` as a fallback/test MIDlet inside the package, but the main user-facing entry point is still the SDL launcher.

```bash
cd phoneme-funkey-opk-wrapper
FUNKEY_PHONEME_ROOT=../phoneme-funkey-clean/phoneME-GP2X-SDL \
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
./package-funkey-opk.sh
```

**With your own bundled MIDlet:**

```bash
cd phoneme-funkey-opk-wrapper
FUNKEY_PHONEME_ROOT=../phoneme-funkey-clean/phoneME-GP2X-SDL \
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
./package-funkey-opk.sh /path/to/game.jar
```

If the JAD is separate, pass it as second argument.  
Set `FUNKEY_MIDLET_MAIN` to specify the main class explicitly.

Result: `phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/pm.opk`

## Current compatibility fixes

These fixes are implemented in the runtime/launcher, so they apply to normal
JARs launched from the SD card and do not require patching individual games.

- **Soft keys** — Chameleon soft-button handling no longer eats Nokia
  `SOFT1`/`SOFT2` key events when no UI command is active. Games such as
  `Bomberman`, `Bounce`, and `Gothic 3` can receive `Canvas.keyPressed(-6)`
  and `Canvas.keyPressed(-7)` for in-game menus.
- **Nokia FullCanvas** — `com.nokia.mid.ui.FullCanvas` now enters fullscreen
  mode on construction. This matches Nokia game expectations and prevents the
  Chameleon command area from appearing below fullscreen games after saved
  display settings are reapplied.
- **Nokia DirectUtils / DirectGraphics rendering** —
  `DirectUtils.createImage(width, height, argb)` now creates mutable images
  with real alpha buffers when requested, keeps non-zero RGB fill colors
  opaque, and preserves alpha through mutable-image snapshots and image-to-image
  compositing. `DirectGraphics.drawImage` also maps Nokia manipulation flags to
  MIDP transforms, including Nokia's counter-clockwise `ROTATE_90`/`ROTATE_270`
  semantics plus horizontal/vertical flips. This fixes Bounce-style black
  outlines, the large ball sprite, and transformed slope/surface tiles.
- **Runtime-only OPK packaging** — `FUNKEY_RUNTIME_ONLY=1` builds a launcher
  OPK without bundling any MIDlet JAR/JAD, so release packages can stay generic.

## Known issues

- **Controls** — some games use vendor-specific or device-specific key codes.
  The launcher provides selectable key profiles and per-game bindings, but a
  few games may still need manual mapping.
- **No 3D** — JSR 184 (M3G) is not supported. 3D J2ME games will not run.

## Debugging

After launching a game, the runtime writes a log to:

`/mnt/FunKey/.pm/runmidlet.log`
