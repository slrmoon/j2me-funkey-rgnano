#!/usr/bin/env python3
"""Generate a minimal Java .class file for the obfuscated class 'do'.

The class name 'do' conflicts with Java's 'do' keyword,
so it cannot be written as a .java source file.
We generate the bytecode directly.

Class: com.mascotcapsule.eruption.docomostar.do
Superclass: java.lang.Object
No methods, no fields — minimal stub.
"""

import struct
import os

def u1(value):
    return struct.pack('>B', value & 0xFF)

def u2(value):
    return struct.pack('>H', value & 0xFFFF)

def u4(value):
    return struct.pack('>I', value & 0xFFFFFFFF)

def build_class():
    # Constant pool
    cp = []
    cp_utf8 = {}  # value -> index

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

    # 1: Utf8 "com/mascotcapsule/eruption/docomostar/do"
    utf8_this = add_utf8('com/mascotcapsule/eruption/docomostar/do')
    # 2: Class "com/mascotcapsule/eruption/docomostar/do"
    cls_this = add_class('com/mascotcapsule/eruption/docomostar/do')
    # 3: Utf8 "java/lang/Object"
    utf8_obj = add_utf8('java/lang/Object')
    # 4: Class "java/lang/Object"
    cls_obj = add_class('java/lang/Object')
    # 5: Utf8 "Code"
    utf8_code = add_utf8('Code')
    # 6: Utf8 "com/mascotcapsule/eruption/docomostar/do"
    # Already added (set to class name)
    # For the SourceFile attribute:
    # (skip SourceFile for simplicity — not needed at CLDC 1.1 level)

    # Write the class
    buf = bytearray()

    # Magic number 0xCAFEBABE
    buf += u4(0xCAFEBABE)
    # Minor version 0
    buf += u2(0)
    # Major version 45 (Java 1.1 — CLDC baseline)
    buf += u2(45)

    # Constant pool count
    buf += u2(len(cp) + 1)

    for tag, val in cp:
        if tag == 1:  # Utf8
            data = val.encode('utf-8')
            buf += u1(1)
            buf += u2(len(data))
            buf += data
        elif tag == 7:  # Class
            buf += u1(7)
            buf += u2(val)

    # Access flags: public
    buf += u2(0x0001)
    # This class
    buf += u2(cls_this)
    # Super class
    buf += u2(cls_obj)
    # Interfaces count: 0
    buf += u2(0)
    # Fields count: 0
    buf += u2(0)
    # Methods count: 0
    buf += u2(0)
    # Attributes count: 0
    buf += u2(0)

    return bytes(buf)


def main():
    out_dir = '/home/pq/funkey-github-ready/phoneme-funkey-opk-wrapper/packaging/funkey-s/doja-stubs/com/mascotcapsule/eruption/docomostar'
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'do.class')

    class_bytes = build_class()
    with open(out_path, 'wb') as f:
        f.write(class_bytes)

    print(f"Generated {out_path} ({len(class_bytes)} bytes)")

if __name__ == '__main__':
    main()
