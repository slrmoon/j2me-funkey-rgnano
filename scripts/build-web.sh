#!/bin/sh
# WebAssembly build of the VM (work in progress, branch "web").
#
#   JDK_DIR=/path/to/zulu7...linux_x64 ./build-web.sh
#
# Step 1: CLDC only (C interpreter, no JIT, no MIDP yet): cldc_vm.js +
# cldc_vm.wasm, run with node:
#   node --no-experimental-fetch out/web/cldc/emscripten_c/dist/bin/cldc_vm.js \
#       -cp <classes> Hello
# (--no-experimental-fetch: node 18+ breaks the Emscripten 3.1.6 glue.)
#
# Needs Emscripten (tested: Debian/Ubuntu "emscripten" 3.1.6 package) and,
# for the host tools the VM builds first (romgen, loopgen: 32-bit x86), the
# i386 packages of build-linux-x86.sh. Ubuntu 24.04's static i386 libm
# lacks fmod/fmodf:
#   gcc -m32 -O2 -c tools/cloud-test/fmodshim.c -o /tmp/fmodshim.o
#   HOST_LINK_EXTRA=/tmp/fmodshim.o ./build-web.sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MEHOME=$ROOT/runtime
BUILD_OUTPUT_DIR=${BUILD_OUTPUT_DIR:-$ROOT/out/web}
MAKE_JOBS=${MAKE_JOBS:-$(nproc 2>/dev/null || echo 1)}
HOST_LINK_EXTRA=${HOST_LINK_EXTRA:-}
EMSCRIPTEN_LINK_FLAGS=${EMSCRIPTEN_LINK_FLAGS:--O2 -sNODERAWFS=1 -sASYNCIFY}
EMSCRIPTEN_MIDP_LINK_FLAGS=${EMSCRIPTEN_MIDP_LINK_FLAGS:--O2 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB}

if [ -z "${JDK_DIR:-}" ]; then
    for d in "$ROOT"/zulu7*-linux_x64; do
        [ -x "$d/bin/javac" ] && JDK_DIR=$d && break
    done
fi
[ -n "${JDK_DIR:-}" ] || { echo "JDK_DIR is not set" >&2; exit 1; }
command -v emcc >/dev/null || { echo "emcc not found (install Emscripten)" >&2; exit 1; }

export JDK_DIR PATH="$JDK_DIR/bin:$PATH" MEHOME
export JVMWorkSpace=$MEHOME/cldc JVMBuildSpace=$BUILD_OUTPUT_DIR/cldc
export ENABLE_COMPILATION_WARNINGS=true GCC_VERSION= USE_LIBFLOAT=false AWK=awk

mkdir -p "$BUILD_OUTPUT_DIR"

echo "=== PCSL (wasm)"
make -C "$MEHOME/pcsl" -j"$MAKE_JOBS" PCSL_PLATFORM=emscripten_wasm32 \
    PCSL_OUTPUT_DIR="$BUILD_OUTPUT_DIR/pcsl" CFLAGS="-c -O2"

echo "=== CLDC (wasm, C interpreter)"
make -C "$MEHOME/cldc/build/emscripten_c" ENABLE_PCSL=true \
    PCSL_PLATFORM=emscripten_wasm32 PCSL_OUTPUT_DIR="$BUILD_OUTPUT_DIR/pcsl" \
    ENABLE_ISOLATES=true USER_CFLAGS="-fpermissive -Wno-narrowing" \
    HOST_LINK_EXTRA="$HOST_LINK_EXTRA" \
    EMSCRIPTEN_LINK_FLAGS="$EMSCRIPTEN_LINK_FLAGS" product

# the VM makefiles copy only the executable (cldc_vm.js) to dist/
cp "$BUILD_OUTPUT_DIR/cldc/emscripten_c/target/bin/cldc_vm.wasm" \
    "$BUILD_OUTPUT_DIR/cldc/emscripten_c/dist/bin/"

CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/emscripten_c/dist
[ "${WEB_CLDC_ONLY:-}" = 1 ] && { echo "=== done: $CLDC_DIST_DIR/bin/cldc_vm.js"; exit 0; }

