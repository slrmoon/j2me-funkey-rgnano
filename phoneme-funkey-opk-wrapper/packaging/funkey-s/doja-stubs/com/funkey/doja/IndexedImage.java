package com.funkey.doja;

import java.io.IOException;
import java.util.Hashtable;

/**
 * An image as palette indices: decoded from GIF or 8-bit BMP as they are,
 * other formats (PNG, JPEG) reduced to at most 256 colors.
 */
public final class IndexedImage {
    public int width;
    public int height;
    public byte[] pixels;
    /** ARGB entries */
    public int[] palette;
    /** -1 = none */
    public int transparentIndex = -1;

    public static IndexedImage decode(byte[] data) throws IOException {
        if (data == null) {
            throw new NullPointerException("data");
        }
        IndexedImage r = new IndexedImage();
        if (data.length > 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F') {
            GifDecoder.DecodedImage d = GifDecoder.decode(data);
            r.width = d.width;
            r.height = d.height;
            r.pixels = d.indices;
            r.palette = d.palette;
            r.transparentIndex = d.transparentIndex;
            return r;
        }
        if (data.length > 54 && data[0] == 'B' && data[1] == 'M' && bmp8(data, r)) {
            return r;
        }
        javax.microedition.lcdui.Image img;
        try {
            img = javax.microedition.lcdui.Image.createImage(data, 0, data.length);
        } catch (IllegalArgumentException e) {
            throw new IOException("unsupported image");
        }
        int w = img.getWidth();
        int h = img.getHeight();
        int[] argb = new int[w * h];
        img.getRGB(argb, 0, w, 0, 0, w, h);
        quantize(r, argb, w, h);
        return r;
    }

    /** exact colors while they fit in 256 entries, the rest map to entry 0 */
    public static void quantize(IndexedImage r, int[] argb, int w, int h) {
        Hashtable map = new Hashtable();
        int[] pal = new int[256];
        int n = 0;
        byte[] px = new byte[w * h];
        int transparent = -1;
        for (int i = 0; i < argb.length; i++) {
            int c = argb[i];
            if ((c >>> 24) < 128) {
                c = 0;
            } else {
                c |= 0xff000000;
            }
            Integer key = new Integer(c);
            Integer idx = (Integer) map.get(key);
            if (idx == null) {
                if (n >= 256) {
                    idx = new Integer(0);
                } else {
                    idx = new Integer(n);
                    map.put(key, idx);
                    pal[n] = c;
                    if (c == 0) {
                        transparent = n;
                    }
                    n++;
                }
            }
            px[i] = (byte) idx.intValue();
        }
        int[] p2 = new int[n > 0 ? n : 1];
        System.arraycopy(pal, 0, p2, 0, n);
        r.width = w;
        r.height = h;
        r.pixels = px;
        r.palette = p2;
        r.transparentIndex = transparent;
    }

    private static int le32(byte[] d, int o) {
        return (d[o] & 255) | ((d[o + 1] & 255) << 8) | ((d[o + 2] & 255) << 16) | ((d[o + 3] & 255) << 24);
    }

    private static int le16(byte[] d, int o) {
        return (d[o] & 255) | ((d[o + 1] & 255) << 8);
    }

    /** uncompressed 1/4/8-bit BMP */
    private static boolean bmp8(byte[] d, IndexedImage r) {
        int off = le32(d, 10);
        int hdr = le32(d, 14);
        int w = le32(d, 18);
        int h = le32(d, 22);
        int bpp = le16(d, 28);
        int comp = le32(d, 30);
        if (comp != 0 || (bpp != 1 && bpp != 4 && bpp != 8) || w <= 0 || h == 0) {
            return false;
        }
        int colors = le32(d, 46);
        if (colors == 0) {
            colors = 1 << bpp;
        }
        int[] pal = new int[colors];
        int po = 14 + hdr;
        for (int i = 0; i < colors && po + 4 * i + 2 < d.length; i++) {
            int b = d[po + 4 * i] & 255;
            int g = d[po + 4 * i + 1] & 255;
            int rr = d[po + 4 * i + 2] & 255;
            pal[i] = 0xff000000 | (rr << 16) | (g << 8) | b;
        }
        boolean bottomUp = h > 0;
        if (h < 0) {
            h = -h;
        }
        int stride = ((w * bpp + 31) / 32) * 4;
        byte[] px = new byte[w * h];
        for (int y = 0; y < h; y++) {
            int row = off + (bottomUp ? h - 1 - y : y) * stride;
            for (int x = 0; x < w; x++) {
                int v;
                if (bpp == 8) {
                    v = row + x < d.length ? d[row + x] & 255 : 0;
                } else if (bpp == 4) {
                    int b = row + x / 2 < d.length ? d[row + x / 2] & 255 : 0;
                    v = (x & 1) == 0 ? b >> 4 : b & 15;
                } else {
                    int b = row + x / 8 < d.length ? d[row + x / 8] & 255 : 0;
                    v = (b >> (7 - (x & 7))) & 1;
                }
                px[y * w + x] = (byte) v;
            }
        }
        r.width = w;
        r.height = h;
        r.pixels = px;
        r.palette = pal;
        return true;
    }

    /** ARGB pixels for a palette (RGB565 entries as the handset stores them, or ARGB) */
    public static int[] render(byte[] px, int[] argbPalette, int transparent) {
        int[] out = new int[px.length];
        int n = argbPalette.length;
        for (int i = 0; i < px.length; i++) {
            int idx = px[i] & 255;
            if (idx == transparent) {
                out[i] = 0;
            } else {
                out[i] = idx < n ? argbPalette[idx] : 0xff000000;
            }
        }
        return out;
    }
}
