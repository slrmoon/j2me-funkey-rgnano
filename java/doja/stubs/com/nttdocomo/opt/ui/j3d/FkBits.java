package com.nttdocomo.opt.ui.j3d;

import java.io.IOException;

/** Little-endian byte reader with an LSB-first bit reader (MascotCapsule files). */
final class FkBits {
    private final byte[] b;
    int pos;
    private int cache;
    private int bits;

    FkBits(byte[] data) {
        b = data;
    }

    int u8() throws IOException {
        if (pos >= b.length) {
            throw new IOException("unexpected end of data");
        }
        return b[pos++] & 255;
    }

    int s8() throws IOException {
        return (byte) u8();
    }

    int u16() throws IOException {
        int lo = u8();
        return lo | (u8() << 8);
    }

    int s16() throws IOException {
        return (short) u16();
    }

    int s32() throws IOException {
        int a = u16();
        return a | (u16() << 16);
    }

    int ubits(int n) throws IOException {
        if (n == 0) {
            return 0;
        }
        while (bits < n) {
            cache |= u8() << bits;
            bits += 8;
        }
        int v = n >= 32 ? cache : cache & ((1 << n) - 1);
        cache = n >= 32 ? 0 : cache >>> n;
        bits -= n;
        return v;
    }

    int sbits(int n) throws IOException {
        int v = ubits(n);
        int sh = 32 - n;
        return (v << sh) >> sh;
    }

    void align() {
        cache = 0;
        bits = 0;
    }
}
