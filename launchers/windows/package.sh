#!/bin/bash
# Pack the Windows runtime built by scripts/build-win32.sh into
# a folder and a .zip:
#
#   JDK_DIR=/path/to/jdk7 launchers/windows/package.sh [out_dir]
#
# Result: <out_dir>/phoneme-j2me-win32/{phoneME.exe,bin,lib,appdb,
# games,README.txt} and <out_dir>/phoneme-j2me-win32.zip
# (default out_dir: out/dist). Needs i686-w64-mingw32-gcc and the MinGW
# sysroot with zlib that build-win32.sh used (MINGW_SYSROOT).

set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
BUILD=${BUILD_OUTPUT_DIR:-$ROOT/out/win32}
MINGW_SYSROOT=${MINGW_SYSROOT:-$BUILD/sysroot}
MINGW_PREFIX=${MINGW_PREFIX:-i686-w64-mingw32-}
OUT=${1:-$ROOT/out/dist}
NAME=phoneme-j2me-win32
STAGE=$OUT/$NAME
MIDP=$BUILD/midp
CLDC_DIST=$BUILD/cldc/mingw_i386/dist
DOJA_STUB_SRC=$ROOT/java/doja/stubs
DOJA_API_SRC=$ROOT/java/doja/api

if [ -z "${JDK_DIR:-}" ] || [ ! -x "$JDK_DIR/bin/javac" ]; then
    echo "Set JDK_DIR to a JDK 7 directory." >&2
    exit 1
fi
if [ ! -f "$MIDP/bin/i386/runMidlet.exe" ]; then
    echo "No runtime in $MIDP: run scripts/build-win32.sh first." >&2
    exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE/bin"
cp "$MIDP/bin/i386/runMidlet.exe" "$STAGE/bin/"
"${MINGW_PREFIX}strip" "$STAGE/bin/runMidlet.exe" 2>/dev/null || true
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
# preverify is a Linux host tool of the CLDC build
"$CLDC_DIST/bin/preverify" -classpath "$MIDP/classes.zip:$CLDC_DIST/lib/cldc_classes.zip" \
    -d "$TMP/verified" "$TMP/classes"
(cd "$DOJA_STUB_SRC" && find . -name '*.bin' -exec cp --parents {} "$TMP/verified/" \;)
(cd "$TMP/verified" && "$JDK_DIR/bin/jar" cf "$STAGE/lib/doja-support.jar" .)
find "$DOJA_STUB_SRC" -maxdepth 1 -name 'NOTICE-*' -exec cp {} "$STAGE/lib/" \;

echo "Building the launcher (phoneME.exe)"
(cd "$HERE" &&
 "${MINGW_PREFIX}windres" launcher.rc -O coff -o "$TMP/launcher.res")
"${MINGW_PREFIX}gcc" -O2 -Wall -municode -mwindows -I"$MINGW_SYSROOT/include" \
    "$HERE/launcher.c" "$TMP/launcher.res" -o "$STAGE/phoneME.exe" \
    -L"$MINGW_SYSROOT/lib" -static -lz -lcomctl32 -lcomdlg32 -lshlwapi -lole32 -lshell32 -luuid
"${MINGW_PREFIX}strip" "$STAGE/phoneME.exe" 2>/dev/null || true

# Windows line endings for the text files people open in Notepad;
# settings.ini is written by the launcher itself
sed 's/\r*$/\r/' "$HERE/README.txt" > "$STAGE/README.txt"
mkdir -p "$STAGE/games"
printf 'Put your games (.jar, i-appli .jar + .jam) here, sub-folders are fine.\r\n' \
    > "$STAGE/games/put-games-here.txt"

rm -f "$OUT/$NAME.zip"
(cd "$OUT" && python3 -c 'import os,sys,zipfile
z=zipfile.ZipFile(sys.argv[1],"w",zipfile.ZIP_DEFLATED,compresslevel=9)
for d,_,fs in os.walk(sys.argv[2]):
    for f in sorted(fs): z.write(os.path.join(d,f))' "$NAME.zip" "$NAME")
echo "Done: $STAGE"
echo "      $OUT/$NAME.zip ($(du -h "$OUT/$NAME.zip" | cut -f1))"
