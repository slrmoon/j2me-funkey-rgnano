#!/usr/bin/env python3
"""Build the DoJa runtime bitmap fonts from Shinonome BDF files (public domain,
Debian package xfonts-shinonome; convert the .pcf.gz files with pcf2bdf).

usage: mkfont.py out.bin halfwidth.bdf fullwidth.bdf

Output format (big endian):
  'DJF1' u8 height u8 ascent u16 count
  u16 code[count]   (Unicode, sorted)
  u8  width[count]  (advance = bitmap width)
  bitmaps: per glyph height rows of ceil(width/8) bytes, MSB first
"""
import sys

def parse_bdf(path):
    glyphs = {}
    ascent = descent = None
    with open(path, encoding='latin-1') as f:
        lines = f.read().split('\n')
    i = 0
    while i < len(lines):
        l = lines[i]
        if l.startswith('FONT_ASCENT'):
            ascent = int(l.split()[1])
        elif l.startswith('FONT_DESCENT'):
            descent = int(l.split()[1])
        elif l.startswith('STARTCHAR'):
            enc = None; bbx = None; dw = None; rows = []
            i += 1
            while not lines[i].startswith('ENDCHAR'):
                t = lines[i]
                if t.startswith('ENCODING'):
                    enc = int(t.split()[1])
                elif t.startswith('DWIDTH'):
                    dw = int(t.split()[1])
                elif t.startswith('BBX'):
                    bbx = [int(x) for x in t.split()[1:]]
                elif t == 'BITMAP':
                    i += 1
                    while not lines[i].startswith('ENDCHAR'):
                        rows.append(lines[i].strip()); i += 1
                    break
                i += 1
            glyphs[enc] = (dw, bbx, rows)
        i += 1
    return glyphs, ascent, descent

def render(glyph, height, ascent):
    dw, (bw, bh, bx, by), rows = glyph
    w = dw
    grid = [[0] * w for _ in range(height)]
    top = ascent - (by + bh)
    for r, hexrow in enumerate(rows):
        bits = int(hexrow, 16) if hexrow else 0
        nbits = len(hexrow) * 4
        for c in range(bw):
            if bits >> (nbits - 1 - c) & 1:
                y = top + r; x = bx + c
                if 0 <= y < height and 0 <= x < w:
                    grid[y][x] = 1
    return w, grid

def jis_to_unicode(code):
    hi, lo = code >> 8, code & 0xff
    # JIS -> Shift-JIS
    s1 = ((hi + 1) >> 1) + (0x70 if hi <= 0x5e else 0xb0)
    s2 = lo + (0x1f if hi & 1 else 0x7e)
    if hi & 1 and lo >= 0x60:
        s2 += 1
    try:
        return bytes([s1, s2]).decode('cp932')
    except UnicodeDecodeError:
        return None

ALIASES = {  # MS932 code point -> other Unicode spellings of the same JIS char
    '～': '〜', '∥': '‖', '－': '−', '￠': '¢',
    '￡': '£', '￢': '¬', '―': '—', '￥': '¥',
}

def main():
    out, half, full = sys.argv[1:4]
    hg, ha, hd = parse_bdf(half)
    fg, fa, fd = parse_bdf(full)
    height = fa + fd
    ascent = fa
    table = {}
    for enc, g in hg.items():
        if 0x20 <= enc <= 0x7e:
            u = chr(enc)
        elif 0xa1 <= enc <= 0xdf:
            u = chr(0xff61 + enc - 0xa1)
        else:
            continue
        table[ord(u)] = render(g, height, ascent)
    for enc, g in fg.items():
        u = jis_to_unicode(enc)
        if u is None or len(u) != 1:
            continue
        r = render(g, height, ascent)
        table.setdefault(ord(u), r)
        if u in ALIASES:
            table.setdefault(ord(ALIASES[u]), r)
    # JIS X 0201 roman puts the yen sign at 0x5c; SJIS text decodes it as backslash
    # but i-mode handsets drew a yen sign there.
    if 0x5c in table:
        table[0xa5] = table[0x5c]
    codes = sorted(c for c in table if c <= 0xffff)
    data = bytearray(b'DJF1')
    data += bytes([height, ascent]) + len(codes).to_bytes(2, 'big')
    for c in codes:
        data += c.to_bytes(2, 'big')
    for c in codes:
        data.append(table[c][0])
    for c in codes:
        w, grid = table[c]
        rb = (w + 7) // 8
        for row in grid:
            v = 0
            for x in range(rb * 8):
                v = (v << 1) | (row[x] if x < w else 0)
            data += v.to_bytes(rb, 'big')
    open(out, 'wb').write(data)
    print(out, 'glyphs', len(codes), 'bytes', len(data), 'height', height, 'ascent', ascent)

main()
