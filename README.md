# phoneME FunKey J2ME Runtime

> If a specific game doesn't work, reach out on Telegram: [@slr_moon](https://t.me/slr_moon)

A `phoneME`-based Java ME runtime and `OPK` wrapper for `FunKey S / RG Nano`.

Based on the upstream phoneME-GP2X-SDL source at:  
<https://github.com/j2me-preservation/phoneME-GP2X-SDL>

## Screenshots

<p float="left">
  <img src="IMG_0018.PNG" width="240" />
  <img src="IMG_0019.PNG" width="240" />
  <img src="IMG_0023.PNG" width="240" />
  <img src="IMG_0024.PNG" width="240" />
</p>

## Installation

1. Download `pm.opk` from the [latest release](https://github.com/slrmoon/j2me-funkey-rgnano/releases/latest)
2. Place it in the `emulators` folder on your FunKey/RG Nano SD card
3. Create a `java` folder in the root of the SD card
4. Put your `.jar` files into that `java/` folder

## Scaling setup

Each game can have its own scaling mode configured via the overlay menu. Launch a game, press **Power**, go to scaling settings and choose:

**Fit**  
The entire game fits within the screen. Aspect ratio is preserved. Safest mode for most games.

**Fill**  
Game fills more of the screen, but edges may be cropped. Useful when the game is nearly square and black bars are distracting.

**Stretch**  
Game is stretched to fill the whole 240x240 screen without preserving proportions. Use only if distortion doesn't bother you.

**Original**  
Game is displayed at its native resolution. Small games will have lots of empty space; large games may not fit.

### Recommendations

| Native resolution | Best mode |
|---|---|
| 128x128, 128x160, 176x208 | **Fit**. Try **Fill** if the image is too small. |
| 240x320, 240x400, 320x240 | **Fit** — keeps UI and text intact. For 240x320 portrait games the image shrinks to 180x240, that's normal. |
| 240x240 | **Original** or **Fit**. There will be almost no difference. |
| 176x220, 240x260, non-standard | Start with **Fit**. Try **Fill** if borders are too large. |

### Rule of thumb

If text, menus, HUD, health, ammo, buttons and bottom hints matter — use **Fit**.

If the game is visually simple and you can afford to lose some edges — try **Fill**.

Keep **Stretch** as a last resort, when the game feels too small or black bars annoy you more than distortion.

---

## Repository structure

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

## Known issues

- **Controls** — button mapping is still incomplete on FunKey S / RG Nano; some games may require tweaking `PHONEME_KEY_PROFILE` and `PHONEME_ENABLE_GP2X_KEYS` env vars inside `r.sh`.
- **No 3D** — JSR 184 (M3G) is not supported. 3D J2ME games will not run.

## Debugging

After launching a game, the runtime writes a log to:

`/mnt/FunKey/.pm/runmidlet.log`
