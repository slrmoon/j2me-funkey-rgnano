#!/bin/bash
# Stress tests of the browser JIT (runtime/cldc/src/vm/cpu/c/WasmJit_c.hpp),
# run in node on the CLDC VM of the web build (./build.sh web first):
#
#   tools/jit-test/run.sh [seconds]
#
# Each test checks its own results and runs with the interpreter and with
# the JIT at several compile thresholds; any FAIL, exception, crash or
# timeout is an error.
#   StreamTest  tile streaming as in an open-world game: loader threads,
#               wait/notify queues, object arrays, garbage collection
#   LockTest    contended synchronized methods while threads start all the
#               time (compiled code once ran on a foreign thread's frame)
#   StackTest   deep recursion (stack growth), exceptions and finally
#               through compiled frames
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
D=$ROOT/out/web/cldc/emscripten_c/dist
JDK=${JDK_DIR:?set JDK_DIR (JDK 7)}
SECS=${1:-8}
W=$(mktemp -d)
mkdir "$W/cls" "$W/pv"
"$JDK/bin/javac" -nowarn -source 1.3 -target 1.3 -bootclasspath "$D/lib/cldc_classes.zip" \
    -d "$W/cls" "$ROOT"/tools/jit-test/*.java 2>&1 | grep -v '^Picked up\|^Note:\|warning' || true
"$D/bin/preverify" -classpath "$D/lib/cldc_classes.zip" -d "$W/pv" "$W/cls"
bad=0
cd "$W/pv"
for t in StreamTest LockTest StackTest; do
    for cfg in "0 400" "1 400" "1 20" "1 1"; do
        set -- $cfg
        if PHONEME_JIT=$1 PHONEME_JIT_THRESHOLD=$2 timeout $((SECS * 4 + 20)) \
               node --no-experimental-fetch "$D/bin/cldc_vm.js" -classpath . $t "$SECS" > "$W/out.txt" 2>&1 \
           && grep -q '^done' "$W/out.txt" \
           && ! cut -c1-200 "$W/out.txt" | grep -q 'FAIL\|Uncaught\|RuntimeError'; then
            echo "ok    $t jit=$1 threshold=$2: $(grep '^done' "$W/out.txt")"
        else
            echo "ERROR $t jit=$1 threshold=$2"
            cut -c1-200 "$W/out.txt" | grep -v '^var Module' | head -8
            bad=1
        fi
    done
done
rm -rf "$W"
exit $bad
