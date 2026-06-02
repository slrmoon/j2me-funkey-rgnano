#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 4 ]]; then
    echo "usage: $0 MODULE.bin [OUTPUT.asm] [START_OFFSET] [STOP_OFFSET]" >&2
    exit 2
fi

module=$1
output=${2:-"${module%.bin}.asm"}
start=${3:-0x110}
stop=${4:-}
objdump=${C55X_OBJDUMP:-/tmp/c55x-binutils/build/binutils/objdump}

if [[ ! -x "$objdump" ]]; then
    echo "C55x objdump not found: $objdump" >&2
    echo "run tools/build_c55x_binutils.sh or set C55X_OBJDUMP" >&2
    exit 1
fi

args=(-D -b binary -m tms320c55x "--start-address=$start")
if [[ -n "$stop" ]]; then
    args+=("--stop-address=$stop")
fi

"$objdump" "${args[@]}" "$module" >"$output"
echo "$output"
