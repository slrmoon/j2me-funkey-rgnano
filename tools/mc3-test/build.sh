#!/bin/sh
# Builds MC3Test.jar: a MascotCapsule Micro3D v3 smoke test (animated lit
# figure, primitives, point sprites, a translated/clipped corner view).
#
#   JDK_DIR=/path/to/jdk7 BUILD_OUTPUT_DIR=out/<target> tools/mc3-test/build.sh [out.jar]
#
# BUILD_OUTPUT_DIR: a runtime build (its midp/classes.zip and preverify).
set -e
D=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-$PWD/MC3Test.jar}
M=$BUILD_OUTPUT_DIR/midp
PV=$(ls "$M"/bin/*/preverify 2>/dev/null | head -n 1)
[ -x "$JDK_DIR/bin/javac" ] || { echo "Set JDK_DIR to a JDK 7." >&2; exit 1; }
[ -f "$M/classes.zip" ] && [ -n "$PV" ] || { echo "Set BUILD_OUTPUT_DIR to a runtime build." >&2; exit 1; }
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
mkdir -p "$W/res" "$W/cls" "$W/pv"
python3 "$D/gen.py" "$W/res" >/dev/null
"$JDK_DIR/bin/javac" -nowarn -source 1.4 -target 1.4 -bootclasspath "$M/classes.zip" \
    -d "$W/cls" "$D/MC3Test.java"
"$PV" -classpath "$M/classes.zip" -d "$W/pv" "$W/cls"
cp "$W"/res/* "$W/pv/"
printf 'MIDlet-1: MC3Test,,MC3Test\nMIDlet-Name: MC3Test\nMIDlet-Vendor: test\nMIDlet-Version: 1.0\nMicroEdition-Profile: MIDP-2.0\nMicroEdition-Configuration: CLDC-1.1\n' > "$W/man.txt"
"$JDK_DIR/bin/jar" cfm "$OUT" "$W/man.txt" -C "$W/pv" .
echo "Built $OUT"
