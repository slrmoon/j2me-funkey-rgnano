package com.nttdocomo.ui;

import com.nttdocomo.lang.XString;

/**
 * DoJa 2D graphics context on top of a MIDP Graphics.
 *
 * Coordinates: setOrigin() moves the origin (absolute, not cumulative);
 * clip rectangles are given relative to the origin. drawString() takes the
 * baseline as y. Colors are 0xRRGGBB values (alpha in the top byte when
 * created with the four argument getColorOfRGB()).
 */
public class Graphics implements com.nttdocomo.opt.ui.j3d.Graphics3D,
        com.nttdocomo.ui.graphics3d.Graphics3D {
    public static final int BLACK = 0;
    public static final int BLUE = 1;
    public static final int LIME = 2;
    public static final int AQUA = 3;
    public static final int RED = 4;
    public static final int FUCHSIA = 5;
    public static final int YELLOW = 6;
    public static final int WHITE = 7;
    public static final int GRAY = 8;
    public static final int NAVY = 9;
    public static final int GREEN = 10;
    public static final int TEAL = 11;
    public static final int MAROON = 12;
    public static final int PURPLE = 13;
    public static final int OLIVE = 14;
    public static final int SILVER = 15;

    public static final int FLIP_NONE = 0;
    public static final int FLIP_HORIZONTAL = 1;
    public static final int FLIP_VERTICAL = 2;
    public static final int FLIP_ROTATE = 3;
    public static final int FLIP_ROTATE_LEFT = 4;
    public static final int FLIP_ROTATE_RIGHT = 5;
    public static final int FLIP_ROTATE_RIGHT_HORIZONTAL = 6;
    public static final int FLIP_ROTATE_RIGHT_VERTICAL = 7;

    private static final int[] NAMED = {
        0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff, 0xffff00, 0xffffff,
        0x808080, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xc0c0c0
    };
    /* DoJa flip mode -> MIDP Sprite transform */
    private static final int[] FLIP_TO_TRANS = { 0, 2, 1, 3, 6, 5, 7, 4 };

    static final int TL = javax.microedition.lcdui.Graphics.TOP | javax.microedition.lcdui.Graphics.LEFT;

    javax.microedition.lcdui.Graphics g;
    /** the MIDP image this context draws into */
    javax.microedition.lcdui.Image target;
    /** set when drawing into a DoJa Image */
    Image image;
    /** set when drawing on a Canvas */
    Canvas canvas;
    int width;
    int height;
    int color;
    Font font;
    int originX;
    int originY;
    int flipMode;
    boolean disposed;
    /* opt render mode (Graphics2.setRenderMode) */
    int renderOp;
    /* opt.ui.Graphics2 render mode; ratios are 0..255 (255 = 100 %) */
    int renderSrc = 255;
    int renderDst = 255;
    /* MascotCapsule renderer state, created on first 3D call */
    Object r3d;

    protected Graphics() {
    }

    void init(javax.microedition.lcdui.Image t, Canvas c, Image img) {
        target = t;
        canvas = c;
        image = img;
        g = t.getGraphics();
        width = t.getWidth();
        height = t.getHeight();
        font = Font.getDefaultFont();
        color = 0;
        g.setColor(0);
        originX = 0;
        originY = 0;
        flipMode = 0;
    }

    /* ------------------------------------------------------------------ */
    /* colors                                                              */

    public static int getColorOfName(int name) {
        if (name < 0 || name >= NAMED.length) {
            throw new IllegalArgumentException("name");
        }
        return NAMED[name];
    }

    public static int getColorOfRGB(int r, int gr, int b) {
        if ((r | gr | b) < 0 || r > 255 || gr > 255 || b > 255) {
            throw new IllegalArgumentException("rgb");
        }
        return (r << 16) | (gr << 8) | b;
    }

    public static int getColorOfRGB(int r, int gr, int b, int a) {
        if ((r | gr | b | a) < 0 || r > 255 || gr > 255 || b > 255 || a > 255) {
            throw new IllegalArgumentException("rgba");
        }
        return (a << 24) | (r << 16) | (gr << 8) | b;
    }

    public void setColor(int c) {
        color = c;
        g.setColor(c & 0xffffff);
    }

    /* ------------------------------------------------------------------ */
    /* state                                                               */

    public void lock() {
        if (canvas != null) {
            canvas.lockCount++;
        }
    }

    public void unlock(boolean forced) {
        if (canvas == null) {
            return;
        }
        if (canvas.lockCount > 0) {
            canvas.lockCount--;
        }
        if (forced || canvas.lockCount == 0) {
            canvas.flushFromGraphics();
        }
    }

    public void setOrigin(int x, int y) {
        g.translate(x - g.getTranslateX(), y - g.getTranslateY());
        originX = x;
        originY = y;
    }

    protected final int getOriginX() {
        return originX;
    }

    protected final int getOriginY() {
        return originY;
    }

    public void setClip(int x, int y, int w, int h) {
        g.setClip(x, y, w, h);
    }

    public void clipRect(int x, int y, int w, int h) {
        g.clipRect(x, y, w, h);
    }

    public void clearClip() {
        g.setClip(-originX, -originY, width, height);
    }

    public void setFont(Font f) {
        font = f == null ? Font.getDefaultFont() : f;
    }

    public void setFlipMode(int mode) {
        if (mode < 0 || mode > 7) {
            throw new IllegalArgumentException("flipMode");
        }
        flipMode = mode;
    }

    public void setPictoColorEnabled(boolean enabled) {
    }

    public void dispose() {
        disposed = true;
    }

    public Graphics copy() {
        Graphics c = FkBridge.allocGraphics();
        c.target = target;
        c.canvas = canvas;
        c.image = image;
        c.g = target.getGraphics();
        c.width = width;
        c.height = height;
        c.font = font;
        c.setColor(color);
        c.setOrigin(originX, originY);
        c.g.setClip(g.getClipX(), g.getClipY(), g.getClipWidth(), g.getClipHeight());
        c.flipMode = flipMode;
        c.renderOp = renderOp;
        c.renderSrc = renderSrc;
        c.renderDst = renderDst;
        return c;
    }

    final void changed() {
        if (image != null) {
            image.version++;
        } else if (canvas != null) {
            canvas.drawn();
        }
    }

    /* ------------------------------------------------------------------ */
    /* primitives                                                          */

    public void drawLine(int x1, int y1, int x2, int y2) {
        g.drawLine(x1, y1, x2, y2);
        changed();
    }

    public void drawRect(int x, int y, int w, int h) {
        if (w < 0 || h < 0) {
            return;
        }
        g.drawRect(x, y, w, h);
        changed();
    }

    public void fillRect(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) {
            return;
        }
        int a = color >>> 24;
        if (a != 0 && a != 255) {
            blendRect(x, y, w, h, color);
        } else if (renderOp != 0 || renderSrc != 255) {
            blendRect(x, y, w, h, 0xff000000 | color);
        } else {
            g.fillRect(x, y, w, h);
        }
        changed();
    }

    public void clearRect(int x, int y, int w, int h) {
        int bg = canvas != null ? canvas.background : 0xffffff;
        g.setColor(bg & 0xffffff);
        g.fillRect(x, y, w, h);
        g.setColor(color & 0xffffff);
        changed();
    }

    public void drawArc(int x, int y, int w, int h, int start, int arc) {
        g.drawArc(x, y, w, h, start, arc);
        changed();
    }

    public void fillArc(int x, int y, int w, int h, int start, int arc) {
        g.fillArc(x, y, w, h, start, arc);
        changed();
    }

    public void drawPolyline(int[] xs, int[] ys, int n) {
        for (int i = 0; i + 1 < n; i++) {
            g.drawLine(xs[i], ys[i], xs[i + 1], ys[i + 1]);
        }
        changed();
    }

    public void drawPolyline(int[] xs, int[] ys, int offset, int count) {
        for (int i = 0; i + 1 < count; i++) {
            g.drawLine(xs[offset + i], ys[offset + i], xs[offset + i + 1], ys[offset + i + 1]);
        }
        changed();
    }

    public void fillPolygon(int[] xs, int[] ys, int n) {
        fillPolygon(xs, ys, 0, n);
    }

    public void fillPolygon(int[] xs, int[] ys, int offset, int count) {
        if (count < 3) {
            return;
        }
        if (count == 3) {
            g.fillTriangle(xs[offset], ys[offset], xs[offset + 1], ys[offset + 1],
                    xs[offset + 2], ys[offset + 2]);
            changed();
            return;
        }
        int minY = Integer.MAX_VALUE;
        int maxY = Integer.MIN_VALUE;
        for (int i = 0; i < count; i++) {
            int y = ys[offset + i];
            if (y < minY) {
                minY = y;
            }
            if (y > maxY) {
                maxY = y;
            }
        }
        int[] cross = new int[count];
        for (int y = minY; y <= maxY; y++) {
            int nc = 0;
            for (int i = 0; i < count; i++) {
                int j = (i + 1) % count;
                int y1 = ys[offset + i];
                int y2 = ys[offset + j];
                int x1 = xs[offset + i];
                int x2 = xs[offset + j];
                if (y1 == y2) {
                    continue;
                }
                if (y1 > y2) {
                    int t = y1; y1 = y2; y2 = t;
                    t = x1; x1 = x2; x2 = t;
                }
                if (y < y1 || y >= y2) {
                    continue;
                }
                cross[nc++] = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
            }
            for (int a = 1; a < nc; a++) {
                int v = cross[a];
                int b = a - 1;
                while (b >= 0 && cross[b] > v) {
                    cross[b + 1] = cross[b];
                    b--;
                }
                cross[b + 1] = v;
            }
            for (int k = 0; k + 1 < nc; k += 2) {
                g.fillRect(cross[k], y, cross[k + 1] - cross[k] + 1, 1);
            }
        }
        changed();
    }

    /* ------------------------------------------------------------------ */
    /* text                                                                */

    public void drawString(String str, int x, int y) {
        if (str == null) {
            throw new NullPointerException("str");
        }
        if (str.length() == 0) {
            return;
        }
        font.draw(g, str, x, y, color & 0xffffff);
        changed();
    }

    public void drawString(XString str, int x, int y) {
        drawString(str.toString(), x, y);
    }

    public void drawString(XString str, int x, int y, int off, int len) {
        drawString(str.toString().substring(off, off + len), x, y);
    }

    public void drawChars(char[] data, int x, int y, int off, int len) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        if (len <= 0) {
            return;
        }
        font.draw(g, data, off, len, x, y, color & 0xffffff);
        changed();
    }

    /** Draws a number right-aligned in a field of digit characters. */
    public void drawNumber(int x, int y, int value, int digit) {
        String s = String.valueOf(value);
        while (s.length() < digit) {
            s = " " + s;
        }
        drawString(s, x, y);
    }

    /* ------------------------------------------------------------------ */
    /* images                                                              */

    public void drawImage(Image img, int x, int y) {
        if (img == null) {
            throw new NullPointerException("image");
        }
        javax.microedition.lcdui.Image src = img.drawable();
        drawRegion(src, 0, 0, src.getWidth(), src.getHeight(), x, y);
    }

    public void drawImage(Image img, int x, int y, int sx, int sy, int w, int h) {
        if (img == null) {
            throw new NullPointerException("image");
        }
        javax.microedition.lcdui.Image src = img.drawable();
        drawRegion(src, sx, sy, w, h, x, y);
    }

    void drawRegion(javax.microedition.lcdui.Image src, int sx, int sy, int sw, int sh,
            int dx, int dy) {
        int iw = src.getWidth();
        int ih = src.getHeight();
        if (sw <= 0 || sh <= 0) {
            return;
        }
        /* clip the source rectangle to the image; keep the destination aligned */
        if (sx < 0) {
            sw += sx;
            if (flipMode == 0) {
                dx -= sx;
            }
            sx = 0;
        }
        if (sy < 0) {
            sh += sy;
            if (flipMode == 0) {
                dy -= sy;
            }
            sy = 0;
        }
        if (sx + sw > iw) {
            sw = iw - sx;
        }
        if (sy + sh > ih) {
            sh = ih - sy;
        }
        if (sw <= 0 || sh <= 0) {
            return;
        }
        if (renderOp != 0 || renderSrc != 255) {
            blendImage(src, sx, sy, sw, sh, dx, dy);
            changed();
            return;
        }
        if (flipMode == 0 && sx == 0 && sy == 0 && sw == iw && sh == ih) {
            g.drawImage(src, dx, dy, TL);
        } else {
            g.drawRegion(src, sx, sy, sw, sh, FLIP_TO_TRANS[flipMode], dx, dy, TL);
        }
        changed();
    }

    public void drawScaledImage(Image img, int dx, int dy, int dw, int dh,
            int sx, int sy, int sw, int sh) {
        if (img == null) {
            throw new NullPointerException("image");
        }
        if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) {
            return;
        }
        javax.microedition.lcdui.Image src = img.drawable();
        if (dw == sw && dh == sh) {
            drawRegion(src, sx, sy, sw, sh, dx, dy);
            return;
        }
        int iw = src.getWidth();
        int ih = src.getHeight();
        /* only scale the part that is visible */
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cr = cx + g.getClipWidth();
        int cb = cy + g.getClipHeight();
        int x0 = dx < cx ? cx : dx;
        int y0 = dy < cy ? cy : dy;
        int x1 = dx + dw > cr ? cr : dx + dw;
        int y1 = dy + dh > cb ? cb : dy + dh;
        if (x0 >= x1 || y0 >= y1) {
            return;
        }
        int ow = x1 - x0;
        int oh = y1 - y0;
        int[] row = new int[sw];
        int[] out = new int[ow * oh];
        int lastSy = -1;
        for (int y = 0; y < oh; y++) {
            int syy = sy + (y + y0 - dy) * sh / dh;
            if (syy < 0 || syy >= ih) {
                continue;
            }
            if (syy != lastSy) {
                int rx = sx < 0 ? 0 : sx;
                int rw = sx + sw > iw ? iw - rx : sx + sw - rx;
                if (rw <= 0) {
                    continue;
                }
                for (int i = 0; i < row.length; i++) {
                    row[i] = 0;
                }
                src.getRGB(row, rx - sx, sw, rx, syy, rw, 1);
                lastSy = syy;
            }
            int ob = y * ow;
            for (int x = 0; x < ow; x++) {
                int sxx = (x + x0 - dx) * sw / dw;
                if (flipMode == FLIP_HORIZONTAL) {
                    sxx = sw - 1 - sxx;
                }
                out[ob + x] = row[sxx];
            }
        }
        if (flipMode == FLIP_VERTICAL) {
            int[] tmp = new int[ow];
            for (int y = 0; y < oh / 2; y++) {
                System.arraycopy(out, y * ow, tmp, 0, ow);
                System.arraycopy(out, (oh - 1 - y) * ow, out, y * ow, ow);
                System.arraycopy(tmp, 0, out, (oh - 1 - y) * ow, ow);
            }
        }
        g.drawRGB(out, 0, ow, x0, y0, ow, oh, true);
        changed();
    }

    /** Affine image drawing; matrix is {m00, m01, m02, m10, m11, m12} in 1/4096 units. */
    public void drawImage(Image img, int[] matrix) {
        if (img == null || matrix == null) {
            throw new NullPointerException();
        }
        javax.microedition.lcdui.Image src = img.drawable();
        affine(src, matrix, 0, 0, src.getWidth(), src.getHeight());
    }

    public void drawImage(Image img, int[] matrix, int sx, int sy, int w, int h) {
        if (img == null || matrix == null) {
            throw new NullPointerException();
        }
        affine(img.drawable(), matrix, sx, sy, w, h);
    }

    void affine(javax.microedition.lcdui.Image src, int[] m, int sx, int sy, int sw, int sh) {
        int iw = src.getWidth();
        int ih = src.getHeight();
        if (sx < 0) {
            sw += sx;
            sx = 0;
        }
        if (sy < 0) {
            sh += sy;
            sy = 0;
        }
        if (sx + sw > iw) {
            sw = iw - sx;
        }
        if (sy + sh > ih) {
            sh = ih - sy;
        }
        if (sw <= 0 || sh <= 0) {
            return;
        }
        long a = m[0];
        long b = m[1];
        long c = m[2];
        long d = m[3];
        long e = m[4];
        long f = m[5];
        /* destination bounds of the transformed source rectangle */
        long[] xs = { c, a * sw + c, b * sh + c, a * sw + b * sh + c };
        long[] ys = { f, d * sw + f, e * sh + f, d * sw + e * sh + f };
        long minX = xs[0];
        long maxX = xs[0];
        long minY = ys[0];
        long maxY = ys[0];
        for (int i = 1; i < 4; i++) {
            if (xs[i] < minX) {
                minX = xs[i];
            }
            if (xs[i] > maxX) {
                maxX = xs[i];
            }
            if (ys[i] < minY) {
                minY = ys[i];
            }
            if (ys[i] > maxY) {
                maxY = ys[i];
            }
        }
        int bx0 = (int) (minX >> 12);
        int by0 = (int) (minY >> 12);
        int bx1 = (int) ((maxX + 4095) >> 12);
        int by1 = (int) ((maxY + 4095) >> 12);
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cr = cx + g.getClipWidth();
        int cb = cy + g.getClipHeight();
        if (bx0 < cx) {
            bx0 = cx;
        }
        if (by0 < cy) {
            by0 = cy;
        }
        if (bx1 > cr) {
            bx1 = cr;
        }
        if (by1 > cb) {
            by1 = cb;
        }
        int ow = bx1 - bx0;
        int oh = by1 - by0;
        if (ow <= 0 || oh <= 0) {
            return;
        }
        long det = a * e - b * d;
        if (det == 0) {
            return;
        }
        int[] srcPx = new int[sw * sh];
        src.getRGB(srcPx, 0, sw, sx, sy, sw, sh);
        int[] out = new int[ow * oh];
        /* inverse mapping, fixed point: src = inv(M) * (dst - t) */
        for (int y = 0; y < oh; y++) {
            long py = ((long) (y + by0) << 12) + 2048 - f;
            for (int x = 0; x < ow; x++) {
                long px = ((long) (x + bx0) << 12) + 2048 - c;
                long u = (e * px - b * py) / det;
                long v = (a * py - d * px) / det;
                if (u >= 0 && v >= 0 && u < sw && v < sh) {
                    out[y * ow + x] = srcPx[(int) v * sw + (int) u];
                }
            }
        }
        g.drawRGB(out, 0, ow, bx0, by0, ow, oh, true);
        changed();
    }

    public void drawSpriteSet(SpriteSet sprites) {
        drawSpriteSet(sprites, 0, sprites.getCount());
    }

    public void drawSpriteSet(SpriteSet sprites, int offset, int count) {
        int saved = flipMode;
        for (int i = 0; i < count; i++) {
            Sprite s = sprites.getSprite(offset + i);
            if (s == null || !s.visible || s.image == null) {
                continue;
            }
            flipMode = s.flipMode;
            drawImage(s.image, s.x, s.y, s.sx, s.sy, s.width, s.height);
        }
        flipMode = saved;
    }

    public void drawImageMap(ImageMap map, int x, int y) {
        if (map == null) {
            throw new NullPointerException("map");
        }
        map.draw(this, x, y);
    }

    /* ------------------------------------------------------------------ */
    /* pixels                                                              */

    public int getPixel(int x, int y) {
        return getRGBPixel(x, y);
    }

    public int getRGBPixel(int x, int y) {
        int ax = x + originX;
        int ay = y + originY;
        if (ax < 0 || ay < 0 || ax >= width || ay >= height) {
            throw new ArrayIndexOutOfBoundsException();
        }
        int[] px = new int[1];
        target.getRGB(px, 0, 1, ax, ay, 1, 1);
        return px[0] & 0xffffff;
    }

    public int[] getPixels(int x, int y, int w, int h, int[] pixels, int offset) {
        return getRGBPixels(x, y, w, h, pixels, offset);
    }

    public int[] getRGBPixels(int x, int y, int w, int h, int[] pixels, int offset) {
        if (pixels == null) {
            pixels = new int[w * h];
            offset = 0;
        }
        int ax = x + originX;
        int ay = y + originY;
        if (ax < 0 || ay < 0 || ax + w > width || ay + h > height) {
            throw new ArrayIndexOutOfBoundsException();
        }
        target.getRGB(pixels, offset, w, ax, ay, w, h);
        for (int i = 0; i < w * h; i++) {
            pixels[offset + i] &= 0xffffff;
        }
        return pixels;
    }

    public void setPixel(int x, int y) {
        g.fillRect(x, y, 1, 1);
        changed();
    }

    public void setPixel(int x, int y, int c) {
        setRGBPixel(x, y, c);
    }

    public void setRGBPixel(int x, int y, int c) {
        g.setColor(c & 0xffffff);
        g.fillRect(x, y, 1, 1);
        g.setColor(color & 0xffffff);
        changed();
    }

    public void setPixels(int x, int y, int w, int h, int[] pixels, int offset) {
        setRGBPixels(x, y, w, h, pixels, offset);
    }

    public void setRGBPixels(int x, int y, int w, int h, int[] pixels, int offset) {
        if (w <= 0 || h <= 0) {
            return;
        }
        int[] buf = new int[w * h];
        for (int i = 0; i < buf.length; i++) {
            buf[i] = 0xff000000 | pixels[offset + i];
        }
        g.drawRGB(buf, 0, w, x, y, w, h, false);
        changed();
    }

    public void copyArea(int x, int y, int w, int h, int dx, int dy) {
        if (w <= 0 || h <= 0) {
            return;
        }
        int ax = x + originX;
        int ay = y + originY;
        if (ax < 0) {
            w += ax;
            ax = 0;
        }
        if (ay < 0) {
            h += ay;
            ay = 0;
        }
        if (ax + w > width) {
            w = width - ax;
        }
        if (ay + h > height) {
            h = height - ay;
        }
        if (w <= 0 || h <= 0) {
            return;
        }
        int[] buf = new int[w * h];
        target.getRGB(buf, 0, w, ax, ay, w, h);
        g.drawRGB(buf, 0, w, ax - originX + dx, ay - originY + dy, w, h, false);
        changed();
    }

    protected final Image getImageRegion(int x, int y, int w, int h) {
        int ax = x + originX;
        int ay = y + originY;
        javax.microedition.lcdui.Image m = javax.microedition.lcdui.Image.createImage(w, h);
        m.getGraphics().drawImage(target, -ax, -ay, TL);
        return new FkImage(m, true);
    }

    /* ------------------------------------------------------------------ */
    /* blending helpers (opt render mode, translucent colors)             */

    private void blendRect(int x, int y, int w, int h, int argb) {
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cr = cx + g.getClipWidth();
        int cb = cy + g.getClipHeight();
        int x0 = x < cx ? cx : x;
        int y0 = y < cy ? cy : y;
        int x1 = x + w > cr ? cr : x + w;
        int y1 = y + h > cb ? cb : y + h;
        if (x0 >= x1 || y0 >= y1) {
            return;
        }
        int ow = x1 - x0;
        int oh = y1 - y0;
        int[] dst = scratch(ow * oh);
        target.getRGB(dst, 0, ow, x0 + originX, y0 + originY, ow, oh);
        int a = argb >>> 24;
        if (renderOp == 0 && renderSrc == 255) {
            /* plain alpha blend of one color: precomputed source terms */
            int na = 255 - a;
            int sr = ((argb >> 16) & 255) * a;
            int sg = ((argb >> 8) & 255) * a;
            int sb = (argb & 255) * a;
            for (int i = 0, cnt = ow * oh; i < cnt; i++) {
                int d = dst[i];
                int r = (sr + ((d >> 16) & 255) * na + 127) / 255;
                int gg = (sg + ((d >> 8) & 255) * na + 127) / 255;
                int b = (sb + (d & 255) * na + 127) / 255;
                dst[i] = 0xff000000 | (r << 16) | (gg << 8) | b;
            }
        } else {
            for (int i = 0, cnt = ow * oh; i < cnt; i++) {
                dst[i] = mix(argb, a, dst[i]);
            }
        }
        g.drawRGB(dst, 0, ow, x0, y0, ow, oh, false);
    }

    private int[] scratchBuf = new int[0];

    /** pixel buffer reused by this Graphics */
    private int[] scratch(int n) {
        if (scratchBuf.length < n) {
            scratchBuf = new int[n];
        }
        return scratchBuf;
    }

    private void blendImage(javax.microedition.lcdui.Image src, int sx, int sy, int sw, int sh,
            int dx, int dy) {
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cr = cx + g.getClipWidth();
        int cb = cy + g.getClipHeight();
        int x0 = dx < cx ? cx : dx;
        int y0 = dy < cy ? cy : dy;
        int x1 = dx + sw > cr ? cr : dx + sw;
        int y1 = dy + sh > cb ? cb : dy + sh;
        if (x0 >= x1 || y0 >= y1) {
            return;
        }
        int ow = x1 - x0;
        int oh = y1 - y0;
        int[] s = new int[ow * oh];
        src.getRGB(s, 0, ow, sx + (x0 - dx), sy + (y0 - dy), ow, oh);
        int[] d = new int[ow * oh];
        target.getRGB(d, 0, ow, x0 + originX, y0 + originY, ow, oh);
        for (int i = 0; i < d.length; i++) {
            int sp = s[i];
            int sa = sp >>> 24;
            if (sa == 0) {
                continue;
            }
            d[i] = mix(sp, sa, d[i]);
        }
        g.drawRGB(d, 0, ow, x0, y0, ow, oh, false);
    }

    /**
     * Combines source (alpha a) and destination per the opt render mode,
     * as openDoJa does (ratios 0..255):
     *   REPL: d*(255-a)/255 + s*a*src/255^2
     *   ADD:  d*((255-a)*255 + a*dst)/255^2 + s*a*src/255^2   (SUB: minus)
     */
    private int mix(int s, int a, int d) {
        int unc = (255 - a) * 255;
        int srcN = a * renderSrc;
        int dstN = renderOp == 0 ? unc : unc + a * renderDst;
        int sign = renderOp == 2 ? -1 : 1;
        int r = ch((d >> 16) & 255, (s >> 16) & 255, dstN, srcN, sign);
        int gg = ch((d >> 8) & 255, (s >> 8) & 255, dstN, srcN, sign);
        int b = ch(d & 255, s & 255, dstN, srcN, sign);
        return 0xff000000 | (r << 16) | (gg << 8) | b;
    }

    private static int ch(int d, int s, int dstN, int srcN, int sign) {
        int n = d * dstN + sign * s * srcN;   /* fits: 255 * 2 * 65025 < 2^31 */
        if (n <= 0) {
            return 0;
        }
        if (n >= 255 * 65025) {
            return 255;
        }
        return (n + 32512) / 65025;
    }

    /* ------------------------------------------------------------------ */
    /* 3D entry points (MascotCapsule v3 / DoJa graphics3d)                */

    public void setViewTrans(com.nttdocomo.opt.ui.j3d.AffineTrans transform) {
        FkBridge.render3d(this).setViewTrans(transform);
    }

    public void setViewTransArray(com.nttdocomo.opt.ui.j3d.AffineTrans[] transforms) {
        FkBridge.render3d(this).setViewTransArray(transforms);
    }

    public void setViewTrans(int index) {
        FkBridge.render3d(this).setViewTrans(index);
    }

    public void setScreenCenter(int x, int y) {
        FkBridge.render3d(this).setScreenCenter(x, y);
    }

    public void setScreenScale(int x, int y) {
        FkBridge.render3d(this).setScreenScale(x, y);
    }

    public void setScreenView(int x, int y) {
        FkBridge.render3d(this).setScreenView(x, y);
    }

    public void setPerspective(int near, int far, int w) {
        FkBridge.render3d(this).setPerspective(near, far, w);
    }

    public void setPerspective(int near, int far, int w, int h) {
        FkBridge.render3d(this).setPerspective(near, far, w, h);
    }

    public void drawFigure(com.nttdocomo.opt.ui.j3d.Figure figure) {
        FkBridge.render3d(this).drawFigure(figure);
        changed();
    }

    public void renderFigure(com.nttdocomo.opt.ui.j3d.Figure figure) {
        FkBridge.render3d(this).renderFigure(figure);
    }

    public void flush() {
        FkBridge.render3d(this).flush();
        changed();
    }

    public void enableSphereMap(boolean enabled) {
        FkBridge.render3d(this).enableSphereMap(enabled);
    }

    public void setSphereTexture(com.nttdocomo.opt.ui.j3d.Texture texture) {
        FkBridge.render3d(this).setSphereTexture(texture);
    }

    public void enableLight(boolean enabled) {
        FkBridge.render3d(this).enableLight(enabled);
    }

    public void setAmbientLight(int c) {
        FkBridge.render3d(this).setAmbientLight(c);
    }

    public void setDirectionLight(com.nttdocomo.opt.ui.j3d.Vector3D direction, int c) {
        FkBridge.render3d(this).setDirectionLight(direction, c);
    }

    public void enableSemiTransparent(boolean enabled) {
        FkBridge.render3d(this).enableSemiTransparent(enabled);
    }

    public void setClipRect3D(int x, int y, int w, int h) {
        FkBridge.render3d(this).setClipRect3D(x, y, w, h);
    }

    public void enableToonShader(boolean enabled) {
        FkBridge.render3d(this).enableToonShader(enabled);
    }

    public void setToonParam(int highlight, int mid, int shadow) {
        FkBridge.render3d(this).setToonParam(highlight, mid, shadow);
    }

    public void setPrimitiveTextureArray(com.nttdocomo.opt.ui.j3d.Texture texture) {
        FkBridge.render3d(this).setPrimitiveTextureArray(texture);
    }

    public void setPrimitiveTextureArray(com.nttdocomo.opt.ui.j3d.Texture[] textures) {
        FkBridge.render3d(this).setPrimitiveTextureArray(textures);
    }

    public void setPrimitiveTexture(int index) {
        FkBridge.render3d(this).setPrimitiveTexture(index);
    }

    public void renderPrimitives(com.nttdocomo.opt.ui.j3d.PrimitiveArray primitives, int attr) {
        FkBridge.render3d(this).renderPrimitives(primitives, 0, primitives.size(), attr);
    }

    public void renderPrimitives(com.nttdocomo.opt.ui.j3d.PrimitiveArray primitives,
            int start, int count, int attr) {
        FkBridge.render3d(this).renderPrimitives(primitives, start, count, attr);
    }

    public void executeCommandList(int[] commands) {
        FkBridge.render3d(this).executeCommandList(commands);
    }

    /* DoJa 5 graphics3d */
    public void setClipRectFor3D(int x, int y, int w, int h) {
    }

    public void setParallelView(int w, int h) {
    }

    public void setPerspectiveView(float a, float b, int w, int h) {
    }

    public void setPerspectiveView(float a, float b, float c) {
    }

    public void flushBuffer() {
    }

    public void setTransform(com.nttdocomo.ui.util3d.Transform transform) {
    }

    public void addLight(com.nttdocomo.ui.graphics3d.Light light,
            com.nttdocomo.ui.util3d.Transform transform) {
    }

    public void resetLights() {
    }

    public void setFog(com.nttdocomo.ui.graphics3d.Fog fog) {
    }

    public void renderObject3D(com.nttdocomo.ui.graphics3d.DrawableObject3D object,
            com.nttdocomo.ui.util3d.Transform transform) {
    }
}
