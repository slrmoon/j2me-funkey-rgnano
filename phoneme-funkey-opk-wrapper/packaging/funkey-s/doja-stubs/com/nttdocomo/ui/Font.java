package com.nttdocomo.ui;

public final class Font {
    public static final int FACE_SYSTEM = 0;
    public static final int STYLE_PLAIN = 0;
    public static final int STYLE_BOLD = 1;
    public static final int SIZE_SMALL = 8;
    public static final int SIZE_MEDIUM = 0;
    public static final int SIZE_LARGE = 16;

    private static Font defaultFont = new Font(javax.microedition.lcdui.Font.getDefaultFont());
    private javax.microedition.lcdui.Font font;

    private Font(javax.microedition.lcdui.Font f) {
        font = f;
    }

    static Font wrap(javax.microedition.lcdui.Font f) {
        return new Font(f);
    }

    javax.microedition.lcdui.Font midpFont() {
        return font;
    }

    public static Font getDefaultFont() {
        return defaultFont;
    }

    public static void setDefaultFont(Font f) {
        if (f != null) {
            defaultFont = f;
        }
    }

    public static Font getFont(int size) {
        int midpSize = javax.microedition.lcdui.Font.SIZE_MEDIUM;
        if (size == SIZE_SMALL) {
            midpSize = javax.microedition.lcdui.Font.SIZE_SMALL;
        } else if (size == SIZE_LARGE) {
            midpSize = javax.microedition.lcdui.Font.SIZE_LARGE;
        }
        return new Font(javax.microedition.lcdui.Font.getFont(
            javax.microedition.lcdui.Font.FACE_SYSTEM,
            javax.microedition.lcdui.Font.STYLE_PLAIN,
            midpSize));
    }

    public static Font getFont(int face, int style, int size) {
        int midpStyle = javax.microedition.lcdui.Font.STYLE_PLAIN;
        int midpSize = javax.microedition.lcdui.Font.SIZE_MEDIUM;
        if ((style & STYLE_BOLD) != 0) {
            midpStyle = javax.microedition.lcdui.Font.STYLE_BOLD;
        }
        if (size == SIZE_SMALL) {
            midpSize = javax.microedition.lcdui.Font.SIZE_SMALL;
        } else if (size == SIZE_LARGE) {
            midpSize = javax.microedition.lcdui.Font.SIZE_LARGE;
        }
        return new Font(javax.microedition.lcdui.Font.getFont(
            javax.microedition.lcdui.Font.FACE_SYSTEM, midpStyle, midpSize));
    }

    public int getHeight() {
        return font.getHeight();
    }

    public int getAscent() {
        return font.getBaselinePosition();
    }

    public int getDescent() {
        return font.getHeight() - font.getBaselinePosition();
    }

    public int stringWidth(String text) {
        if (com.funkey.doja.JapaneseFont.hasJapanese(text)) {
            return com.funkey.doja.JapaneseFont.stringWidth(text, font.charWidth(' '));
        }
        return font.stringWidth(text == null ? "" : text);
    }

    public int charWidth(char ch) {
        if (ch >= 0x80) {
            return com.funkey.doja.JapaneseFont.stringWidth(String.valueOf(ch), font.charWidth(' '));
        }
        return font.charWidth(ch);
    }

    public int getLineBreak(String text, int offset, int width, int flags) {
        if (text == null || offset >= text.length()) {
            return 0;
        }
        int count = 0;
        int used = 0;
        while (offset + count < text.length()) {
            int w = font.charWidth(text.charAt(offset + count));
            if (count > 0 && used + w > width) {
                break;
            }
            used += w;
            count++;
        }
        return count;
    }
}
