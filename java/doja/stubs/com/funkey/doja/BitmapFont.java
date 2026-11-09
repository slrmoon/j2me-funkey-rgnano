package com.funkey.doja;

import java.io.DataInputStream;
import java.io.InputStream;

import javax.microedition.lcdui.Graphics;

/**
 * Japanese bitmap fonts (Shinonome, public domain) built by
 * tools/doja/mkfont.py. Text is rendered into an ARGB buffer and drawn with
 * one drawRGB call.
 */
public final class BitmapFont {
    private static final BitmapFont[] BASE = new BitmapFont[2];
    private static final int[] BASE_SIZES = { 12, 16 };
    private static int[] buffer = new int[2048];
    private static final Object LOCK = new Object();

    private final int height;
    private final int ascent;
    private final char[] codes;
    private final byte[] widths;
    private final int[] offsets;
    private final byte[] bits;

    private final BitmapFont base;
    private final int scale;

    private BitmapFont(int height, int ascent, char[] codes, byte[] widths,
            int[] offsets, byte[] bits) {
        this.height = height;
        this.ascent = ascent;
        this.codes = codes;
        this.widths = widths;
        this.offsets = offsets;
        this.bits = bits;
        this.base = this;
        this.scale = 1;
    }

    private BitmapFont(BitmapFont base, int scale) {
        this.base = base;
        this.scale = scale;
        this.height = base.height * scale;
        this.ascent = base.ascent * scale;
        this.codes = null;
        this.widths = null;
        this.offsets = null;
        this.bits = null;
    }

    /** Returns a font close to the requested pixel height. */
    public static BitmapFont forPixelSize(int px) {
        if (px <= 13) {
            return base(0);
        }
        if (px <= 19) {
            return base(1);
        }
        if (px <= 27) {
            return scaled(0, 2);
        }
        return scaled(1, 2);
    }

    private static final BitmapFont[] SCALED = new BitmapFont[2];

    private static synchronized BitmapFont scaled(int index, int scale) {
        if (SCALED[index] == null) {
            BitmapFont b = base(index);
            SCALED[index] = b == null ? null : new BitmapFont(b, scale);
        }
        return SCALED[index] != null ? SCALED[index] : base(0);
    }

    private static synchronized BitmapFont base(int index) {
        if (BASE[index] == null) {
            BASE[index] = load("/com/funkey/doja/f" + BASE_SIZES[index] + ".bin");
            if (BASE[index] == null && index != 0) {
                return base(0);
            }
        }
        return BASE[index];
    }

    private static BitmapFont load(String path) {
        try {
            InputStream raw = new ResourceLoader().open(path);
            DataInputStream in = new DataInputStream(raw);
            if (in.readInt() != 0x444a4631) { /* DJF1 */
                return null;
            }
            int h = in.readUnsignedByte();
            int a = in.readUnsignedByte();
            int count = in.readUnsignedShort();
            char[] codes = new char[count];
            for (int i = 0; i < count; i++) {
                codes[i] = in.readChar();
            }
            byte[] widths = new byte[count];
            in.readFully(widths);
            int[] offsets = new int[count + 1];
            int total = 0;
            for (int i = 0; i < count; i++) {
                offsets[i] = total;
                total += h * (((widths[i] & 255) + 7) >> 3);
            }
            offsets[count] = total;
            byte[] bits = new byte[total];
            in.readFully(bits);
            in.close();
            return new BitmapFont(h, a, codes, widths, offsets, bits);
        } catch (Throwable t) {
            System.out.println("DoJa: font load failed " + path + ": " + t);
            return null;
        }
    }

    public int getHeight() {
        return height;
    }

    public int getAscent() {
        return ascent;
    }

    public int getDescent() {
        return height - ascent;
    }

    private int find(char c) {
        char[] cs = base.codes;
        int lo = 0;
        int hi = cs.length - 1;
        while (lo <= hi) {
            int mid = (lo + hi) >>> 1;
            char m = cs[mid];
            if (m < c) {
                lo = mid + 1;
            } else if (m > c) {
                hi = mid - 1;
            } else {
                return mid;
            }
        }
        return -1;
    }

    private int baseWidth(char c) {
        int i = find(c);
        if (i >= 0) {
            return base.widths[i] & 255;
        }
        if (c < 0x20) {
            return 0;
        }
        if (c < 0x100 || (c >= 0xff61 && c <= 0xff9f)) {
            return base.height / 2;
        }
        return base.height;
    }

