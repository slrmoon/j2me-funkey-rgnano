#!/bin/sh
# Windows build of the runtime: 32-bit x86 (i686) Win32, SDL 1.2 window,
# cross-compiled on Linux with MinGW-w64. No WSL/Cygwin needed to run it.
#
#   JDK_DIR=/path/to/zulu7...linux_x64 ./build-win32.sh
#
# Host packages (Debian/Ubuntu):
#   apt install mingw-w64 gcc-multilib g++-multilib libc6-dev-i386 make git
# The VM's host tools (romgen, loopgen, preverify) are 32-bit Linux programs,
# as in build-linux-x86.sh. Ubuntu 24.04's static i386 libm lacks fmod:
#   gcc -m32 -O2 -c tools/cloud-test/fmodshim.c -o /tmp/fmodshim.o
#   HOST_LINK_EXTRA=/tmp/fmodshim.o ./build-win32.sh
#
# SDL 1.2, SDL_gfx and zlib are built (static) into $MINGW_SYSROOT from
# GitHub sources the first time (SDL_SRC, SDL_GFX_SRC, ZLIB_SRC: local
# checkouts instead).
#
# Output: $BUILD_OUTPUT_DIR/midp/bin/i386/runMidlet.exe plus lib/ and
# appdb/; launchers/windows/package.sh packs them with
# the launcher into a zip.

set -eu

# repository root (scripts/..): JDK, FunKey SDK and .toolchains are looked up there
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MEHOME=$ROOT/runtime
BUILD_OUTPUT_DIR=${BUILD_OUTPUT_DIR:-$ROOT/out/win32}
MINGW_PREFIX=${MINGW_PREFIX:-i686-w64-mingw32-}
MINGW_SYSROOT=${MINGW_SYSROOT:-$BUILD_OUTPUT_DIR/sysroot}
MAKE_JOBS=${MAKE_JOBS:-$(nproc 2>/dev/null || echo 1)}
# MIDP generates sources with tools it builds first, and CLDC's romgen and
# loopgen both build classes.zip: parallel make races in both.
MIDP_MAKE_JOBS=${MIDP_MAKE_JOBS:-1}
CLDC_MAKE_JOBS=${CLDC_MAKE_JOBS:-1}
# SSE2 maths (every x86 CPU of the last 20 years), no x87 excess precision.
WIN_CFLAGS=${WIN_CFLAGS:--O2 -march=i686 -msse2 -mfpmath=sse}
HOST_LINK_EXTRA=${HOST_LINK_EXTRA:-}

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
if ! command -v "${MINGW_PREFIX}gcc" >/dev/null 2>&1; then
    echo "${MINGW_PREFIX}gcc not found (apt install mingw-w64)." >&2
    exit 1
fi

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
export MINGW_PREFIX

mkdir -p "$BUILD_OUTPUT_DIR" "$MINGW_SYSROOT/lib" "$MINGW_SYSROOT/include/SDL"
DEPS=$BUILD_OUTPUT_DIR/deps
mkdir -p "$DEPS"

fetch() {  # fetch <dir> <git url>
    if [ ! -d "$1" ]; then
        git clone -q --depth 1 "$2" "$1"
    fi
}

# ---- third-party libraries (static) -------------------------------------
if [ ! -f "$MINGW_SYSROOT/lib/libz.a" ]; then
    echo "Building zlib..."
    ZLIB_SRC=${ZLIB_SRC:-$DEPS/zlib}
    fetch "$ZLIB_SRC" https://github.com/madler/zlib
    (cd "$ZLIB_SRC" && make -f win32/Makefile.gcc PREFIX="$MINGW_PREFIX" libz.a >/dev/null)
    cp "$ZLIB_SRC/libz.a" "$MINGW_SYSROOT/lib/"
    cp "$ZLIB_SRC/zlib.h" "$ZLIB_SRC/zconf.h" "$MINGW_SYSROOT/include/"
fi
if [ ! -f "$MINGW_SYSROOT/lib/libSDL.a" ]; then
    echo "Building SDL 1.2..."
    SDL_SRC=${SDL_SRC:-$DEPS/SDL-1.2}
    fetch "$SDL_SRC" https://github.com/libsdl-org/SDL-1.2
    (cd "$SDL_SRC" &&
     ./configure --host="${MINGW_PREFIX%-}" --prefix="$MINGW_SYSROOT" \
         --disable-shared --enable-static --disable-video-opengl \
         --disable-stdio-redirect >/dev/null &&
     make -j "$MAKE_JOBS" >/dev/null && make install >/dev/null)
