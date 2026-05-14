# phoneME FunKey J2ME Runtime

GitHub-ready snapshot of a `phoneME`-based Java ME runtime and `OPK` wrapper for `FunKey S / RG Nano`.

Based on the upstream phoneME-GP2X-SDL source at:  
<https://github.com/j2me-preservation/phoneME-GP2X-SDL>

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
  FunKey-sdk-2.3.0/                       # unpack separately, not committed
  zulu7.24.0.1-ca-jdk7.0.191-linux_x64/   # unpack separately, not committed
```

You can also keep SDK/JDK anywhere else and pass them explicitly via env vars.

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

## Build OPK

```bash
cd phoneme-funkey-opk-wrapper
FUNKEY_PHONEME_ROOT=../phoneme-funkey-clean/phoneME-GP2X-SDL \
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
./package-funkey-opk.sh /path/to/game.jar
```

Result:

- `phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/pm.opk`

## Notes

- The launcher desktop category is currently set to `emulators`.
- The launcher title is currently set to `J2ME`.
- If you move the SDK after unpacking, re-run its `relocate-sdk.sh`.
