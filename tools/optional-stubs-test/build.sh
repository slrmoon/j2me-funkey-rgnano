#!/bin/sh
# Builds StubTest.jar: checks that the optional JSR stubs (JSR 75, 82, 179,
# 226, 239) load and refuse the way games expect; prints "StubTest: PASS".
#
#   JDK_DIR=/path/to/jdk7 BUILD_OUTPUT_DIR=.../build_output_xxx tools/optional-stubs-test/build.sh [out.jar]
set -e
D=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-$PWD/StubTest.jar}
M=$BUILD_OUTPUT_DIR/midp
PV=$(ls "$M"/bin/*/preverify 2>/dev/null | head -n 1)
[ -x "$JDK_DIR/bin/javac" ] || { echo "Set JDK_DIR to a JDK 7." >&2; exit 1; }
[ -f "$M/classes.zip" ] && [ -n "$PV" ] || { echo "Set BUILD_OUTPUT_DIR to a runtime build." >&2; exit 1; }
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
mkdir -p "$W/cls" "$W/pv"
"$JDK_DIR/bin/javac" -nowarn -source 1.4 -target 1.4 -bootclasspath "$M/classes.zip" \
    -d "$W/cls" "$D/StubTest.java"
"$PV" -classpath "$M/classes.zip" -d "$W/pv" "$W/cls"
printf 'MIDlet-1: StubTest,,StubTest\nMIDlet-Name: StubTest\nMIDlet-Vendor: test\nMIDlet-Version: 1.0\nMicroEdition-Profile: MIDP-2.0\nMicroEdition-Configuration: CLDC-1.1\n' > "$W/man.txt"
"$JDK_DIR/bin/jar" cfm "$OUT" "$W/man.txt" -C "$W/pv" .
echo "Built $OUT"
