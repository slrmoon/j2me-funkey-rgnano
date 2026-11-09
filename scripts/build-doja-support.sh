#!/bin/sh
# Builds lib/doja-support.jar (the i-appli/DoJa API on top of MIDP) for a
# runtime build: build-doja-support.sh <midp output dir> <cldc dist dir> <out.jar>
# Uses JDK_DIR. (The Linux and Windows packages do the same in their
# package.sh.)
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MIDP=$1 CLDC_DIST=$2
OUT=$(cd "$(dirname "$3")" && pwd)/$(basename "$3")
STUBS=$ROOT/java/doja/stubs API=$ROOT/java/doja/api
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/classes" "$TMP/verified"
find "$STUBS" "$API" -name '*.java' | sort > "$TMP/sources.list"
"$JDK_DIR/bin/javac" -nowarn -source 1.4 -target 1.4 \
    -bootclasspath "$MIDP/classes.zip" -classpath "$MIDP/classes.zip" \
    -d "$TMP/classes" @"$TMP/sources.list" 2>&1 | grep -v '^Picked up\|^Note:\|warning' || true
# binary stubs whose class names are Java keywords
(cd "$STUBS" && find . -name '*.class' -exec cp --parents {} "$TMP/classes/" \;)
"$MIDP/bin/i386/preverify" -classpath "$MIDP/classes.zip:$CLDC_DIST/lib/cldc_classes.zip" \
    -d "$TMP/verified" "$TMP/classes"
(cd "$STUBS" && find . -name '*.bin' -exec cp --parents {} "$TMP/verified/" \;)
(cd "$TMP/verified" && "$JDK_DIR/bin/jar" cf "$OUT" .)
find "$STUBS" -maxdepth 1 -name 'NOTICE-*' -exec cp {} "$(dirname "$OUT")/" \;