DEPS=$BUILD_OUTPUT_DIR/deps
mkdir -p "$DEPS"
SDL_GFX_SRC=${SDL_GFX_SRC:-$DEPS/SDL_gfx}
if [ ! -f "$DEPS/libSDL_gfx.a" ]; then
    echo "=== SDL_gfx (wasm)"
    [ -d "$SDL_GFX_SRC" ] || git clone -q --depth 1 https://github.com/ferzkopp/SDL_gfx "$SDL_GFX_SRC"
    for f in SDL_gfxPrimitives SDL_rotozoom; do
        emcc -O2 -c "$SDL_GFX_SRC/$f.c" -o "$DEPS/$f.o"
    done
    emar rcs "$DEPS/libSDL_gfx.a" "$DEPS/SDL_gfxPrimitives.o" "$DEPS/SDL_rotozoom.o"
fi

ZLIB_SRC=${ZLIB_SRC:-$DEPS/zlib}
if [ ! -f "$DEPS/libz.a" ]; then
    echo "=== zlib (wasm)"
    [ -d "$ZLIB_SRC" ] || git clone -q --depth 1 https://github.com/madler/zlib "$ZLIB_SRC"
    mkdir -p "$DEPS/zlib-obj"
    for f in adler32 crc32 deflate infback inffast inflate inftrees trees zutil \
             compress uncompr; do
        emcc -O2 -c "$ZLIB_SRC/$f.c" -o "$DEPS/zlib-obj/$f.o"
    done
    emar rcs "$DEPS/libz.a" "$DEPS"/zlib-obj/*.o
fi

# MIDP's makefiles call plain gcc/g++/ar from GNU_TOOLS_DIR/bin
TOOLCHAIN_DIR=$BUILD_OUTPUT_DIR/toolchain
mkdir -p "$TOOLCHAIN_DIR/bin"
# (wrapper scripts: emcc finds its python half by its own name)
for pair in gcc:emcc g++:em++ ar:emar ranlib:emranlib; do
    printf '#!/bin/sh\nexec %s "$@"\n' "$(command -v "${pair#*:}")" \
        > "$TOOLCHAIN_DIR/bin/${pair%%:*}"
    chmod +x "$TOOLCHAIN_DIR/bin/${pair%%:*}"
done
# MIDP looks for PCSL under its TARGET_OS_TARGET_CPU name
ln -sfn linux_c "$BUILD_OUTPUT_DIR/pcsl/linux_i386"

echo "=== MIDP SDL (wasm)"
make -C "$MEHOME/midp/build/emscripten_sdl" -j1 \
    USE_SDL_ABB=true \
    MIDP_OUTPUT_DIR="$BUILD_OUTPUT_DIR/midp" \
    PCSL_OUTPUT_DIR="$BUILD_OUTPUT_DIR/pcsl" \
    CLDC_DIST_DIR="$CLDC_DIST_DIR" \
    JSR_120_DIR="$MEHOME/jsr120" \
    PROJECT_JSR_120_DIR="$MEHOME/jsr120" \
    USE_JSR_120=true \
    USE_JSR_184=true \
    JSR_184_DIR="$MEHOME/jsr184" \
    PROJECT_JSR_184_DIR="$MEHOME/jsr184" \
    SWERVE_DIR="$MEHOME/jsr184/src/native/upstream/m3g" \
    FUNKEY_ENABLE_M3G_NATIVE=true \
    TOOLS_DIR="$MEHOME/tools" \
    TARGET_CPU=i386 \
    USE_MULTIPLE_ISOLATES=false \
    GNU_TOOLS_DIR="$TOOLCHAIN_DIR" \
    CMDLINE_CFLAGS="-O2" \
    SDL_GFX_INCLUDES="-I$SDL_GFX_SRC -I$ZLIB_SRC" \
    SDL_GFX_LIB="$DEPS/libSDL_gfx.a -L$DEPS -lz" \
    EMSCRIPTEN_LINK_FLAGS="$EMSCRIPTEN_MIDP_LINK_FLAGS" \
    USE_LIBFLOAT=false

echo "=== done: $BUILD_OUTPUT_DIR/midp/bin/i386/runMidlet.js"
