package com.funkey.mc3;

import java.io.IOException;
import java.util.Hashtable;

/**
 * MascotCapsule texture: palette indices and an RGB palette, from an
 * uncompressed 1/4/8-bit BMP (the handset format). Other images the
 * platform can decode (PNG...) are reduced to 256 colors. Palette index
 * 0 is the color key.
 */
public final class TexData {
    public int width;
    public int height;
    public byte[] pixels;
    /** ARGB entries */
    public int[] palette;
    /** false: environment (sphere map) texture */
    public boolean forModel;
    /** per-texture toon shading (DoJa) */
    public boolean toon;
    public int toonHighlight = 128;
    public int toonMid = 255;
    public int toonShadow = 96;
    /** renderer cache (M3G textures and appearances), made on first use */
    public Object m3g;

    public static TexData load(byte[] data, boolean forModel) throws IOException {
        if (data == null) {
            throw new NullPointerException("data");
        }
        TexData t = new TexData();
        t.forModel = forModel;
        if (data.length > 54 && data[0] == 'B' && data[1] == 'M' && t.bmp(data)) {
            return t;
        }
        javax.microedition.lcdui.Image img;
        try {
            img = javax.microedition.lcdui.Image.createImage(data, 0, data.length);
        } catch (IllegalArgumentException e) {
            throw new IOException("unsupported texture image");
        }
        int w = img.getWidth();
        int h = img.getHeight();
        int[] argb = new int[w * h];
        img.getRGB(argb, 0, w, 0, 0, w, h);
        t.quantize(argb, w, h);
        return t;
    }

    private static int le32(byte[] d, int o) {
        return (d[o] & 255) | ((d[o + 1] & 255) << 8) | ((d[o + 2] & 255) << 16) | ((d[o + 3] & 255) << 24);
    }

    private static int le16(byte[] d, int o) {
        return (d[o] & 255) | ((d[o + 1] & 255) << 8);
    }

    /** uncompressed 1/4/8-bit BMP; false for anything else */
    private boolean bmp(byte[] d) {
        int off = le32(d, 10);
        int hdr = le32(d, 14);
        int w = le32(d, 18);
        int h = le32(d, 22);
        int bpp = le16(d, 28);
        int comp = le32(d, 30);
        if (comp != 0 || (bpp != 1 && bpp != 4 && bpp != 8) || w <= 0 || h == 0
                || w > 4096 || h > 4096 || h < -4096 || hdr < 12 || hdr > d.length
                || off < 0 || off >= d.length) {
            return false;
        }
        int colors = le32(d, 46);
        if (colors <= 0 || colors > (1 << bpp)) {
            colors = 1 << bpp;
        }
        int[] pal = new int[colors];
        int po = 14 + hdr;
        for (int i = 0; i < colors && po + 4 * i + 2 < d.length; i++) {
            int b = d[po + 4 * i] & 255;
            int g = d[po + 4 * i + 1] & 255;
            int r = d[po + 4 * i + 2] & 255;
            pal[i] = 0xff000000 | (r << 16) | (g << 8) | b;
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
        width = w;
        height = h;
        pixels = px;
        palette = pal;
        return true;
    }

    /** exact colors while they fit in 256 entries; transparent pixels get index 0 */
    private void quantize(int[] argb, int w, int h) {
        Hashtable map = new Hashtable();
        int[] pal = new int[256];
        int n = 1;              /* index 0: the color key */
        byte[] px = new byte[w * h];
        for (int i = 0; i < argb.length; i++) {
            int c = argb[i];
            if ((c >>> 24) < 128) {
                px[i] = 0;
                continue;
            }
            c |= 0xff000000;
            Integer key = new Integer(c);
            Integer idx = (Integer) map.get(key);
            if (idx == null) {
                idx = new Integer(n < 256 ? n : 1);
                if (n < 256) {
                    map.put(key, idx);
                    pal[n++] = c;
                }
            }
            px[i] = (byte) idx.intValue();
        }
        int[] p2 = new int[n];
        System.arraycopy(pal, 0, p2, 0, n);
        width = w;
        height = h;
        pixels = px;
        palette = p2;
    }
}
