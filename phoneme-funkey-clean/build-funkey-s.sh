#!/bin/sh

if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

FUNKEY_SDK_DIR="$(cd "$(dirname "$0")" && pwd)/FunKey-sdk-2.3.0"

if [ ! -d "$FUNKEY_SDK_DIR" ]; then
    echo "FunKey SDK not found: $FUNKEY_SDK_DIR" >&2
    exit 1
fi

echo "Using FunKey SDK: $FUNKEY_SDK_DIR"

if [ -x "$FUNKEY_SDK_DIR/relocate-sdk.sh" ]; then
    echo "Relocating/checking FunKey SDK..."
    "$FUNKEY_SDK_DIR/relocate-sdk.sh"
fi

ENV_SETUP=$(find -L "$FUNKEY_SDK_DIR" -maxdepth 2 -type f -name 'environment-setup*' | head -n 1)
if [ -z "$ENV_SETUP" ]; then
    echo "Could not find environment-setup in FUNKEY_SDK_DIR=$FUNKEY_SDK_DIR" >&2
    exit 1
fi

echo "Sourcing FunKey SDK environment: $ENV_SETUP"
# shellcheck disable=SC1090
. "$ENV_SETUP"
echo "FunKey SDK environment loaded."

if [ -z "${CC:-}" ] || [ -z "${CXX:-}" ]; then
    echo "FunKey SDK environment did not set CC/CXX." >&2
    exit 1
fi

CROSS_CC=${CC%% *}
CROSS_CXX=${CXX%% *}
CROSS_CPP=${CPP:-}
CROSS_CPP=${CROSS_CPP%% *}
CROSS_AR=${AR:-}
CROSS_AS=${AS:-}
CROSS_LD=${LD:-}
CROSS_RANLIB=${RANLIB:-}
CROSS_STRIP=${STRIP:-}
CROSS_OBJCOPY=${OBJCOPY:-}
CROSS_OBJDUMP=${OBJDUMP:-}
CROSS_NM=${NM:-}

echo "SDK CC: $CROSS_CC"
echo "SDK CXX: $CROSS_CXX"
CROSS_CC_PATH=$(command -v "$CROSS_CC")
if [ -L "$CROSS_CC_PATH" ]; then
    CROSS_CC_REAL=$(readlink -f "$CROSS_CC_PATH")
    if [ "$(basename "$CROSS_CC_REAL")" = "toolchain-wrapper" ] &&
       head -n 1 "$CROSS_CC_REAL" 2>/dev/null | grep -q '^#!/bin/sh'; then
        echo "FunKey SDK toolchain-wrapper looks damaged: $CROSS_CC_REAL" >&2
        echo "Re-extract the FunKey SDK, then run this script again." >&2
        exit 1
    fi
fi
ARM_TRIPLET=$("$CROSS_CC" -dumpmachine)
echo "SDK target triplet: $ARM_TRIPLET"

SDK_TARGET_FLAGS=${CFLAGS:-}
SDK_TARGET_LDFLAGS=${LDFLAGS:-}

if command -v sdl-config >/dev/null 2>&1; then
    export SDL_CONFIG=$(command -v sdl-config)
fi

export ARM_TRIPLET
export ARM_ABI=hard
export ARM_CLDC_PLATFORM=${ARM_CLDC_PLATFORM:-linux_arm_vfp}
export ARM_TOOLCHAIN_DIR=${ARM_TOOLCHAIN_DIR:-$ROOT/.toolchains/funkey-s}
export CROSS_CC CROSS_CXX CROSS_CPP CROSS_AR CROSS_AS CROSS_LD
export CROSS_RANLIB CROSS_STRIP CROSS_OBJCOPY CROSS_OBJDUMP CROSS_NM
export ARM_CFLAGS=${ARM_CFLAGS:-"$SDK_TARGET_FLAGS -std=gnu89 -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard"}
export ARM_ASM_FLAGS=${ARM_ASM_FLAGS:-"-march=armv7-a -mfpu=vfpv3-d16"}
export ARM_LINK_FLAGS=${ARM_LINK_FLAGS:-"$ARM_CFLAGS $SDK_TARGET_LDFLAGS"}
export MIDP_CMDLINE_CFLAGS=${MIDP_CMDLINE_CFLAGS:-"$ARM_CFLAGS"}
export BUILD_OUTPUT_DIR=${BUILD_OUTPUT_DIR:-$ROOT/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s}
export USE_LIBFLOAT=false
export GCC_VERSION=

echo "Build output: $BUILD_OUTPUT_DIR"
echo "Handing off to build-ubuntu26-armv7.sh..."
exec "$ROOT/build-ubuntu26-armv7.sh"
