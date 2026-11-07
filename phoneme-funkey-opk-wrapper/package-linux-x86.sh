#!/bin/bash
# Pack the desktop runtime built by phoneme-funkey-clean/build-linux-x86.sh
# into a runnable folder and a .tar.gz:
#
#   JDK_DIR=/path/to/jdk7 ./package-linux-x86.sh [out_dir]
#
# Result: <out_dir>/funkey-j2me-linux-x86/{bin,lib,appdb,run.sh,README.txt}
# and <out_dir>/funkey-j2me-linux-x86.tar.gz (default out_dir: ./dist).

set -euo pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
MEHOME=${MEHOME:-$ROOT/../phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature}
BUILD=${BUILD_OUTPUT_DIR:-$MEHOME/build_output_linux_x86}
OUT=${1:-$ROOT/dist}
NAME=funkey-j2me-linux-x86
STAGE=$OUT/$NAME
MIDP=$BUILD/midp
CLDC_DIST=$BUILD/cldc/linux_i386/dist
DOJA_STUB_SRC=$ROOT/packaging/funkey-s/doja-stubs
DOJA_API_SRC=$ROOT/packaging/funkey-s/doja-api

if [ -z "${JDK_DIR:-}" ] || [ ! -x "$JDK_DIR/bin/javac" ]; then
    echo "Set JDK_DIR to a JDK 7 directory." >&2
    exit 1
fi
if [ ! -x "$MIDP/bin/i386/runMidlet" ]; then
    echo "No runtime in $MIDP: run phoneme-funkey-clean/build-linux-x86.sh first." >&2
    exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE/bin"
cp "$MIDP/bin/i386/runMidlet" "$STAGE/bin/"
strip "$STAGE/bin/runMidlet" 2>/dev/null || true
cp -R "$MIDP/lib" "$STAGE/lib"
cp -R "$MIDP/appdb" "$STAGE/appdb"

echo "Building DoJa support (lib/doja-support.jar)"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/classes" "$TMP/verified"
find "$DOJA_STUB_SRC" "$DOJA_API_SRC" -name '*.java' | sort > "$TMP/sources.list"
"$JDK_DIR/bin/javac" -nowarn -source 1.4 -target 1.4 \
    -bootclasspath "$MIDP/classes.zip" -classpath "$MIDP/classes.zip" \
    -d "$TMP/classes" @"$TMP/sources.list" 2>&1 | grep -v '^Picked up\|^Note:\|warning' || true
# binary stubs whose class names are Java keywords
(cd "$DOJA_STUB_SRC" && find . -name '*.class' -exec cp --parents {} "$TMP/classes/" \;)
"$MIDP/bin/i386/preverify" -classpath "$MIDP/classes.zip:$CLDC_DIST/lib/cldc_classes.zip" \
    -d "$TMP/verified" "$TMP/classes"
(cd "$DOJA_STUB_SRC" && find . -name '*.bin' -exec cp --parents {} "$TMP/verified/" \;)
(cd "$TMP/verified" && "$JDK_DIR/bin/jar" cf "$STAGE/lib/doja-support.jar" .)
find "$DOJA_STUB_SRC" -maxdepth 1 -name 'NOTICE-*' -exec cp {} "$STAGE/lib/" \;

cp "$ROOT/packaging/linux-x86/run.sh" "$STAGE/run.sh"
cp "$ROOT/packaging/linux-x86/README.txt" "$STAGE/README.txt"
cp "$ROOT/packaging/linux-x86/test-games.py" "$STAGE/test-games.py"
# helper for test-games.py: scripted keys, screenshots, faster clock
${CC:-gcc} -m32 -O2 -shared -fPIC $(sdl-config --cflags) \
    "$ROOT/packaging/linux-x86/fktest.c" -o "$STAGE/lib/fktest.so" -ldl
chmod +x "$STAGE/run.sh" "$STAGE/bin/runMidlet" "$STAGE/test-games.py"

(cd "$OUT" && tar czf "$NAME.tar.gz" "$NAME")
echo "Done: $STAGE"
echo "      $OUT/$NAME.tar.gz ($(du -h "$OUT/$NAME.tar.gz" | cut -f1))"
