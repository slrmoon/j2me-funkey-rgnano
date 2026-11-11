#!/bin/bash
# Sets up a fresh cloud container (Ubuntu 24.04, root) for every target -
# FunKey, Linux x86, Windows, browser - and restores the compiler cache, so
# a full build mostly takes results from the cache instead of compiling.
#
#   tools/cloud-setup.sh            packages, JDK, FunKey SDK, cache
#   source /root/tc/env.sh          then ./build.sh <target> as usual
#   tools/cloud-cache-save.sh       after building: upload the new cache
#
# The cache is kept in a private repository of its own (CACHE_REPO, default
# slrmoon/j2me-build-cache): build-cache.tar.zst.NN, one commit, replaced on
# every save. A Claude session needs that repository attached to it.
#
# What the cache holds: ccache's directory (results of every compiler run:
# gcc, the MinGW and FunKey cross compilers, emcc). Make itself rebuilds
# everything in a fresh clone (new file dates), but each compiler call is
# answered from the cache when its input is the same.
#
# Options (environment): TC=/root/tc (tools and env.sh), NO_CACHE=1 (do not
# download the cache), CACHE_REPO=slrmoon/j2me-build-cache.

set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TC=${TC:-/root/tc}
CACHE_REPO=${CACHE_REPO:-slrmoon/j2me-build-cache}
JDK=zulu7.24.0.1-jdk7.0.191-linux_x64
JDK_URL=https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz
SDK=FunKey-sdk-2.3.0
SDK_URL=https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz
mkdir -p "$TC"
LOG=$TC/setup.log
: > "$LOG"
step() { echo "== $*"; }

step "packages"
if ! dpkg --print-foreign-architectures | grep -q i386; then
    dpkg --add-architecture i386
fi
apt-get update -qq >>"$LOG" 2>&1
DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends \
    gcc g++ gcc-multilib g++-multilib libc6-dev-i386 make file git curl \
    zstd ccache squashfs-tools python3 \
    zlib1g-dev:i386 libsdl1.2-dev:i386 libsdl-image1.2-dev:i386 \
    libsdl-gfx1.2-dev:i386 \
    mingw-w64 \
    emscripten >>"$LOG" 2>&1

step "JDK 7"
if [ ! -x "$TC/$JDK/bin/javac" ]; then
    curl -sSfL "$JDK_URL" | tar -xz -C "$TC"
fi

step "FunKey SDK"
if [ ! -d "$TC/$SDK" ]; then
    curl -sSfL "$SDK_URL" | tar -xz -C "$TC"
    (cd "$TC/$SDK" && ./relocate-sdk.sh >>"$LOG" 2>&1)
fi

step "fmod shim (Ubuntu 24.04's i386 libm has no fmod for romgen)"
gcc -m32 -O2 -c "$ROOT/tools/cloud-test/fmodshim.c" -o "$TC/fmodshim.o"

step "ccache wrappers"
# compilers looked up by name in PATH go through ccache (the FunKey SDK's
# directory is put in front of PATH by its environment-setup:
# scripts/build-funkey.sh puts CCACHE_WRAPPERS back in front)
W=$TC/ccache-bin
mkdir -p "$W"
for c in gcc g++ cc c++ i686-w64-mingw32-gcc i686-w64-mingw32-g++ \
         arm-funkey-linux-musleabihf-gcc arm-funkey-linux-musleabihf-g++; do
    ln -sf "$(command -v ccache)" "$W/$c"
done

cat > "$TC/env.sh" <<EOF
# made by tools/cloud-setup.sh: source it before ./build.sh
export JDK_DIR=$TC/$JDK
export FUNKEY_SDK_DIR=$TC/$SDK
export HOST_LINK_EXTRA=$TC/fmodshim.o
export HOST_LINK_FLAGS="-m32 -pthread $TC/fmodshim.o"
export CCACHE_DIR=$TC/ccache
export CCACHE_BASEDIR=$ROOT
export CCACHE_COMPILERCHECK=content
export CCACHE_MAXSIZE=3G
export CCACHE_WRAPPERS=$W
export EM_COMPILER_WRAPPER=ccache
case ":\$PATH:" in *":$W:"*) ;; *) export PATH=$W:\$PATH ;; esac
EOF

if [ -z "${NO_CACHE:-}" ]; then
    step "compiler cache ($CACHE_REPO)"
    rm -rf "$TC/cache-repo"
    if git clone -q --depth 1 "https://github.com/$CACHE_REPO" "$TC/cache-repo" \
            >>"$LOG" 2>&1 && ls "$TC"/cache-repo/build-cache.tar.zst.* >/dev/null 2>&1; then
        cat "$TC"/cache-repo/build-cache.tar.zst.* | zstd -dc | tar -x -C "$TC"
        . "$TC/env.sh"
        ccache -s | grep -i "cache size" || true
    else
        echo "no cache (repository $CACHE_REPO not reachable or empty)"
    fi
fi

step "done: source $TC/env.sh, then ./build.sh <target>"
