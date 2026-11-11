# Building the runtime

`runtime/` is a cleaned `phoneME` feature tree (upstream:
<https://github.com/j2me-preservation/phoneME-GP2X-SDL>) with our ports.
Each target is built by one script in `scripts/`, called by `./build.sh`:

| Script | Target | Output |
|---|---|---|
| `scripts/build-funkey.sh` | FunKey S / RG Nano (sets up the FunKey SDK, then runs `build-armv7.sh`) | `out/funkey` |
| `scripts/build-armv7.sh` | generic ARMv7 hard-float Linux (any cross compiler in `CROSS_CC`) | `out/armv7` |
| `scripts/build-linux-x86.sh` | 32-bit x86 Linux, SDL window | `out/linux-x86` |
| `scripts/build-win32.sh` | 32-bit Windows, MinGW-w64 | `out/win32` |
| `scripts/build-web.sh` | the browser (WebAssembly, Emscripten 3.1.6 from Debian/Ubuntu; C interpreter, VM in slave mode) | `out/web/site` |

Every script builds PCSL, then the CLDC VM (`runtime/cldc/build/<platform>`),
then MIDP (`runtime/midp/build/<platform>`). `BUILD_OUTPUT_DIR` overrides the
output folder. The JDK (`JDK_DIR`) and the FunKey SDK (`FUNKEY_SDK_DIR`) are
looked up in the repository root when not set.

| Target | PCSL | CLDC | MIDP |
|---|---|---|---|
| FunKey | `linux_arm_gcc` | `linux_arm_vfp` | `linux_sdl_gcc`, `TARGET_CPU=arm` |
| Linux x86 | `linux_i386_gcc` | `linux_i386` | `linux_sdl_gcc`, `TARGET_CPU=i386` |
| Windows | `win32_i386_mingw` | `mingw_i386` | `win32_sdl_mingw` |

The browser build (`./build.sh web`) needs the `emscripten` package and the
32-bit host compiler (`gcc-multilib`) for the VM's build tools; on Ubuntu
24.04 also `tools/cloud-test/fmodshim.c` compiled with `gcc -m32` and passed
as `HOST_LINK_EXTRA`. `scripts/publish-web.sh` puts `out/web/site` on the
`gh-pages` branch; `.github/workflows/web.yml` does both on every push to
`main` that touches the runtime or the page.

In a fresh cloud container (Ubuntu 24.04, e.g. a Claude session) one
script sets up every target and restores the compiler cache:

```bash
tools/cloud-setup.sh          # packages, JDK 7, FunKey SDK, ccache wrappers, cache
. /root/tc/env.sh             # JDK_DIR, FUNKEY_SDK_DIR, fmod shim, ccache
./build.sh web                # or funkey / linux / windows [package]
tools/cloud-cache-save.sh     # after building: upload the updated cache
```

The cache (ccache's directory) is kept in the private repository
`slrmoon/j2me-build-cache` (a Claude session needs it attached), one commit
replaced on each save. Make rebuilds everything in a fresh clone, but every compiler call
(gcc, MinGW, the FunKey compiler, emcc) is answered from the cache when its
input did not change. The browser build in GitHub Actions keeps its own
ccache with `actions/cache`.

Main FunKey artifacts:

- `out/funkey/midp/bin/arm/runMidlet`
- `out/funkey/cldc/linux_arm_vfp/dist/bin/cldc_vm`

## Scope

This is not a pristine upstream checkout. It already contains the changes needed for the current FunKey branch, including build and compatibility work for `musl/armhf`.

FunKey-specific subsystems in this tree:

| Path (under `runtime/`) | What |
|---|---|
| `jsr184/src/native/funkey/` | M3G (JSR 184): KNI glue and the software OpenGL ES 1.x renderer (`ngl_software.h`) |
| `jsr184/src/native/upstream/m3g/` | Nokia M3G core (EPL 1.0) |
| `midp/src/media-sdl/reference/classes/` | MMAPI (JSR 135) player, controls, vendor sound APIs (Nokia, Samsung, Siemens, Motorola) |
| `midp/src/media-sdl/reference/native/fkaudio/` | fkaudio: format parsers/decoders, sequencer, mixer, SDL output, KNI |
| `midp/src/media-sdl/reference/native/sonivox/` | Sonivox EAS synthesizer (Apache 2.0), see `README.funkey` for local changes |
| `midp/src/highlevelui/lfjport/sdl/native/` | screen presenter, scaler (`lfjport_scale.h`) and the in-game menu |
| `midp/src/events/mastermode_port/sdl/native/` | input, key profiles, PC key map, modal menu loop |
| `midp/src/core/phoneme_config/` | the settings a launcher passes (see `runtime-interface.md`) |

Runtime Java in this tree is compiled with JDK 7 `-source 1.4` for CLDC 1.1:
do not use class literals (`Foo.class`) there — javac turns them into a helper
that calls `Throwable.initCause`, which CLDC does not have.

Launchers and packaging live in `launchers/<target>/`; what they pass to the
runtime is described in [runtime-interface.md](runtime-interface.md).
