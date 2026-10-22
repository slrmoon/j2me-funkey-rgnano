#!/bin/bash
# Reference renderer #2 (the one fka_mld.c now matches bit for bit): the MLD
# sequencer + MA-3 sampler as fixed in openDoJa (GrenderG/openDoJa, GPLv3;
# only used here as a test oracle, nothing of it is shipped). Its FueTrek
# engine (a ROM extracted from a proprietary synth) is not built.
# usage: build-opendoja.sh [workdir]
#   java -cp <workdir>/out ORender in.mld out.wav [rate] [maxsec] [loop 0|1]
#   java -cp <workdir>/out TRender ...   same, sampler calls traced on stderr
#   (C side: gcc -DFKA_MLD_TRACE ... render.c fka_mld.c, see tcmp.sh)
set -e
H=$(cd "$(dirname "$0")" && pwd)
W=${1:-/tmp/odref}
JDK=${JDK:-/usr/lib/jvm/java-21-openjdk-amd64}
mkdir -p "$W"
if [ ! -d "$W/openDoJa" ]; then
    git clone -q --depth 1 --filter=blob:none --sparse https://github.com/GrenderG/openDoJa.git "$W/openDoJa"
    git -C "$W/openDoJa" sparse-checkout set src/main/java/opendoja/audio/mld src/main/resources/opendoja/audio/mld
fi
S=$W/openDoJa/src/main/java/opendoja/audio/mld
rm -rf "$W/out" "$W/stub" && mkdir -p "$W/out" "$W/stub/opendoja/host"
cat > "$W/stub/opendoja/host/DoJaEncoding.java" <<'J'
package opendoja.host;
public final class DoJaEncoding { public static final java.nio.charset.Charset DEFAULT_CHARSET = java.nio.charset.Charset.forName("Shift_JIS"); }
J
"$JDK/bin/javac" -nowarn -d "$W/out" $(ls "$S"/*.java "$S"/ma3/*.java | grep -v 'MLDPCMPlayer\|MLDSynth') \
    "$W/stub/opendoja/host/DoJaEncoding.java"
cp -r "$W/openDoJa/src/main/resources/opendoja" "$W/out/"
"$JDK/bin/javac" -nowarn -cp "$W/out" -d "$W/out" "$H/ORender.java" "$H/TRender.java" "$H/LogSampler.java"
echo "built $W/out"
