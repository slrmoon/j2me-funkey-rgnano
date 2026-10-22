package com.nttdocomo.ui;

import com.funkey.doja.BitmapFont;
import com.funkey.doja.DoJaRuntime;
import com.nttdocomo.lang.XString;

/**
 * DoJa font backed by the bundled Japanese bitmap fonts.
 */
public class Font {
    public static final int TYPE_DEFAULT = 0;
    public static final int TYPE_HEADING = 1;
    public static final int FACE_SYSTEM = 0x71000000;
    public static final int FACE_MONOSPACE = 0x72000000;
    public static final int FACE_PROPORTIONAL = 0x73000000;
    public static final int STYLE_PLAIN = 0x70100000;
    public static final int STYLE_BOLD = 0x70110000;
    public static final int STYLE_ITALIC = 0x70120000;
    public static final int STYLE_BOLDITALIC = 0x70130000;
    public static final int SIZE_SMALL = 0x70000100;
    public static final int SIZE_MEDIUM = 0x70000200;
    public static final int SIZE_LARGE = 0x70000300;
    public static final int SIZE_TINY = 0x70000400;

    private static Font defaultFont;

    final BitmapFont bmp;
    final boolean bold;
    final int pixelSize;

    protected Font() {
        this(12, false);
    }

    Font(int px, boolean isBold) {
        pixelSize = px;
        bmp = BitmapFont.forPixelSize(px);
        bold = isBold;
    }

    /* ------------------------------------------------------------------ */

    private static boolean legacyProfile() {
        int major = DoJaRuntime.profileMajor();
        if (major > 0) {
            return major < 3;
        }
        return DoJaRuntime.drawWidth() > 0 && DoJaRuntime.drawWidth() < 200;
    }

    static int pixelsForSize(int size) {
        if (legacyProfile()) {
            switch (size) {
            case SIZE_SMALL:
                return 12;
            case SIZE_LARGE:
                return 16;
            default:
                return 12;
            }
        }
        switch (size) {
        case SIZE_TINY:
            return 12;
        case SIZE_SMALL:
            return 16;
        case SIZE_MEDIUM:
            return 24;
        case SIZE_LARGE:
            return 30;
        default:
            return 12;
        }
    }

    public static synchronized Font getDefaultFont() {
        if (defaultFont == null) {
            defaultFont = new Font(12, false);
        }
        return defaultFont;
    }

    public static synchronized void setDefaultFont(Font font) {
        if (font == null) {
            throw new NullPointerException("font");
        }
        defaultFont = font;
    }

    public static Font getFont(int value) {
        if (value == TYPE_DEFAULT) {
            return getDefaultFont();
        }
        if (value == TYPE_HEADING) {
            return new Font(pixelsForSize(legacyProfile() ? SIZE_MEDIUM : SIZE_SMALL), true);
        }
        int size = 0x70000000 | (value & 0x0000ff00);
        if ((value & 0x0000ff00) == 0) {
            size = legacyProfile() ? SIZE_MEDIUM : SIZE_TINY;
        }
        int style = 0x70000000 | (value & 0x00ff0000);
        boolean b = style == STYLE_BOLD || style == STYLE_BOLDITALIC;
        return new Font(pixelsForSize(size), b);
    }

    public static Font getFont(int faceAndStyle, int size) {
        int style = 0x70000000 | (faceAndStyle & 0x00ff0000);
        boolean b = style == STYLE_BOLD || style == STYLE_BOLDITALIC;
        if (size <= 0) {
            size = 12;
        }
        return new Font(size, b);
    }

    public static int[] getSupportedFontSizes() {
        return new int[] { 12, 16, 24, 30 };
    }

    /* ------------------------------------------------------------------ */

    public int getAscent() {
        return bmp.getAscent();
    }

    public int getDescent() {
        return bmp.getDescent();
    }

    public int getHeight() {
        return bmp.getHeight();
    }

    public int stringWidth(String text) {
        if (text == null) {
            throw new NullPointerException("text");
        }
        return bmp.stringWidth(text) + (bold && text.length() > 0 ? 1 : 0);
    }

    public int stringWidth(XString text) {
        return stringWidth(text.toString());
    }

    public int stringWidth(XString text, int offset, int length) {
        return stringWidth(text.toString().substring(offset, offset + length));
    }

    int charsWidth(char[] data, int off, int len) {
        return bmp.charsWidth(data, off, len);
    }

    public int getBBoxWidth(String text) {
        return stringWidth(text);
    }

    public int getBBoxWidth(XString text) {
        return stringWidth(text);
    }

    public int getBBoxWidth(XString text, int offset, int length) {
        return stringWidth(text, offset, length);
    }

    public int getBBoxHeight(String text) {
        return getHeight();
    }

    public int getBBoxHeight(XString text) {
        return getHeight();
    }

    public int getLineBreak(String text, int offset, int length, int width) {
        if (text == null) {
            throw new NullPointerException("text");
        }
        if (offset < 0 || length < 0 || offset + length > text.length()) {
            throw new StringIndexOutOfBoundsException();
        }
        if (width < 0) {
            throw new IllegalArgumentException("width");
        }
        int limit = offset + length;
        int used = 0;
        int i = offset;
        while (i < limit) {
            int w = bmp.charWidth(text.charAt(i));
            if (used + w > width) {
                break;
            }
            used += w;
            i++;
        }
        return i;
    }

    public int getLineBreak(XString text, int offset, int length, int width) {
        return getLineBreak(text.toString(), offset, length, width);
    }

    /* ------------------------------------------------------------------ */

    void draw(javax.microedition.lcdui.Graphics g, String s, int x, int baseline, int rgb) {
        bmp.draw(g, s, 0, s.length(), x, baseline - bmp.getAscent(), rgb, bold);
    }

    void draw(javax.microedition.lcdui.Graphics g, char[] s, int off, int len,
            int x, int baseline, int rgb) {
        bmp.draw(g, s, off, len, x, baseline - bmp.getAscent(), rgb, bold);
    }
}
