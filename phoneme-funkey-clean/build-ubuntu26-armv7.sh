#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
MEHOME=$ROOT/phoneME-GP2X-SDL/phoneme_feature
BUILD_OUTPUT_DIR=${BUILD_OUTPUT_DIR:-$MEHOME/build_output}
ARM_TRIPLET=${ARM_TRIPLET:-arm-linux-gnueabihf}
ARM_ABI=${ARM_ABI:-hard}
ARM_CLDC_PLATFORM=${ARM_CLDC_PLATFORM:-linux_arm_vfp}
ARM_TOOLCHAIN_DIR=${ARM_TOOLCHAIN_DIR:-$ROOT/.toolchains/armv7}
CROSS_CC=${CROSS_CC:-}
CROSS_CXX=${CROSS_CXX:-}
CROSS_CPP=${CROSS_CPP:-}
CROSS_AR=${CROSS_AR:-}
CROSS_AS=${CROSS_AS:-}
CROSS_LD=${CROSS_LD:-}
CROSS_RANLIB=${CROSS_RANLIB:-}
CROSS_STRIP=${CROSS_STRIP:-}
CROSS_OBJCOPY=${CROSS_OBJCOPY:-}
CROSS_OBJDUMP=${CROSS_OBJDUMP:-}
CROSS_NM=${CROSS_NM:-}
MAKE_JOBS=${MAKE_JOBS:-}
HOST_CFLAGS=${HOST_CFLAGS:--m32}
HOST_CXXFLAGS=${HOST_CXXFLAGS:--m32 -fpermissive}
HOST_LINK_FLAGS=${HOST_LINK_FLAGS:--m32}
HOST_ASM_FLAGS=${HOST_ASM_FLAGS:---32}
HOST_PCSL_CFLAGS=${HOST_PCSL_CFLAGS:--c -m32 -O3 -fexpensive-optimizations}
HOST_PCSL_LD_FLAGS=${HOST_PCSL_LD_FLAGS:--m32}
HOST_MULTIARCH_INCLUDE=${HOST_MULTIARCH_INCLUDE:-}

if [ -z "$HOST_MULTIARCH_INCLUDE" ]; then
    for include_dir in /usr/include/i386-linux-gnu /usr/include/x86_64-linux-gnu; do
        if [ -f "$include_dir/asm/errno.h" ]; then
            HOST_MULTIARCH_INCLUDE=$include_dir
            break
        fi
    done
fi

if [ -n "$HOST_MULTIARCH_INCLUDE" ]; then
    HOST_CFLAGS="$HOST_CFLAGS -I$HOST_MULTIARCH_INCLUDE"
    HOST_CXXFLAGS="$HOST_CXXFLAGS -I$HOST_MULTIARCH_INCLUDE"
    HOST_PCSL_CFLAGS="$HOST_PCSL_CFLAGS -I$HOST_MULTIARCH_INCLUDE"
fi

case "$ARM_ABI" in
    hard)
        ARM_DEFAULT_CFLAGS="-std=gnu89 -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard"
        ARM_DEFAULT_ASM_FLAGS="-march=armv7-a -mfpu=vfpv3-d16"
        USE_LIBFLOAT=${USE_LIBFLOAT:-false}
        ;;
    softfp)
        ARM_DEFAULT_CFLAGS="-std=gnu89 -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp"
        ARM_DEFAULT_ASM_FLAGS="-march=armv7-a -mfpu=vfpv3-d16"
        USE_LIBFLOAT=${USE_LIBFLOAT:-false}
        ;;
    soft)
        ARM_DEFAULT_CFLAGS="-std=gnu89 -marm -march=armv7-a -mfloat-abi=soft"
        ARM_DEFAULT_ASM_FLAGS="-march=armv7-a"
        USE_LIBFLOAT=${USE_LIBFLOAT:-true}
        ;;
    *)
        echo "ARM_ABI must be one of: hard, softfp, soft" >&2
        exit 1
        ;;
esac

