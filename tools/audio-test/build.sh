#!/bin/bash
# Builds fkrender: the runtime's audio engine (fkaudio + Sonivox EAS) as a
# desktop program that renders one media file to a WAV, off line.
#   tools/audio-test/build.sh [output dir]      (default /tmp/audio-test)
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
N=$ROOT/runtime/midp/src/media-sdl/reference/native
OUT=${1:-/tmp/audio-test}
mkdir -p "$OUT"
EAS=$(sed -n '/^FKA_EAS_FILES/,/^$/p' "$ROOT/runtime/midp/src/media-sdl/lib.gmk" | grep '\.c' | sed 's/\\//;s/ //g' | sed "s|^|$N/sonivox/|")
gcc -O2 -std=gnu99 -fno-strict-aliasing -w -I"$N/fkaudio" -I"$N/sonivox" -I"$N/sonivox/fakes" \
    -o "$OUT/fkrender" "$ROOT/tools/audio-test/fkrender.c" \
    "$N"/fkaudio/fka_engine.c "$N"/fkaudio/fka_song.c "$N"/fkaudio/fka_formats.c "$N"/fkaudio/fka_smaf.c \
    "$N"/fkaudio/fka_decode.c "$N"/fkaudio/fka_resample.c "$N"/fkaudio/fka_mld.c "$N"/fkaudio/fka_eas.c \
    $EAS -lm -lpthread -lz
echo "built $OUT/fkrender"
