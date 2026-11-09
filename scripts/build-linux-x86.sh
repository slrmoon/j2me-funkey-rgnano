#!/bin/sh
# Desktop build of the runtime: 32-bit x86 Linux, SDL 1.2 window.
#
#   JDK_DIR=/path/to/zulu7...linux_x64 ./build-linux-x86.sh
#
# The VM (CLDC linux_i386) is 32-bit only; the result runs on 64-bit Linux
# with the i386 libraries installed. Host packages (Debian/Ubuntu):
#   dpkg --add-architecture i386 && apt update
#   apt install gcc-multilib g++-multilib libc6-dev-i386 zlib1g-dev:i386 \
#       libsdl1.2-dev:i386 libsdl-image1.2-dev:i386 libsdl-gfx1.2-dev:i386
# Ubuntu 24.04's static i386 libm lacks fmod/fmodf (romgen link fails):
#   gcc -m32 -O2 -c tools/cloud-test/fmodshim.c -o /tmp/fmodshim.o
#   HOST_LINK_FLAGS="-m32 /tmp/fmodshim.o" ./build-linux-x86.sh
#
# Output: $BUILD_OUTPUT_DIR/midp/bin/i386/runMidlet plus lib/ and appdb/;
# launchers/linux/package.sh packs them into a runnable folder.

set -eu

# repository root (scripts/..): JDK, FunKey SDK and .toolchains are looked up there
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MEHOME=$ROOT/runtime
BUILD_OUTPUT_DIR=${BUILD_OUTPUT_DIR:-$ROOT/out/linux-x86}
MAKE_JOBS=${MAKE_JOBS:-$(nproc 2>/dev/null || echo 1)}
# MIDP generates sources with tools it builds first, and CLDC's romgen and
# loopgen both build classes.zip: parallel make races in both.
MIDP_MAKE_JOBS=${MIDP_MAKE_JOBS:-1}
CLDC_MAKE_JOBS=${CLDC_MAKE_JOBS:-1}
# Large-file API (stat64, readdir64, ...): without it stat() fails with
# EOVERFLOW on file systems with 64-bit inode numbers (WSL /mnt/c, NFS,
# XFS, btrfs): "IOException while loading skin: Value too large".
LFS_CFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE"
HOST_CFLAGS=${HOST_CFLAGS:--m32 $LFS_CFLAGS}
# -pthread: glibc < 2.34 keeps pthread/sem_* in libpthread
HOST_LINK_FLAGS=${HOST_LINK_FLAGS:--m32 -pthread}
HOST_ASM_FLAGS=${HOST_ASM_FLAGS:---32}
# CFLAGS on the make command line replaces PCSL's own "CFLAGS +=" lines,
# so the server-socket switch has to be repeated here.
HOST_PCSL_CFLAGS=${HOST_PCSL_CFLAGS:--c -m32 -O3 -fexpensive-optimizations -DENABLE_SERVER_SOCKET=1 $LFS_CFLAGS}
HOST_PCSL_LD_FLAGS=${HOST_PCSL_LD_FLAGS:--m32}
# SSE2 maths (every x86 CPU of the last 20 years), no x87 excess precision.
X86_CFLAGS=${X86_CFLAGS:--m32 -O2 -march=i686 -msse2 -mfpmath=sse $LFS_CFLAGS}

if [ -z "${JDK_DIR:-}" ]; then
    for d in "$ROOT/zulu7.24.0.1-jdk7.0.191-linux_x64" \
             "$ROOT/zulu7.24.0.1-ca-jdk7.0.191-linux_x64"; do
        [ -d "$d" ] && JDK_DIR=$d && break
    done
fi
if [ -z "${JDK_DIR:-}" ] || [ ! -x "$JDK_DIR/bin/javac" ]; then
    echo "Set JDK_DIR to a JDK 7 directory." >&2
    exit 1
fi

for include_dir in /usr/include/i386-linux-gnu /usr/include/x86_64-linux-gnu; do
    if [ -f "$include_dir/asm/errno.h" ]; then
        HOST_CFLAGS="$HOST_CFLAGS -I$include_dir"
        HOST_PCSL_CFLAGS="$HOST_PCSL_CFLAGS -I$include_dir"
        break
    fi
done

export MEHOME BUILD_OUTPUT_DIR JDK_DIR
export AWK=${AWK:-awk}
export PATH=$JDK_DIR/bin:$PATH
export PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl
export JVMWorkSpace=$MEHOME/cldc
export JVMBuildSpace=$BUILD_OUTPUT_DIR/cldc
export MIDP_OUTPUT_DIR=$BUILD_OUTPUT_DIR/midp
export USE_LIBFLOAT=false
export ENABLE_COMPILATION_WARNINGS=true
export GCC_VERSION=

echo "Checking 32-bit SDL..."
printf '%s\n' '#include <SDL.h>' '#include <SDL_image.h>' '#include <SDL_gfxPrimitives.h>' \
    'int main(void){characterColor(0,0,0,0,0); return SDL_Init(0) < 0 && IMG_Linked_Version() == 0;}' \
    > "${TMPDIR:-/tmp}/fk-sdl-$$.c"
gcc $X86_CFLAGS $(sdl-config --cflags) "${TMPDIR:-/tmp}/fk-sdl-$$.c" \
    -o "${TMPDIR:-/tmp}/fk-sdl-$$" -lSDL -lSDL_image -lSDL_gfx || {
    echo "32-bit SDL 1.2 / SDL_image / SDL_gfx development files are missing (see header)." >&2
    exit 1
}
rm -f "${TMPDIR:-/tmp}/fk-sdl-$$.c" "${TMPDIR:-/tmp}/fk-sdl-$$"

mkdir -p "$BUILD_OUTPUT_DIR"

echo "Building PCSL (linux_i386_gcc)..."
cd "$MEHOME/pcsl"
make -j "$MAKE_JOBS" \
    PCSL_PLATFORM=linux_i386_gcc \
    NETWORK_MODULE=bsd/generic \
    CFLAGS="$HOST_PCSL_CFLAGS" \
    LD_FLAGS="$HOST_PCSL_LD_FLAGS"

echo "Building CLDC (linux_i386)..."
cd "$JVMWorkSpace/build/linux_i386"
make -j "$CLDC_MAKE_JOBS" \
    ENABLE_PCSL=true \
    PCSL_PLATFORM=linux_i386_gcc \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
    ENABLE_ISOLATES=true \
    USER_CFLAGS="$HOST_CFLAGS -fpermissive -Wno-narrowing" \
    LINK_FLAGS="$HOST_LINK_FLAGS" \
    ASM_FLAGS="$HOST_ASM_FLAGS"

CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/linux_i386/dist

echo "Building MIDP SDL (i386)..."
cd "$MEHOME/midp/build/linux_sdl_gcc"
make -j "$MIDP_MAKE_JOBS" \
    USE_SDL_ABB=true \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
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
    CMDLINE_CFLAGS="$X86_CFLAGS" \
    LD_FLAGS="-m32 -pthread" \
    USE_LIBFLOAT=false

echo "Built: $MIDP_OUTPUT_DIR/bin/i386/runMidlet"
file "$MIDP_OUTPUT_DIR/bin/i386/runMidlet"
