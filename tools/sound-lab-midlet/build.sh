#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO=$(CDPATH= cd -- "$ROOT/../.." && pwd)
JDK="$REPO/phoneme-funkey-clean/zulu7.24.0.1-jdk7.0.191-linux_x64/bin"
MIDP_CLASSES="$REPO/phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/midp/classes.zip"
RAW="$ROOT/build/raw"
BUILD="$ROOT/build/classes"
JAR="$ROOT/build/SoundLab.jar"
PREVERIFY="$REPO/phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/midp/bin/arm/preverify"

rm -rf "$ROOT/build"
mkdir -p "$RAW" "$BUILD" "$ROOT/build"

python3 "$ROOT/make_resources.py"

"$JDK/javac" -source 1.3 -target 1.1 \
  -bootclasspath "$MIDP_CLASSES" \
  -d "$RAW" \
  "$ROOT/src/SoundLab.java"

"$PREVERIFY" -classpath "$MIDP_CLASSES:$RAW" -d "$BUILD" "$RAW"

cp "$ROOT/res/"* "$BUILD/"
cat > "$ROOT/build/MANIFEST.MF" <<EOF
Manifest-Version: 1.0
MIDlet-Name: SoundLab
MIDlet-Version: 1.0.0
MIDlet-Vendor: Codex
MIDlet-1: SoundLab,,SoundLab
MicroEdition-Profile: MIDP-2.0
MicroEdition-Configuration: CLDC-1.1
EOF

(cd "$BUILD" && "$JDK/jar" cfm "$JAR" "$ROOT/build/MANIFEST.MF" .)
echo "$JAR"
