# FunKey OPK Wrapper for phoneME

This directory contains the `OPK` packaging layer around the runtime:

- launcher
- JAR/JAD wrapping
- icon + desktop entry
- Nokia compatibility stubs

It is intentionally kept separate from the runtime source tree.

## Contents

- `package-funkey-opk.sh`
- `packaging/funkey-s/pm-launch.c`
- `packaging/funkey-s/HelloMidlet.java`
- `packaging/funkey-s/nokia-stubs/*`
- `packaging/funkey-s/java-runtime-icon.png`

## Expected layout

By default the wrapper expects the runtime source tree next to it:

```text
repo/
  phoneme-funkey-clean/
  phoneme-funkey-opk-wrapper/
```

Default runtime root used by the script:

`../phoneme-funkey-clean/phoneME-GP2X-SDL`

You can override it with:

```bash
FUNKEY_PHONEME_ROOT=/path/to/phoneME-GP2X-SDL
```

## Dependency versions

1. Runtime tree
   Build `../phoneme-funkey-clean` first so that `build_output_funkey_s` exists.

2. `FunKey SDK 2.3.0`
   Direct release used by this project:
   `https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz`

3. `Azul Zulu JDK 7`
   Exact archive used by this project:
   `zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`
   Direct download:
   `https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz`

## Build an OPK

```bash
FUNKEY_PHONEME_ROOT=../phoneme-funkey-clean/phoneME-GP2X-SDL \
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
./package-funkey-opk.sh /path/to/game.jar
```

Result:

- `../phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/pm.opk`

If no `JAR` is provided, the script still builds the launcher `OPK` and bundles a tiny built-in `HelloMidlet` as a fallback/test MIDlet. The main entry point remains the SDL launcher.

For a pure launcher/runtime OPK without any bundled MIDlet JAR:

```bash
FUNKEY_RUNTIME_ONLY=1 ./package-funkey-opk.sh
```

## Current launcher defaults

- App title: `J2ME`
- Desktop category: `emulators`

## Not included here

- `build_output*`
- prebuilt `OPK` files
- SDK/JDK bundles
- local test JARs
