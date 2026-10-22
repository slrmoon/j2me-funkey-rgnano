#!/bin/bash
# Reference MLD renderer: the Keitai Wiki MLD sequencer + Yamaha MA-3 FM synth
# (public domain / MPL-2.0, maintained in SquirrelJME) run on a desktop JDK.
# usage: build.sh [workdir]; then: java -cp <workdir>/out Render in.mld out.wav [rate] [maxsec]
set -e
H=$(cd "$(dirname "$0")" && pwd)
W=${1:-/tmp/mldref}
JDK=${JDK:-/usr/lib/jvm/java-21-openjdk-amd64}
mkdir -p "$W"
if [ ! -d "$W/sqj" ]; then
    git clone -q --depth 1 --filter=blob:none --sparse https://github.com/SquirrelJME/SquirrelJME.git "$W/sqj"
    git -C "$W/sqj" sparse-checkout set modules/vendor-api-keitaiwiki-music
fi
M=$W/sqj/modules/vendor-api-keitaiwiki-music/src/main
rm -rf "$W/out" && mkdir -p "$W/out/com/keitaiwiki/music"
"$JDK/bin/javac" -nowarn -d "$W/out" $(find "$M/java/com" "$H/shims" -name '*.java') "$H/Render.java"
for f in "$M"/resources/com/keitaiwiki/music/*.__mime; do
    b=$(basename "$f" .__mime)
    tail -n +2 "$f" | grep -v '^====$' | base64 -d > "$W/out/com/keitaiwiki/music/$b"
done
echo "built $W/out"