    public int charWidth(char c) {
        return baseWidth(c) * scale;
    }

    public int stringWidth(String s) {
        return s == null ? 0 : stringWidth(s, 0, s.length());
    }

    public int stringWidth(String s, int off, int len) {
        int w = 0;
        for (int i = 0; i < len; i++) {
            w += baseWidth(s.charAt(off + i));
        }
        return w * scale;
    }

    public int charsWidth(char[] s, int off, int len) {
        int w = 0;
        for (int i = 0; i < len; i++) {
            w += baseWidth(s[off + i]);
        }
        return w * scale;
    }

    /**
     * Draws text with its top-left corner at (x, top).
     */
    public void draw(Graphics g, String s, int off, int len, int x, int top,
            int rgb, boolean bold) {
        if (s == null || len <= 0) {
            return;
        }
        char[] cs = new char[len];
        s.getChars(off, off + len, cs, 0);
        draw(g, cs, 0, len, x, top, rgb, bold);
    }

    public void draw(Graphics g, char[] s, int off, int len, int x, int top,
            int rgb, boolean bold) {
        if (s == null || len <= 0) {
            return;
        }
        int bw = 0;
        for (int i = 0; i < len; i++) {
            bw += baseWidth(s[off + i]);
        }
        if (bw <= 0) {
            return;
        }
        if (bold) {
            bw++;
        }
        /* skip text that is entirely outside the clip */
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cw = g.getClipWidth();
        int ch = g.getClipHeight();
        int pw = bw * scale;
        int ph = height;
        if (x >= cx + cw || top >= cy + ch || x + pw <= cx || top + ph <= cy) {
            return;
        }
        /* draw only the visible horizontal range to keep the buffer small */
        int startX = x < cx ? cx - x : 0;
        int endX = x + pw > cx + cw ? cx + cw - x : pw;
        int visW = endX - startX;
        if (visW <= 0) {
            return;
        }
        int argb = 0xff000000 | rgb;
        synchronized (LOCK) {
            int need = visW * ph;
            if (buffer.length < need) {
                buffer = new int[need];
            }
            int[] buf = buffer;
            for (int i = 0; i < need; i++) {
                buf[i] = 0;
            }
            int bh = base.height;
            int pen = 0;
            for (int i = 0; i < len; i++) {
                char c = s[off + i];
                int gi = find(c);
                int gw;
                if (gi >= 0) {
                    gw = base.widths[gi] & 255;
                    int rb = (gw + 7) >> 3;
                    int o = base.offsets[gi];
                    for (int row = 0; row < bh; row++) {
                        int rowOff = o + row * rb;
                        for (int col = 0; col < gw; col++) {
                            if ((base.bits[rowOff + (col >> 3)] & (0x80 >> (col & 7))) != 0) {
                                plot(buf, visW, startX, (pen + col) * scale, row * scale, argb, bold);
                            }
                        }
                    }
                } else {
                    gw = baseWidth(c);
                    if (gw > 2 && c >= 0x80) {
                        /* missing glyph: hollow box */
                        for (int col = 1; col < gw - 1; col++) {
                            plot(buf, visW, startX, (pen + col) * scale, 1 * scale, argb, false);
                            plot(buf, visW, startX, (pen + col) * scale, (bh - 2) * scale, argb, false);
                        }
                        for (int row = 1; row < bh - 1; row++) {
                            plot(buf, visW, startX, (pen + 1) * scale, row * scale, argb, false);
                            plot(buf, visW, startX, (pen + gw - 2) * scale, row * scale, argb, false);
                        }
                    }
                }
                pen += gw;
            }
            g.drawRGB(buf, 0, visW, x + startX, top, visW, ph, true);
        }
    }

    private void plot(int[] buf, int stride, int startX, int px, int py, int argb, boolean bold) {
        int s = scale;
        int cols = bold ? s + 1 : s;
        for (int dy = 0; dy < s; dy++) {
            int y = py + dy;
            if (y >= height) {
                break;
            }
            int rowBase = y * stride;
            for (int dx = 0; dx < cols; dx++) {
                int xx = px + dx - startX;
                if (xx >= 0 && xx < stride) {
                    buf[rowBase + xx] = argb;
                }
            }
        }
    }

    /** True when text can be drawn by this font (has at least the ASCII set). */
    public static boolean available() {
        return base(0) != null;
    }
}