ARM_CFLAGS=${ARM_CFLAGS:-$ARM_DEFAULT_CFLAGS}
ARM_ASM_FLAGS=${ARM_ASM_FLAGS:-$ARM_DEFAULT_ASM_FLAGS}
ARM_LINK_FLAGS=${ARM_LINK_FLAGS:-$ARM_CFLAGS}
PCSL_ARM_CFLAGS=${PCSL_ARM_CFLAGS:-$ARM_CFLAGS}
PCSL_ARM_LD_FLAGS=${PCSL_ARM_LD_FLAGS:-$ARM_LINK_FLAGS}
CLDC_USER_CFLAGS=${CLDC_USER_CFLAGS:--fpermissive -Wno-narrowing}
MIDP_CMDLINE_CFLAGS=${MIDP_CMDLINE_CFLAGS:-$ARM_CFLAGS}
MIDP_USE_MULTIPLE_ISOLATES=${MIDP_USE_MULTIPLE_ISOLATES:-false}

if [ -z "${JDK_DIR:-}" ]; then
    if [ -d "$ROOT/zulu7.24.0.1-jdk7.0.191-linux_x64" ]; then
        JDK_DIR=$ROOT/zulu7.24.0.1-jdk7.0.191-linux_x64
    elif [ -d "$ROOT/zulu7.24.0.1-ca-jdk7.0.191-linux_x64" ]; then
        JDK_DIR=$ROOT/zulu7.24.0.1-ca-jdk7.0.191-linux_x64
    else
        echo "Set JDK_DIR to a JDK 7 directory. JDK 25 is too new for this tree." >&2
        exit 1
    fi
fi

if [ -z "$CROSS_CC" ]; then
    CROSS_CC=$ARM_TRIPLET-gcc
fi
if [ -z "$CROSS_CXX" ]; then
    CROSS_CXX=$ARM_TRIPLET-g++
fi
if [ -z "$CROSS_CPP" ]; then
    CROSS_CPP=$ARM_TRIPLET-cpp
fi
if [ -z "$CROSS_AR" ]; then
    CROSS_AR=$ARM_TRIPLET-ar
fi
if [ -z "$CROSS_AS" ]; then
    CROSS_AS=$ARM_TRIPLET-as
fi
if [ -z "$CROSS_LD" ]; then
    CROSS_LD=$ARM_TRIPLET-ld
fi
if [ -z "$CROSS_RANLIB" ]; then
    CROSS_RANLIB=$ARM_TRIPLET-ranlib
fi
if [ -z "$CROSS_STRIP" ]; then
    CROSS_STRIP=$ARM_TRIPLET-strip
fi
if [ -z "$CROSS_OBJCOPY" ]; then
    CROSS_OBJCOPY=$ARM_TRIPLET-objcopy
fi
if [ -z "$CROSS_OBJDUMP" ]; then
    CROSS_OBJDUMP=$ARM_TRIPLET-objdump
fi
if [ -z "$CROSS_NM" ]; then
    CROSS_NM=$ARM_TRIPLET-nm
fi

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required command: $1" >&2
        exit 1
    fi
}

require_file() {
    if [ ! -f "$1" ]; then
        echo "Missing required file: $1" >&2
        exit 1
    fi
}

require_arm_lib() {
    lib_path=$("$CROSS_CC" -print-file-name="$1")
    if [ "$lib_path" = "$1" ]; then
        echo "Missing ARM target library: $1" >&2
        echo "Install the ARM SDL development packages, for example:" >&2
        echo "  sudo dpkg --add-architecture armhf" >&2
        echo "  sudo apt update" >&2
        echo "  sudo apt install libsdl1.2-dev:armhf libsdl-image1.2-dev:armhf libsdl-mixer1.2-dev:armhf" >&2
        exit 1
    fi
}

run_make() {
    if [ -n "$MAKE_JOBS" ]; then
        make -j "$MAKE_JOBS" "$@"
    else
        make "$@"
    fi
}

write_tool_wrapper() {
    out=$1
    target=$2
    target_path=$(command -v "$target")
    rm -f "$ARM_TOOLCHAIN_DIR/bin/$out"
    printf '%s\n' '#!/bin/sh' "exec \"$target_path\" \"\$@\"" > "$ARM_TOOLCHAIN_DIR/bin/$out"
    chmod +x "$ARM_TOOLCHAIN_DIR/bin/$out"
}

