#!/usr/bin/env python3
"""Test data for the MascotCapsule v3 MIDP test: an MBAC v4 model (two
textured cubes, normals, two bones), an MTRA action (bone 1 spins) and an
8-bit BMP texture whose palette index 0 is the color key."""
import struct, sys, os

out = sys.argv[1]
os.makedirs(out, exist_ok=True)

# ---------------------------------------------------------------- model
# cube corners and faces (quads, corner order 0 1 2 3 drawn as strip
# 0-1-2, 2-1-3): each face gets its own 4 vertices for flat normals
S = 300
faces = [  # normal, 4 corners (strip order)
    ((0, 0, -1), [(-1, -1, -1), (1, -1, -1), (-1, 1, -1), (1, 1, -1)]),
    ((0, 0, 1), [(1, -1, 1), (-1, -1, 1), (1, 1, 1), (-1, 1, 1)]),
    ((-1, 0, 0), [(-1, -1, 1), (-1, -1, -1), (-1, 1, 1), (-1, 1, -1)]),
    ((1, 0, 0), [(1, -1, -1), (1, -1, 1), (1, 1, -1), (1, 1, 1)]),
    ((0, -1, 0), [(-1, -1, 1), (1, -1, 1), (-1, -1, -1), (1, -1, -1)]),
    ((0, 1, 0), [(-1, 1, -1), (1, 1, -1), (-1, 1, 1), (1, 1, 1)]),
]

def cube(scale, dx):
    v, n, polys = [], [], []
    for f, (nrm, cs) in enumerate(faces):
        base = len(v)
        for c in cs:
            v.append((c[0] * scale, c[1] * scale, c[2] * scale))
            n.append(tuple(k * 4096 for k in nrm))
        polys.append((base, base + 1, base + 2, base + 3, f))
    return v, n, polys

v0, n0, p0 = cube(S, 0)           # bone 0: big cube
v1, n1, p1 = cube(S // 3, 0)      # bone 1: small cube on top
verts = v0 + v1
norms = n0 + n1
polys = p0 + [(a + 24, b + 24, c + 24, d + 24, f) for a, b, c, d, f in p1]

uv = [(0, 0), (63, 0), (0, 63), (63, 63)]
mbac = bytearray(b'MB\x04\x00')
mbac += bytes([1, 1, 1, 1])                       # vFmt nFmt pFmt bFmt
mbac += struct.pack('<4H', len(verts), 0, len(polys), 2)  # verts, tri, quad, bones
for x, y, z in verts:
    mbac += struct.pack('<3h', x, y, z)
for x, y, z in norms:
    mbac += struct.pack('<3h', x, y, z)
for a, b, c, d, f in polys:
    # material: double sided (16); the top face of the big cube color-keyed (1)
    mat = 16 | (1 if f == 5 else 0)
    mbac += struct.pack('<5H', mat, a, b, c, d)
    for u, w in uv:
        mbac += bytes([u, w])
# bones: big cube at the origin, small cube 600 units above it (y down)
ident = [4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0]
mbac += struct.pack('<Hh12h', 24, -1, *ident)
child = list(ident)
child[7] = -600
mbac += struct.pack('<Hh12h', 24, 0, *child)
open(out + '/cube.mbac', 'wb').write(mbac)

# --------------------------------------------------------------- action
mtra = bytearray(b'MT\x04\x00')
mtra += struct.pack('<2H', 1, 2)                  # actions, bones
mtra += struct.pack('<8H', *([0] * 8)) + struct.pack('<i', 0)
mtra += struct.pack('<H', 60)                     # max frame
mtra += bytes([1])                                # bone 0: identity
mtra += bytes([2])                                # bone 1: keyed
mtra += struct.pack('<H', 1) + struct.pack('<H3h', 0, 0, 0, 0)            # translation
mtra += struct.pack('<H', 1) + struct.pack('<H3h', 0, 4096, 4096, 4096)   # scale
mtra += struct.pack('<H', 3) + struct.pack('<H3h', 0, 0, 0, 4096) \
    + struct.pack('<H3h', 30, 4096, 0, 0) + struct.pack('<H3h', 60, 0, 0, 4096)  # direction
mtra += struct.pack('<H', 2) + struct.pack('<Hh', 0, 0) + struct.pack('<Hh', 60, 2048)  # roll
open(out + '/spin.mtra', 'wb').write(mtra)

# -------------------------------------------------------------- texture
def bmp8(w, h, pal, px):
    row = (w + 3) & ~3
    data = bytearray()
    for y in range(h - 1, -1, -1):
        r = bytes(px[y * w:(y + 1) * w])
        data += r + b'\0' * (row - w)
    hdr = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 8, 0, len(data), 2835, 2835, len(pal), len(pal))
    palb = b''.join(struct.pack('<BBBB', c & 255, (c >> 8) & 255, (c >> 16) & 255, 0) for c in pal)
    off = 14 + len(hdr) + len(palb)
    return b'BM' + struct.pack('<IHHI', off + len(data), 0, 0, off) + hdr + palb + data

pal = [0xff00ff, 0xe0c040, 0x804020, 0x2060c0, 0xffffff]
px = []
for y in range(64):
    for x in range(64):
        if (x - 32) ** 2 + (y - 32) ** 2 < 100:
            px.append(0)                          # hole: color key
        elif x < 3 or y < 3 or x > 60 or y > 60:
            px.append(4)                          # white border
        else:
            px.append(1 if ((x >> 3) ^ (y >> 3)) & 1 else 2)
open(out + '/tex.bmp', 'wb').write(bmp8(64, 64, pal, px))

# a round sprite: white disc on the key color
spx = []
for y in range(32):
    for x in range(32):
        spx.append(4 if (x - 16) ** 2 + (y - 16) ** 2 < 196 else 0)
open(out + '/spark.bmp', 'wb').write(bmp8(32, 32, pal, spx))
print('ok', len(mbac), len(mtra))
