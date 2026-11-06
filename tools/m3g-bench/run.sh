#!/bin/sh
# Host benchmark of the M3G core + FunKey software renderer.
#
#   tools/m3g-bench/run.sh [frames] [modes]
#
# Builds the bench twice: against the working tree and against the
# ngl_software.h of a git revision (REF, default HEAD), renders each mode,
# compares the last frames and prints callgrind instruction counts.
# Modes (letters combine): n plain, f linear fog, e exp fog, l bilinear,
# m mipmaps (level NEAREST), M level LINEAR, 2 second texture unit,
# d DECAL env (RGBA texture on the cars).
# ARM=1 builds for cortex-a7+NEON with Zig (pip install ziglang) and runs
# under qemu-arm (no instruction counts; qemu timing is meaningless).
set -e
FR=${1:-20}; MODES=${2:-"n f e l lf le"}; REF=${REF:-HEAD}
D=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$D/../.." && pwd)
N=$ROOT/phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/jsr184/src/native
W=${WORK:-/tmp/m3g-bench}
mkdir -p "$W/old"
(cd "$ROOT" && git show "$REF:phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/jsr184/src/native/funkey/ngl_software.h") > "$W/old/ngl_software.h"
cp "$N/funkey/ngl.h" "$W/old/"
build() {  # build <old|new> <out>
    OV=""; [ "$1" = old ] && OV="-I$W/old"
    if [ -n "$ARM" ]; then
        python3 -m ziglang cc -target arm-linux-musleabihf -mcpu=cortex_a7 -mfpu=neon-vfpv4 \
            -mfloat-abi=hard -marm -O2 -static -DNO_ZLIB \
            -DM3G_TARGET_GENERIC -DM3G_USE_NGL_API -fno-strict-aliasing -w $OV \
            -I"$N/funkey" -I"$N/upstream/m3g/inc" -I"$N/upstream/m3g/src" \
            "$D/bench.c" "$N/upstream/m3g/src/m3g_core.c" -o "$2"
    else
        gcc -O2 -g -DM3G_TARGET_GENERIC -DM3G_USE_NGL_API -fno-strict-aliasing -w $OV \
            -I"$N/funkey" -I"$N/upstream/m3g/inc" -I"$N/upstream/m3g/src" \
            "$D/bench.c" "$N/upstream/m3g/src/m3g_core.c" -o "$2" -lz -lm
    fi
}
build old "$W/b_old"; build new "$W/b_new"
RUN=""; [ -n "$ARM" ] && RUN="qemu-arm -cpu cortex-a7"
for m in $MODES; do
    for v in old new; do
        mkdir -p "$W/r_$v"
        (cd "$W/r_$v" && $RUN ../b_$v "$FR" $m > sum.txt && mv out.raw $m.raw)
        if [ -z "$ARM" ] && command -v valgrind >/dev/null; then
            printf '%s %s: ' $m $v
            valgrind --tool=callgrind --callgrind-out-file=/dev/null "$W/b_$v" "$FR" $m 2>&1 | grep -o 'refs: *[0-9,]*'
        fi
    done
    printf '%-3s ' $m; python3 "$D/diff.py" "$W/r_old/$m.raw" "$W/r_new/$m.raw"
done
