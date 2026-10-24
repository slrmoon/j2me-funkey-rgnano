package com.nttdocomo.util;

import java.io.ByteArrayOutputStream;

/** MD5 and SHA-1 (CLDC has no java.security). */
public class MessageDigest {
    private String algorithm;
    private ByteArrayOutputStream buf = new ByteArrayOutputStream();

    protected MessageDigest() {
    }

    public static MessageDigest getInstance(String algorithm) {
        if (algorithm == null) {
            throw new NullPointerException("algorithm");
        }
        String a = algorithm.toUpperCase();
        if (a.equals("SHA1")) {
            a = "SHA-1";
        }
        if (!a.equals("MD5") && !a.equals("SHA-1")) {
            throw new IllegalArgumentException(algorithm);
        }
        MessageDigest m = new MessageDigest();
        m.algorithm = a;
        return m;
    }

    public void update(byte input) {
        buf.write(input);
    }

    public void update(byte[] input, int offset, int length) {
        buf.write(input, offset, length);
    }

    public void update(byte[] input) {
        buf.write(input, 0, input.length);
    }

    public byte[] digest() {
        byte[] data = buf.toByteArray();
        reset();
        return algorithm.equals("MD5") ? md5(data) : sha1(data);
    }

    public int digest(byte[] out, int offset, int length) {
        byte[] d = digest();
        if (length < d.length) {
            throw new IllegalArgumentException("length");
        }
        System.arraycopy(d, 0, out, offset, d.length);
        return d.length;
    }

    public byte[] digest(byte[] input) {
        update(input);
        return digest();
    }

    public void reset() {
        buf = new ByteArrayOutputStream();
    }

    public String toString() {
        return algorithm + " Message Digest";
    }

    public final int getDigestLength() {
        return algorithm.equals("MD5") ? 16 : 20;
    }

    public final String getAlgorithm() {
        return algorithm;
    }

    public static boolean isEqual(byte[] a, byte[] b) {
        if (a == null || b == null || a.length != b.length) {
            return a == b;
        }
        for (int i = 0; i < a.length; i++) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }

    /* ------------------------------------------------------------------ */

    private static byte[] pad(byte[] data, boolean bigEndian) {
        int n = data.length;
        int total = ((n + 8) / 64 + 1) * 64;
        byte[] m = new byte[total];
        System.arraycopy(data, 0, m, 0, n);
        m[n] = (byte) 0x80;
        long bits = (long) n * 8;
        for (int i = 0; i < 8; i++) {
            int shift = bigEndian ? (7 - i) * 8 : i * 8;
            m[total - 8 + i] = (byte) (bits >>> shift);
        }
        return m;
    }

    private static final int[] MD5_S = {
        7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21
    };
    private static final int[] md5K = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };

    private static byte[] md5(byte[] data) {
        byte[] m = pad(data, false);
        int a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;
        int[] w = new int[16];
        for (int off = 0; off < m.length; off += 64) {
            for (int i = 0; i < 16; i++) {
                int p = off + i * 4;
                w[i] = (m[p] & 255) | ((m[p + 1] & 255) << 8) | ((m[p + 2] & 255) << 16) | ((m[p + 3] & 255) << 24);
            }
            int a = a0, b = b0, c = c0, d = d0;
            for (int i = 0; i < 64; i++) {
                int f, g;
                if (i < 16) {
                    f = (b & c) | (~b & d);
                    g = i;
                } else if (i < 32) {
                    f = (d & b) | (~d & c);
                    g = (5 * i + 1) & 15;
                } else if (i < 48) {
                    f = b ^ c ^ d;
                    g = (3 * i + 5) & 15;
                } else {
                    f = c ^ (b | ~d);
                    g = (7 * i) & 15;
                }
                int t = d;
                d = c;
                c = b;
                int x = a + f + md5K[i] + w[g];
                int s = MD5_S[(i >> 4) * 4 + (i & 3)];
                b = b + ((x << s) | (x >>> (32 - s)));
                a = t;
            }
            a0 += a;
            b0 += b;
            c0 += c;
            d0 += d;
        }
        byte[] out = new byte[16];
        int[] h = { a0, b0, c0, d0 };
        for (int i = 0; i < 16; i++) {
            out[i] = (byte) (h[i >> 2] >>> ((i & 3) * 8));
        }
        return out;
    }

    private static byte[] sha1(byte[] data) {
        byte[] m = pad(data, true);
        int h0 = 0x67452301, h1 = 0xefcdab89, h2 = 0x98badcfe, h3 = 0x10325476, h4 = 0xc3d2e1f0;
        int[] w = new int[80];
        for (int off = 0; off < m.length; off += 64) {
            for (int i = 0; i < 16; i++) {
                int p = off + i * 4;
                w[i] = ((m[p] & 255) << 24) | ((m[p + 1] & 255) << 16) | ((m[p + 2] & 255) << 8) | (m[p + 3] & 255);
            }
            for (int i = 16; i < 80; i++) {
                int x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
                w[i] = (x << 1) | (x >>> 31);
            }
            int a = h0, b = h1, c = h2, d = h3, e = h4;
            for (int i = 0; i < 80; i++) {
                int f, k;
                if (i < 20) {
                    f = (b & c) | (~b & d);
                    k = 0x5a827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ed9eba1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8f1bbcdc;
                } else {
                    f = b ^ c ^ d;
                    k = 0xca62c1d6;
                }
                int t = ((a << 5) | (a >>> 27)) + f + e + k + w[i];
                e = d;
                d = c;
                c = (b << 30) | (b >>> 2);
                b = a;
                a = t;
            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }
        int[] h = { h0, h1, h2, h3, h4 };
        byte[] out = new byte[20];
        for (int i = 0; i < 20; i++) {
            out[i] = (byte) (h[i >> 2] >>> ((3 - (i & 3)) * 8));
        }
        return out;
    }
}
