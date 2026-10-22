# phoneME for FunKey S

This directory contains the runtime/source side of the project: a cleaned `phoneME` tree plus FunKey-specific build scripts.

Included here:

- `phoneME-GP2X-SDL/phoneme_feature`
- `build-funkey-s.sh`
- `build-ubuntu26-armv7.sh`
- `README.upstream.md`

Intentionally not included:

- `OPK` packaging
- launcher-only assets
- prebuilt `build_output*`
- downloaded SDK/JDK bundles
- local toolchain cache

## Dependency versions

1. `FunKey SDK 2.3.0`
   Docs:
   <https://doc.funkey-project.com/developer_guide/tutorials/build_system/build_program_using_sdk/>
   Direct release used by this project:
   `https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz`

2. `Azul Zulu JDK 7`
   Exact archive used by this project:
   `zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`
   Direct download:
   `https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`

3. Host packages
   - `bash`
   - `make`
   - `gcc`
   - `g++`
   - `find`
   - `sed`
   - `grep`
   - `file`
   - `gcc-multilib`
   - `g++-multilib`
   - `libc6-dev-i386`

## Build

```bash
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
./build-funkey-s.sh
```

If you unpack the JDK locally inside this folder, the script also accepts:

- `./zulu7.24.0.1-jdk7.0.191-linux_x64`
- `./zulu7.24.0.1-ca-jdk7.0.191-linux_x64`

## Output

Main runtime artifacts:

- `phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/midp/bin/arm/runMidlet`
- `phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/cldc/linux_arm_vfp/dist/bin/cldc_vm`

## Scope

This is not a pristine upstream checkout. It already contains the changes needed for the current FunKey branch, including build and compatibility work for `musl/armhf`.

FunKey-specific subsystems in this tree:

| Path (under `phoneME-GP2X-SDL/phoneme_feature`) | What |
|---|---|
| `jsr184/src/native/funkey/` | M3G (JSR 184): KNI glue and the software OpenGL ES 1.x renderer (`ngl_software.h`) |
| `jsr184/src/native/upstream/m3g/` | Nokia M3G core (EPL 1.0) |
| `midp/src/media-sdl/reference/classes/` | MMAPI (JSR 135) player, controls, vendor sound APIs (Nokia, Samsung, Siemens, Motorola) |
| `midp/src/media-sdl/reference/native/fkaudio/` | fkaudio: format parsers/decoders, sequencer, mixer, SDL output, KNI |
| `midp/src/media-sdl/reference/native/sonivox/` | Sonivox EAS synthesizer (Apache 2.0), see `README.funkey` for local changes |
| `midp/src/highlevelui/lfjport/sdl/native/` | screen presenter, scaler (`lfjport_scale.h`) and the in-game menu |
| `midp/src/events/mastermode_port/sdl/native/` | input, key profiles, modal menu loop |

Runtime Java in this tree is compiled with JDK 7 `-source 1.4` for CLDC 1.1:
do not use class literals (`Foo.class`) there — javac turns them into a helper
that calls `Throwable.initCause`, which CLDC does not have.

For `OPK` packaging and the game launcher layer, use the sibling directory `../phoneme-funkey-opk-wrapper`.
