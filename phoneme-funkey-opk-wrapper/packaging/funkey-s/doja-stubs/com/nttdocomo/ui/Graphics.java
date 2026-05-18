package com.nttdocomo.ui;

public final class Graphics {
    public static final int LEFT = javax.microedition.lcdui.Graphics.LEFT;
    public static final int RIGHT = javax.microedition.lcdui.Graphics.RIGHT;
    public static final int HCENTER = javax.microedition.lcdui.Graphics.HCENTER;
    public static final int TOP = javax.microedition.lcdui.Graphics.TOP;
    public static final int BOTTOM = javax.microedition.lcdui.Graphics.BOTTOM;
    public static final int VCENTER = javax.microedition.lcdui.Graphics.VCENTER;

    private javax.microedition.lcdui.Graphics g;
    private Canvas owner;
    private Font font = Font.getDefaultFont();
    private int originX;
    private int originY;
    private int flipMode;

    Graphics(javax.microedition.lcdui.Graphics graphics) {
        this(graphics, null);
    }

    Graphics(javax.microedition.lcdui.Graphics graphics, Canvas canvas) {
        g = graphics;
        owner = canvas;
        if (font != null) {
            g.setFont(font.midpFont());
        }
    }

    public static int getColorOfRGB(int r, int green, int b) {
        return ((r & 255) << 16) | ((green & 255) << 8) | (b & 255);
    }

    public static int getColorOfRGB(int a, int r, int green, int b) {
        return getColorOfRGB(r, green, b);
    }

    public static int getColorOfName(int name) {
        switch (name) {
        case 0:
            return 0x000000;
        case 1:
            return 0xffffff;
        case 2:
            return 0xff0000;
        case 3:
            return 0x00ff00;
        case 4:
            return 0x0000ff;
        case 5:
            return 0xffff00;
        case 6:
            return 0x00ffff;
        case 7:
            return 0xff00ff;
        default:
            return name;
        }
    }

    public void lock() {
        ensureGraphics();
    }

    public void unlock(boolean repaint) {
        if (repaint && owner != null) {
            owner.repaint();
        }
    }

    public void setColor(int rgb) {
        ensureGraphics();
        g.setColor(rgb);
    }

    public void setOrigin(int x, int y) {
        originX = x;
        originY = y;
    }

    public void setFlipMode(int mode) {
        flipMode = mode;
    }

    public int getColor() {
        ensureGraphics();
        return g.getColor();
    }

    public void setFont(Font f) {
        font = f;
        ensureGraphics();
        if (f != null) {
            g.setFont(f.midpFont());
        }
    }

    public Font getFont() {
        return font;
    }

    public void drawString(String str, int x, int y) {
        ensureGraphics();
        if (com.funkey.doja.JapaneseFont.hasJapanese(str)) {
            com.funkey.doja.JapaneseFont.drawString(g, str, x + originX, y + originY);
        } else {
            g.drawString(str, x + originX, y + originY,
                    javax.microedition.lcdui.Graphics.LEFT | javax.microedition.lcdui.Graphics.TOP);
        }
    }

    public void drawString(String str, int x, int y, int anchor) {
        ensureGraphics();
        if (com.funkey.doja.JapaneseFont.hasJapanese(str)) {
            int drawX = x;
            int drawY = y;
            int width = com.funkey.doja.JapaneseFont.stringWidth(
                str, javax.microedition.lcdui.Font.getDefaultFont().charWidth(' '));
            if ((anchor & javax.microedition.lcdui.Graphics.HCENTER) != 0) {
                drawX -= width / 2;
            } else if ((anchor & javax.microedition.lcdui.Graphics.RIGHT) != 0) {
                drawX -= width;
            }
            if ((anchor & javax.microedition.lcdui.Graphics.VCENTER) != 0) {
                drawY -= 7;
            } else if ((anchor & javax.microedition.lcdui.Graphics.BOTTOM) != 0) {
                drawY -= 14;
            }
            com.funkey.doja.JapaneseFont.drawString(g, str,
                    drawX + originX, drawY + originY);
        } else {
            g.drawString(str, x + originX, y + originY, anchor);
        }
    }

    public void drawImage(Image image, int x, int y) {
        ensureGraphics();
        if (image != null) {
            drawImageRegion(image, 0, 0, image.getWidth(), image.getHeight(),
                    x + originX, y + originY);
        }
    }

