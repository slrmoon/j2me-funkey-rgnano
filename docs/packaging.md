# Packaging and launchers

Each target has its launcher and packaging script in `launchers/<target>/`;
`./build.sh <target> package` runs it after the build. The runtime side of
the interface is in [runtime-interface.md](runtime-interface.md).

| Folder | Launcher | Package |
|---|---|---|
| `launchers/funkey/` | `pm-launch.c`: SDL game browser on the handheld | `out/funkey/opk/pm.opk` |
| `launchers/linux/` | `run.sh`, `test-games.py`, `test-doja.py` | `out/dist/funkey-j2me-linux-x86.tar.gz` |
| `launchers/windows/` | `FunKeyJ2ME.exe` (`launcher.c`): settings window | `out/dist/funkey-j2me-win32.zip` |

All packages include `lib/doja-support.jar`, built from `java/doja/stubs`
(the DoJa runtime) and `java/doja/api` (generated API skeletons).

## FunKey OPK

```bash
FUNKEY_SDK_DIR=/path/to/FunKey-sdk-2.3.0 \
JDK_DIR=/path/to/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
launchers/funkey/package.sh
```

Needs `out/funkey` from `./build.sh funkey` (`BUILD_OUTPUT_DIR` overrides it).
Result: `out/funkey/opk/pm.opk`.

The default OPK is launcher/runtime-only: it contains the browser launcher,
phoneME runtime/JVM, libraries, appdb seed, icon, and helper scripts. It does
not bundle a game `JAR`; put games on external storage and launch them from the
browser UI.

Legacy debug bundles with a built-in MIDlet are still available only when
requested explicitly:

```bash
FUNKEY_BUNDLE_MIDLET=1 launchers/funkey/package.sh /path/to/game.jar [game.jad]
```

Defaults: app title `J2ME`, desktop category `emulators`.

## Linux and Windows

```bash
JDK_DIR=/path/to/zulu7... launchers/linux/package.sh [out_dir]
JDK_DIR=/path/to/zulu7... launchers/windows/package.sh [out_dir]
```

They read `out/linux-x86` and `out/win32` and write to `out/dist` by
default. The Windows script also builds `FunKeyJ2ME.exe` (needs the MinGW
sysroot with zlib that `scripts/build-win32.sh` made in `out/win32/sysroot`).
