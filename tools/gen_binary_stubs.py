#!/usr/bin/env python3
"""Generate minimal .class stubs for all obfuscated mascotcapsule classes.

Some class names are Java keywords or reserved words, so they can't be
written as .java source files. This generates binary .class files directly.
"""

import os
import sys
import struct
import zipfile

OUT_DIR = '/home/pq/funkey-github-ready/phoneme-funkey-opk-wrapper/packaging/funkey-s/doja-stubs/com/mascotcapsule/eruption/docomostar'
PKG_INTERNAL = 'com/mascotcapsule/eruption/docomostar'

def u1(value):
    return struct.pack('>B', value & 0xFF)

def u2(value):
    return struct.pack('>H', value & 0xFFFF)

def u4(value):
    return struct.pack('>I', value & 0xFFFFFFFF)


def build_minimal_class(class_name_internal):
    """Build a minimal public class with no methods/fields."""
    cp = []
    cp_utf8 = {}

    def add_utf8(s):
        if s in cp_utf8:
            return cp_utf8[s]
        idx = len(cp) + 1
        cp.append((1, s))
        cp_utf8[s] = idx
        return idx

    def add_class(name):
        name_idx = add_utf8(name)
        idx = len(cp) + 1
        cp.append((7, name_idx))
        return idx

    # Short class name
    short_name = class_name_internal.split('/')[-1]

    add_utf8(short_name)
    cls_this = add_class(class_name_internal)
    add_utf8('java/lang/Object')
    cls_obj = add_class('java/lang/Object')

    # Build bytecode
    buf = bytearray()
    buf += u4(0xCAFEBABE)
    buf += u2(0)    # minor version
    buf += u2(45)   # major version (Java 1.1/CLDC 1.0 compatible)

    buf += u2(len(cp) + 1)  # cp_count

    for tag, val in cp:
        if tag == 1:
            data = val.encode('utf-8')
            buf += u1(1)
            buf += u2(len(data))
            buf += data
        elif tag == 7:
            buf += u1(7)
            buf += u2(val)

    # Access flags: ACC_PUBLIC
    buf += u2(0x0001)
    # This class
    buf += u2(cls_this)
    # Super class
    buf += u2(cls_obj)
    # Interfaces count
    buf += u2(0)
    # Fields count
    buf += u2(0)
    # Methods count
    buf += u2(0)
    # Attributes count
    buf += u2(0)

    return bytes(buf)


def generate_all():
    """Read the full diff and generate binary stubs for all obfuscated mascotcapsule classes."""

    # List of obfuscated class names to generate
    # Single letters a-z (except those that already exist as Java files or binary)
    obfuscated_names = set()

    # Single letters
    for c in range(ord('a'), ord('z') + 1):
        obfuscated_names.add(chr(c))

    # Double letters aa-zz
    for c1 in range(ord('a'), ord('z') + 1):
        for c2 in range(ord('a'), ord('z') + 1):
            obfuscated_names.add(chr(c1) + chr(c2))

    # Numbers (just in case)
    for c in range(ord('0'), ord('9') + 1):
        obfuscated_names.add(chr(c))

    # Existing meaningful Java source files that we already have
    existing_java = set()
    for f in os.listdir(OUT_DIR):
        if f.endswith('.java'):
            existing_java.add(f[:-5])  # name without .java

    # Also check which .class files already exist (the meaningful compiled ones)
    existing_class = set()
    # We can't easily check .class from java files, but we know them

    # Generate binary stubs
    generated = 0
    skipped = 0

    for name in sorted(obfuscated_names):
        # Skip names that have .java files
        if name in existing_java:
            skipped += 1
            continue

        # Skip 'do' — already generated
        out_path = os.path.join(OUT_DIR, f'{name}.class')
        if os.path.exists(out_path) and name == 'do':
            skipped += 1
            continue

        # Build the class
        internal_name = f'{PKG_INTERNAL}/{name}'
        class_bytes = build_minimal_class(internal_name)

        with open(out_path, 'wb') as f:
            f.write(class_bytes)

        generated += 1
        if generated <= 10:
            print(f"  Generated: {internal_name} -> {out_path} ({len(class_bytes)} bytes)")

    print(f"\nGenerated {generated} binary stubs, skipped {skipped} (have .java source)")


if __name__ == '__main__':
    generate_all()