    public void drawImage(Image image, int x, int y, int anchor) {
        ensureGraphics();
        if (image != null) {
            if (flipMode == 0) {
                g.drawImage(image.midpImage(), x + originX, y + originY, anchor);
            } else {
                drawImageRegion(image, 0, 0, image.getWidth(), image.getHeight(),
                        x + originX, y + originY);
            }
        }
    }

    public void drawImage(Image image, int[] params) {
        if (params == null || params.length < 2) {
            drawImage(image, 0, 0);
            return;
        }
        drawImage(image, params[0], params[1]);
    }

    public void drawImage(Image image, int sx, int sy, int sw, int sh,
            int dx, int dy) {
        ensureGraphics();
        if (image != null) {
            drawImageRegion(image, sx, sy, sw, sh, dx + originX, dy + originY);
        }
    }

    public void drawLine(int x1, int y1, int x2, int y2) {
        ensureGraphics();
        g.drawLine(x1 + originX, y1 + originY, x2 + originX, y2 + originY);
    }

    public void drawRect(int x, int y, int w, int h) {
        ensureGraphics();
        g.drawRect(x + originX, y + originY, w, h);
    }

    public void fillRect(int x, int y, int w, int h) {
        ensureGraphics();
        g.fillRect(x + originX, y + originY, w, h);
    }

    public void clearRect(int x, int y, int w, int h) {
        ensureGraphics();
        int old = g.getColor();
        g.setColor(0xffffff);
        g.fillRect(x + originX, y + originY, w, h);
        g.setColor(old);
    }

    public void setPictoColorEnabled(boolean enabled) {
    }

    public void dispose() {
        /*
         * DoJa code commonly treats Graphics obtained from Canvas as a
         * reusable drawing context. Keep it alive for later lock/unlock cycles.
         */
    }

    private void ensureGraphics() {
        if (g == null && owner != null) {
            g = owner.backBufferGraphics();
            if (font != null) {
                g.setFont(font.midpFont());
            }
        }
    }

    private void drawImageRegion(Image image, int sx, int sy, int sw, int sh,
            int dx, int dy) {
        int imageWidth = image.getWidth();
        int imageHeight = image.getHeight();
        if (imageWidth <= 0 || imageHeight <= 0 || sw == 0 || sh == 0) {
            return;
        }
        if (sw < 0) {
            sx += sw;
            sw = -sw;
        }
        if (sh < 0) {
            sy += sh;
            sh = -sh;
        }
        if (sx < 0) {
            dx -= sx;
            sw += sx;
            sx = 0;
        }
        if (sy < 0) {
            dy -= sy;
            sh += sy;
            sy = 0;
        }
        if (sx + sw > imageWidth) {
            sw = imageWidth - sx;
        }
        if (sy + sh > imageHeight) {
            sh = imageHeight - sy;
        }
        if (sw <= 0 || sh <= 0 || sx >= imageWidth || sy >= imageHeight) {
            return;
        }

        int transform = javax.microedition.lcdui.game.Sprite.TRANS_NONE;
        if (flipMode == 1) {
            transform = javax.microedition.lcdui.game.Sprite.TRANS_MIRROR;
        } else if (flipMode == 2) {
            transform = javax.microedition.lcdui.game.Sprite.TRANS_MIRROR_ROT180;
        } else if (flipMode == 3) {
            transform = javax.microedition.lcdui.game.Sprite.TRANS_ROT180;
        }
        try {
            if (transform == javax.microedition.lcdui.game.Sprite.TRANS_NONE
                    && sx == 0 && sy == 0 && sw == imageWidth && sh == imageHeight) {
                g.drawImage(image.midpImage(), dx, dy,
                        javax.microedition.lcdui.Graphics.LEFT | javax.microedition.lcdui.Graphics.TOP);
            } else {
                g.drawRegion(image.midpImage(), sx, sy, sw, sh, transform, dx, dy,
                        javax.microedition.lcdui.Graphics.LEFT | javax.microedition.lcdui.Graphics.TOP);
            }
        } catch (IllegalArgumentException e) {
            g.drawImage(image.midpImage(), dx, dy,
                    javax.microedition.lcdui.Graphics.LEFT | javax.microedition.lcdui.Graphics.TOP);
        }
    }
}
