#!/usr/bin/env bash
set -euo pipefail

snapshot_url=${C55X_BINUTILS_SNAPSHOT_URL:-https://sourceforge.net/code-snapshots/cvs/c/c5/c55x-binutils.zip}
root=${C55X_BINUTILS_BUILD_ROOT:-/tmp/c55x-binutils}
archive="$root/c55x-binutils.zip"
snapshot="$root/snapshot"
source="$root/source"
build="$root/build"

rm -rf "$root"
mkdir -p "$snapshot" "$source" "$build"
curl -L --fail --silent --show-error "$snapshot_url" -o "$archive"
unzip -q "$archive" -d "$snapshot"

python3 - "$snapshot/c55x-binutils/binutils" "$source" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1])
output = Path(sys.argv[2])


def extract_head(blob: bytes) -> bytes:
    marker = b"\ntext\n@"
    position = blob.find(marker)
    if position < 0:
        raise ValueError("missing RCS head text marker")
    position += len(marker)
    result = bytearray()
    while position < len(blob):
        byte = blob[position]
        if byte == ord("@"):
            if position + 1 < len(blob) and blob[position + 1] == ord("@"):
                result.append(byte)
                position += 2
                continue
            return bytes(result)
        result.append(byte)
        position += 1
    raise ValueError("unterminated RCS head text")


count = 0
for path in source.rglob("*,v"):
    relative = path.relative_to(source)
    target = output / str(relative)[:-2]
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(extract_head(path.read_bytes()))
    if path.stat().st_mode & 0o111:
        target.chmod(0o755)
    count += 1
print(f"extracted {count} RCS head revisions")
PY

cd "$build"
CFLAGS="-O2 -std=gnu89 -Wno-error=implicit-int" \
    "$source/configure" --target=tic55x-coff --disable-nls >configure.log 2>&1
make -j2 all-bfd all-opcodes >make-libs.log 2>&1
make -C binutils -j2 objdump >make-objdump.log 2>&1

echo "$build/binutils/objdump"