create_arm_toolchain_shim() {
    mkdir -p "$ARM_TOOLCHAIN_DIR/bin"

    write_tool_wrapper gcc "$CROSS_CC"
    write_tool_wrapper g++ "$CROSS_CXX"
    write_tool_wrapper gcc.br_real "$CROSS_CC"
    write_tool_wrapper g++.br_real "$CROSS_CXX"

    for tool_pair in \
        "cpp:$CROSS_CPP" \
        "ar:$CROSS_AR" \
        "as:$CROSS_AS" \
        "ld:$CROSS_LD" \
        "ranlib:$CROSS_RANLIB" \
        "strip:$CROSS_STRIP" \
        "objcopy:$CROSS_OBJCOPY" \
        "objdump:$CROSS_OBJDUMP" \
        "nm:$CROSS_NM"; do
        tool=${tool_pair%%:*}
        command_name=${tool_pair#*:}
        if command -v "$command_name" >/dev/null 2>&1; then
            ln -sf "$(command -v "$command_name")" "$ARM_TOOLCHAIN_DIR/bin/$tool"
        fi
    done

    require_file "$ARM_TOOLCHAIN_DIR/bin/gcc"
    require_file "$ARM_TOOLCHAIN_DIR/bin/g++"
    require_file "$ARM_TOOLCHAIN_DIR/bin/ar"
    require_file "$ARM_TOOLCHAIN_DIR/bin/as"
}

invalidate_midp_artifacts_if_cldc_changed() {
    cldc_classes_zip=$ARM_CLDC_DIST_DIR/lib/cldc_classes.zip
    midp_classes_zip=$MIDP_OUTPUT_DIR/classes.zip

    if [ ! -f "$cldc_classes_zip" ] || [ ! -f "$midp_classes_zip" ]; then
        return
    fi

    if [ "$cldc_classes_zip" -nt "$midp_classes_zip" ]; then
        echo "CLDC classes changed; invalidating MIDP classes and ROM artifacts"
        rm -f \
            "$midp_classes_zip" \
            "$MIDP_OUTPUT_DIR/ROMImage.cpp" \
            "$MIDP_OUTPUT_DIR/nativeFunctionTable.cpp" \
            "$MIDP_OUTPUT_DIR/obj/arm/ROMImage.o" \
            "$MIDP_OUTPUT_DIR/obj/arm/nativeFunctionTable.o" \
            "$MIDP_OUTPUT_DIR/bin/arm/runMidlet"
    fi
}

export MEHOME BUILD_OUTPUT_DIR JDK_DIR
export AWK=${AWK:-awk}
export PATH=$JDK_DIR/bin:$PATH
export PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl
export JVMWorkSpace=$MEHOME/cldc
export JVMBuildSpace=$BUILD_OUTPUT_DIR/cldc
export MIDP_OUTPUT_DIR=$BUILD_OUTPUT_DIR/midp
export ENABLE_COMPILATION_WARNINGS=true

require_file "$JDK_DIR/bin/java"
require_file "$JDK_DIR/bin/javac"
require_file "$JDK_DIR/jre/lib/rt.jar"
require_cmd make
require_cmd gcc
require_cmd g++
require_cmd "$CROSS_CC"
require_cmd "$CROSS_CXX"

echo "Checking JDK 7..."
"$JDK_DIR/bin/java" -version
"$JDK_DIR/bin/javac" -source 1.4 -target 1.4 -version >/dev/null

echo "Checking 32-bit host tool support..."
tmp_c=${TMPDIR:-/tmp}/phoneme-t32-$$.c
tmp_exe=${TMPDIR:-/tmp}/phoneme-t32-$$
printf '%s\n' '#include <errno.h>' 'int main(void){return 0;}' > "$tmp_c"
gcc $HOST_CFLAGS "$tmp_c" -o "$tmp_exe"
"$tmp_exe"
rm -f "$tmp_c" "$tmp_exe"

echo "Checking ARM cross compiler..."
"$CROSS_CC" -v
create_arm_toolchain_shim
require_arm_lib libSDL.so
require_arm_lib libSDL_image.so
require_arm_lib libSDL_mixer.so

mkdir -p "$BUILD_OUTPUT_DIR"

echo "Build root: $ROOT"
echo "Output:     $BUILD_OUTPUT_DIR"
echo "JDK:        $JDK_DIR"
echo "ARM ABI:    $ARM_ABI"
echo "Cross CC:   $CROSS_CC"
echo "ARM flags:  $ARM_CFLAGS"
echo "CLDC cfg:   $ARM_CLDC_PLATFORM"
echo "Tool shim:  $ARM_TOOLCHAIN_DIR"
echo "Host flags: $HOST_CFLAGS"
echo "Host inc:   ${HOST_MULTIARCH_INCLUDE:-none}"

echo "Building host PCSL..."
cd "$MEHOME/pcsl"
run_make \
    PCSL_PLATFORM=linux_i386_gcc \
    NETWORK_MODULE=bsd/generic \
    CFLAGS="$HOST_PCSL_CFLAGS" \
    LD_FLAGS="$HOST_PCSL_LD_FLAGS"

echo "Building ARM PCSL..."
cd "$MEHOME/pcsl"
run_make \
    PCSL_PLATFORM=linux_arm_gcc \
    NETWORK_MODULE=bsd/generic \
    GNU_TOOLS_DIR="$ARM_TOOLCHAIN_DIR" \
    PCSL_EXTRA_CFLAGS="$PCSL_ARM_CFLAGS" \
    PCSL_EXTRA_LD_FLAGS="$PCSL_ARM_LD_FLAGS"

echo "Building host CLDC tools and linux_i386 dist..."
cd "$JVMWorkSpace/build/linux_i386"
run_make \
    ENABLE_PCSL=true \
    PCSL_PLATFORM=linux_i386_gcc \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
    ENABLE_ISOLATES=true \
    USER_CFLAGS="$HOST_CFLAGS -fpermissive" \
    LINK_FLAGS="$HOST_LINK_FLAGS" \
    ASM_FLAGS="$HOST_ASM_FLAGS"

echo "Building ARM CLDC dist..."
cd "$JVMWorkSpace/build/$ARM_CLDC_PLATFORM"
run_make \
    ENABLE_PCSL=true \
    PCSL_PLATFORM=linux_arm_gcc \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
    ENABLE_ISOLATES=true \
    GNU_TOOLS_DIR="$ARM_TOOLCHAIN_DIR" \
    USER_CFLAGS="$CLDC_USER_CFLAGS" \
    HOST_GEN_CFLAGS="$HOST_CFLAGS -fpermissive" \
    HOST_GEN_ASM_FLAGS="$HOST_ASM_FLAGS" \
    HOST_GEN_LINK_FLAGS="$HOST_LINK_FLAGS" \
    TARGET_CFLAGS="$ARM_CFLAGS" \
    TARGET_ASM_FLAGS="$ARM_ASM_FLAGS" \
    TARGET_LINK_FLAGS="$ARM_LINK_FLAGS"

ARM_CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/$ARM_CLDC_PLATFORM/dist
require_file "$ARM_CLDC_DIST_DIR/include/jvmconfig.h"
invalidate_midp_artifacts_if_cldc_changed

echo "Building ARM MIDP SDL..."
cd "$MEHOME/midp/build/linux_sdl_gcc"
run_make \
    USE_SDL_ABB=true \
    PCSL_OUTPUT_DIR="$PCSL_OUTPUT_DIR" \
    CLDC_DIST_DIR="$ARM_CLDC_DIST_DIR" \
    TOOLS_DIR="$MEHOME/tools" \
    TARGET_CPU=arm \
    USE_MULTIPLE_ISOLATES="$MIDP_USE_MULTIPLE_ISOLATES" \
    GNU_TOOLS_DIR="$ARM_TOOLCHAIN_DIR" \
    CMDLINE_CFLAGS="$MIDP_CMDLINE_CFLAGS" \
    SDL_MIXER_EXTRA_LIBS="${SDL_MIXER_EXTRA_LIBS:-}" \
    USE_LIBFLOAT="$USE_LIBFLOAT"

echo "ARM outputs:"
find "$BUILD_OUTPUT_DIR" -type f -perm -111 -exec file {} \; | grep -E 'ARM|ELF' || true
