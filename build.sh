#!/bin/bash
# Builds the runtime for one target and, with "package", its launcher package.
#
#   ./build.sh funkey  [package]   FunKey S / RG Nano (ARM): out/funkey, OPK
#   ./build.sh linux   [package]   desktop Linux, 32-bit x86: out/linux-x86
#   ./build.sh windows [package]   Windows, MinGW-w64 cross build: out/win32
#   ./build.sh web                 WebAssembly VM (work in progress): out/web
#
# Needs a JDK 7 (JDK_DIR, or zulu7*-linux_x64 unpacked in the repository
# root). FunKey: the FunKey SDK (FUNKEY_SDK_DIR, or FunKey-sdk-2.3.0 here).
# Packages land in out/funkey/opk/pm.opk and out/dist/.
# Host packages and options: docs/building.md; each step is a script in
# scripts/ (build) or launchers/<target>/package.sh (package).

set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd)

if [ -z "${JDK_DIR:-}" ]; then
    for d in "$ROOT"/zulu7*-linux_x64; do
        [ -x "$d/bin/javac" ] && export JDK_DIR=$d && break
    done
fi

usage() {
    sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
}

[ $# -ge 1 ] || usage
target=$1
package=${2:-}
[ -z "$package" ] || [ "$package" = package ] || usage

case "$target" in
    funkey)
        export FUNKEY_SDK_DIR=${FUNKEY_SDK_DIR:-$ROOT/FunKey-sdk-2.3.0}
        "$ROOT/scripts/build-funkey.sh"
        if [ -n "$package" ]; then
            FUNKEY_CC=${FUNKEY_CC:-$FUNKEY_SDK_DIR/bin/arm-funkey-linux-musleabihf-gcc} \
                "$ROOT/launchers/funkey/package.sh"
        fi
        ;;
    linux)
        "$ROOT/scripts/build-linux-x86.sh"
        [ -z "$package" ] || "$ROOT/launchers/linux/package.sh"
        ;;
    windows)
        "$ROOT/scripts/build-win32.sh"
        [ -z "$package" ] || "$ROOT/launchers/windows/package.sh"
        ;;
    web)
        "$ROOT/scripts/build-web.sh"
        ;;
    *)
        usage
        ;;
esac