fi
if [ ! -f "$MINGW_SYSROOT/lib/libSDL_gfx.a" ]; then
    echo "Building SDL_gfx..."
    SDL_GFX_SRC=${SDL_GFX_SRC:-$DEPS/SDL_gfx}
    fetch "$SDL_GFX_SRC" https://github.com/ferzkopp/SDL_gfx
    mkdir -p "$DEPS/gfx-obj"
    for f in SDL_gfxPrimitives SDL_rotozoom SDL_framerate SDL_imageFilter SDL_gfxBlitFunc; do
        "${MINGW_PREFIX}gcc" $WIN_CFLAGS -I"$MINGW_SYSROOT/include/SDL" \
            -c "$SDL_GFX_SRC/$f.c" -o "$DEPS/gfx-obj/$f.o"
    done
    "${MINGW_PREFIX}ar" rcs "$MINGW_SYSROOT/lib/libSDL_gfx.a" "$DEPS"/gfx-obj/*.o
    cp "$SDL_GFX_SRC"/SDL_gfxPrimitives.h "$SDL_GFX_SRC"/SDL_gfxPrimitives_font.h \
       "$SDL_GFX_SRC"/SDL_rotozoom.h "$SDL_GFX_SRC"/SDL_framerate.h \
       "$SDL_GFX_SRC"/SDL_imageFilter.h "$SDL_GFX_SRC"/SDL_gfxBlitFunc.h \
       "$MINGW_SYSROOT/include/SDL/"
fi

# MIDP's makefiles call plain gcc/g++/ar from GNU_TOOLS_DIR/bin
TOOLCHAIN_DIR=$BUILD_OUTPUT_DIR/toolchain
mkdir -p "$TOOLCHAIN_DIR/bin"
# (with a compiler cache in front of PATH - tools/cloud-setup.sh - the links
# must reach the real MinGW tools, and gcc/g++ go through ccache by script)
real_tool() {
    PATH=$(printf '%s' "$PATH" | tr ':' '\n' | grep -vx "${CCACHE_WRAPPERS:-/nonexistent}" |
           paste -sd:) command -v "$1"
}
for t in gcc g++ cpp ar as ld ranlib strip objcopy objdump nm windres; do
    rm -f "$TOOLCHAIN_DIR/bin/$t"
    if [ -n "${CCACHE_WRAPPERS:-}" ] && { [ "$t" = gcc ] || [ "$t" = g++ ]; }; then
        printf '#!/bin/sh\nexec ccache %s "$@"\n' "$(real_tool "${MINGW_PREFIX}$t")" \
            > "$TOOLCHAIN_DIR/bin/$t"
        chmod +x "$TOOLCHAIN_DIR/bin/$t"
    else
        ln -sf "$(real_tool "${MINGW_PREFIX}$t")" "$TOOLCHAIN_DIR/bin/$t"
    fi
done

# ---- PCSL ---------------------------------------------------------------
echo "Building PCSL (win32_i386_mingw)..."
cd "$MEHOME/pcsl"
make -j "$MAKE_JOBS" \
    PCSL_PLATFORM=win32_i386_mingw \
    MINGW_PREFIX="$MINGW_PREFIX" \
    CFLAGS="-c $WIN_CFLAGS -DENABLE_SERVER_SOCKET=1"
# MIDP looks for PCSL under its TARGET_OS (linux) name
ln -sfn win32_i386 "$PCSL_OUTPUT_DIR/linux_i386"

# ---- CLDC ---------------------------------------------------------------
echo "Building CLDC (mingw_i386)..."
cd "$JVMWorkSpace/build/mingw_i386"
make -j "$CLDC_MAKE_JOBS" \
    ENABLE_PCSL=true \
    PCSL_PLATFORM=win32_i386_mingw \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
    ENABLE_ISOLATES=true \
    USER_CFLAGS="-fpermissive -Wno-narrowing" \
    HOST_LINK_EXTRA="$HOST_LINK_EXTRA"

CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/mingw_i386/dist

# ---- MIDP ---------------------------------------------------------------
echo "Building MIDP SDL (Win32)..."
cd "$MEHOME/midp/build/win32_sdl_mingw"
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
    GNU_TOOLS_DIR="$TOOLCHAIN_DIR" \
    MINGW_SYSROOT="$MINGW_SYSROOT" \
    CMDLINE_CFLAGS="$WIN_CFLAGS" \
    USE_LIBFLOAT=false

echo "Built: $MIDP_OUTPUT_DIR/bin/i386/runMidlet.exe"
file "$MIDP_OUTPUT_DIR/bin/i386/runMidlet.exe"
