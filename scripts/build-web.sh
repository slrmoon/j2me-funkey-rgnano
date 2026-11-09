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
EMSCRIPTEN_LINK_FLAGS=${EMSCRIPTEN_LINK_FLAGS:--O2 -sNODERAWFS=1}

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

echo "=== done: $BUILD_OUTPUT_DIR/cldc/emscripten_c/dist/bin/cldc_vm.js"
