package com.nokia.mid.ui;

import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public final class DirectUtils {
    private DirectUtils() {
    }

    public static DirectGraphics getDirectGraphics(Graphics graphics) {
        return new DirectGraphicsImpl(graphics);
    }

    public static Image createImage(byte[] imageData, int imageOffset, int imageLength) {
        return Image.createImage(imageData, imageOffset, imageLength);
    }

    public static Image createImage(int width, int height, int argb) {
        return Image.createARGBImage(width, height, argb);
    }

    private static final class DirectGraphicsImpl implements DirectGraphics {
        private final Graphics graphics;
        private int argbColor;
        private static int loggedCalls;

        DirectGraphicsImpl(Graphics graphics) {
            this.graphics = graphics;
            this.argbColor = 0xff000000 | (graphics.getColor() & 0x00ffffff);
        }

        public void drawPixels(short[] pixels, boolean transparency, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb = new int[width * height];
            int row;
            int col;
            int out = 0;

            logOnce("drawPixels(short)", width, height, format, manipulation);
            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }
            if (format != TYPE_USHORT_4444 && format != TYPE_USHORT_4444_ARGB
                    && format != TYPE_USHORT_444_RGB && format != TYPE_USHORT_555_RGB
                    && format != TYPE_USHORT_1555_ARGB && format != TYPE_USHORT_565_RGB) {
                return;
            }
            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col] & 0xffff;
                    argb[out++] = pixelToArgb(value, transparency, format);
                }
            }
            drawArgb(argb, transparency, x, y, width, height, manipulation);
        }

        public void drawPixels(byte[] pixels, byte[] alpha, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb = new int[width * height];
            int row;
            int col;
            int out = 0;

            logOnce("drawPixels(byte)", width, height, format, manipulation);
            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }
            if (format == TYPE_BYTE_1_GRAY || format == TYPE_BYTE_1_GRAY_VERTICAL) {
                drawPackedGrayPixels(pixels, alpha, offset, scanlength, x, y,
                        width, height, manipulation, format);
                return;
            }
            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col] & 0xff;
                    int a = alpha == null ? 0xff : alpha[src + col] & 0xff;
                    int color = (format == TYPE_BYTE_332_RGB)
                            ? rgb332ToArgb(value) : grayToArgb(expandGray(value, format));
                    argb[out++] = (a << 24) | color;
                }
            }
            drawArgb(argb, true, x, y, width, height, manipulation);
        }

        public void drawPixels(int[] pixels, boolean transparency, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb = new int[width * height];
            int row;
            int col;
            int out = 0;

            logOnce("drawPixels(int)", width, height, format, manipulation);
            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }
            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col];
                    if (format == TYPE_INT_888_RGB || !transparency) {
                        value = 0xff000000 | (value & 0x00ffffff);
                    }
                    argb[out++] = value;
                }
            }
            drawArgb(argb, true, x, y, width, height, manipulation);
        }

        public void getPixels(short[] pixels, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int[] argb = readArgb(x, y, width, height);
            if (pixels == null || argb == null) {
                return;
            }
            for (int row = 0; row < height; row++) {
                int dst = offset + row * scanlength;
                for (int col = 0; col < width; col++) {
                    int p = argb[row * width + col];
                    int a = (p >>> 24), r = (p >> 16) & 0xff, g = (p >> 8) & 0xff, b = p & 0xff;
                    int v;
                    if (format == TYPE_USHORT_565_RGB) {
                        v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                    } else if (format == TYPE_USHORT_555_RGB) {
                        v = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                    } else if (format == TYPE_USHORT_1555_ARGB) {
                        v = (a >= 0x80 ? 0x8000 : 0) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                    } else if (format == TYPE_USHORT_444_RGB) {
                        v = ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
                    } else {   /* 4444 ARGB */
                        v = ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
                    }
                    int i = dst + col;
                    if (i >= 0 && i < pixels.length) {
                        pixels[i] = (short)v;
                    }
                }
            }
        }

        public void getPixels(byte[] pixels, byte[] alpha, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int[] argb = readArgb(x, y, width, height);
            if (pixels == null || argb == null) {
                return;
            }
            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    int p = argb[row * width + col];
                    int a = p >>> 24, r = (p >> 16) & 0xff, g = (p >> 8) & 0xff, b = p & 0xff;
                    boolean opaque = a >= 0x80;
                    if (format == TYPE_BYTE_1_GRAY) {
                        int bit = offset + row * scanlength + col;
                        int i = bit >> 3, m = 0x80 >> (bit & 7);
                        if (i < 0 || i >= pixels.length) continue;
                        boolean dark = (r * 30 + g * 59 + b * 11) / 100 < 128;
                        pixels[i] = (byte)(dark ? pixels[i] | m : pixels[i] & ~m);
                        if (alpha != null && i < alpha.length) {
                            alpha[i] = (byte)(opaque ? alpha[i] | m : alpha[i] & ~m);
                        }
                    } else {
                        int i = offset + row * scanlength + col;
                        if (i < 0 || i >= pixels.length) continue;
                        int v = format == TYPE_BYTE_332_RGB
                                ? ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6)
                                : (r * 30 + g * 59 + b * 11) / 100;
                        pixels[i] = (byte)v;
                        if (alpha != null && i < alpha.length) {
                            alpha[i] = (byte)a;
                        }
                    }
                }
            }
        }

        public void getPixels(int[] pixels, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int[] argb = readArgb(x, y, width, height);
            if (pixels == null || argb == null) {
                return;
            }
            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    int i = offset + row * scanlength + col;
                    if (i < 0 || i >= pixels.length) continue;
                    int p = argb[row * width + col];
                    pixels[i] = format == TYPE_INT_888_RGB ? (p & 0x00ffffff) : p;
                }
            }
        }

        /* the pixels of the image this Graphics draws into (the screen
         * cannot be read back: null) */
        private int[] readArgb(int x, int y, int width, int height) {
            logOnce("getPixels", width, height, 0, 0);
            if (width <= 0 || height <= 0) {
                return null;
            }
            com.sun.midp.lcdui.GraphicsAccess ga =
                    com.sun.midp.lcdui.GameMap.getGraphicsAccess();
            Image img = ga == null ? null : ga.getGraphicsImage(graphics);
            if (img == null) {
                return null;
            }
            x += graphics.getTranslateX();
            y += graphics.getTranslateY();
            int[] argb = new int[width * height];
            /* clip to the image; outside stays 0 (transparent black) */
            int x0 = Math.max(x, 0), y0 = Math.max(y, 0);
            int x1 = Math.min(x + width, img.getWidth()), y1 = Math.min(y + height, img.getHeight());
            if (x1 > x0 && y1 > y0) {
                img.getRGB(argb, (y0 - y) * width + (x0 - x), width, x0, y0, x1 - x0, y1 - y0);
            }
            return argb;
        }

        public void drawImage(Image image, int x, int y, int anchor, int manipulation) {
            int width;
            int height;
            int[] rgb;
            int[] transformed;
            int drawWidth;
            int drawHeight;
            int drawX;
            int drawY;

            if (image == null) {
                return;
            }

            width = image.getWidth();
            height = image.getHeight();
            rgb = new int[width * height];
            image.getRGB(rgb, 0, width, 0, 0, width, height);
            transformed = transformArgb(rgb, width, height, manipulation);
            drawWidth = transformedWidth(width, height, manipulation);
            drawHeight = transformedHeight(width, height, manipulation);
            drawX = resolveAnchorX(x, anchor, drawWidth);
            drawY = resolveAnchorY(y, anchor, drawHeight);
            graphics.drawRGB(transformed, 0, drawWidth, drawX, drawY,
                    drawWidth, drawHeight, true);
        }

        public void drawPolygon(int[] xPoints, int xOffset, int[] yPoints, int yOffset,
                int nPoints, int argb) {
            int i;

            setARGBColor(argb);
            if (xPoints == null || yPoints == null || nPoints <= 0) {
                return;
            }
            for (i = 0; i < nPoints; i++) {
                int next = (i + 1) % nPoints;
                graphics.drawLine(xPoints[xOffset + i], yPoints[yOffset + i],
                        xPoints[xOffset + next], yPoints[yOffset + next]);
            }
        }

        public void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
                int argb) {
            setARGBColor(argb);
            graphics.drawLine(x1, y1, x2, y2);
            graphics.drawLine(x2, y2, x3, y3);
            graphics.drawLine(x3, y3, x1, y1);
        }

        public void fillPolygon(int[] xPoints, int xOffset, int[] yPoints, int yOffset,
                int nPoints, int argb) {
            int i;

            if (xPoints == null || yPoints == null || nPoints < 3) {
                return;
            }
            for (i = 1; i < nPoints - 1; i++) {
                fillTriangle(xPoints[xOffset], yPoints[yOffset],
                        xPoints[xOffset + i], yPoints[yOffset + i],
                        xPoints[xOffset + i + 1], yPoints[yOffset + i + 1], argb);
            }
        }

        public void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
                int argb) {
            setARGBColor(argb);
            graphics.fillTriangle(x1, y1, x2, y2, x3, y3);
        }

        public void setARGBColor(int argb) {
            argbColor = argb;
            graphics.setColor(argb & 0x00ffffff);
        }

        public int getAlphaComponent() {
            return (argbColor >>> 24) & 0xff;
        }

        public int getNativePixelFormat() {
            return TYPE_USHORT_4444_ARGB;
        }

        private void drawArgb(int[] argb, boolean transparency, int x, int y,
                int width, int height, int manipulation) {
            int[] transformed = transformArgb(argb, width, height, manipulation);
            int drawWidth = transformedWidth(width, height, manipulation);
            graphics.drawRGB(transformed, 0, drawWidth, x, y, drawWidth,
                    transformed.length / drawWidth, transparency);
        }

        private int pixelToArgb(int value, boolean transparency, int format) {
            if (format == TYPE_USHORT_565_RGB) {
                int r = ((value >> 11) & 0x1f) * 255 / 31;
                int g = ((value >> 5) & 0x3f) * 255 / 63;
                int b = (value & 0x1f) * 255 / 31;
                return 0xff000000 | (r << 16) | (g << 8) | b;
            }
            if (format == TYPE_USHORT_555_RGB) {
                int r = ((value >> 10) & 0x1f) * 255 / 31;
                int g = ((value >> 5) & 0x1f) * 255 / 31;
                int b = (value & 0x1f) * 255 / 31;
                return 0xff000000 | (r << 16) | (g << 8) | b;
            }
            if (format == TYPE_USHORT_1555_ARGB) {
                int a = (transparency && ((value & 0x8000) == 0)) ? 0x00 : 0xff;
                int r = ((value >> 10) & 0x1f) * 255 / 31;
                int g = ((value >> 5) & 0x1f) * 255 / 31;
                int b = (value & 0x1f) * 255 / 31;
                return (a << 24) | (r << 16) | (g << 8) | b;
            }
            if (format == TYPE_USHORT_444_RGB) {
                int r = ((value >> 8) & 0x0f) * 17;
                int g = ((value >> 4) & 0x0f) * 17;
                int b = (value & 0x0f) * 17;
                return 0xff000000 | (r << 16) | (g << 8) | b;
            }
            if (format == TYPE_USHORT_4444 || format == TYPE_USHORT_4444_ARGB) {
                int a = transparency ? ((value >> 12) & 0x0f) * 17 : 0xff;
                int r = ((value >> 8) & 0x0f) * 17;
                int g = ((value >> 4) & 0x0f) * 17;
                int b = (value & 0x0f) * 17;
                return (a << 24) | (r << 16) | (g << 8) | b;
            }
            return 0xff000000 | (value & 0x00ffffff);
        }

        private int expandGray(int value, int format) {
            if (format == TYPE_BYTE_1_GRAY || format == TYPE_BYTE_1_GRAY_VERTICAL) {
                return value == 0 ? 0 : 255;
            }
            if (format == TYPE_BYTE_2_GRAY) {
                return (value & 0x03) * 85;
            }
            if (format == TYPE_BYTE_4_GRAY) {
                return (value & 0x0f) * 17;
            }
            if (format == TYPE_BYTE_332_RGB) {
                return rgb332ToArgb(value) & 0xff;
            }
            return value & 0xff;
        }

        private void drawPackedGrayPixels(byte[] pixels, byte[] alpha, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb = new int[width * height];
            int row;
            int col;
            int out = 0;

            if (format == TYPE_BYTE_1_GRAY) {
                /* offset and scanlength count pixels (bits), most
                 * significant bit first, as on the Nokia handsets */
                for (row = 0; row < height; row++) {
                    int src = offset + row * scanlength;
                    for (col = 0; col < width; col++) {
                        int bit = src + col;
                        argb[out++] = packedGrayToArgb(pixels, alpha, bit >> 3,
                                7 - (bit & 7));
                    }
                }
            } else {
                for (row = 0; row < height; row++) {
                    int bitShift = row & 7;
                    int srcBase = ((offset / scanlength) + row) / 8 * scanlength
                            + (offset % scanlength);
                    for (col = 0; col < width; col++) {
                        argb[out++] = packedGrayToArgb(pixels, alpha, srcBase + col,
                                bitShift);
                    }
                }
            }
            drawArgb(argb, true, x, y, width, height, manipulation);
        }

        /* 1-bit gray: a set bit is a dark pixel (black on the mono
         * screens these games were made for); opaque unless an alpha
         * mask clears the bit */
        private int packedGrayToArgb(byte[] pixels, byte[] alpha, int index, int shift) {
            if (index < 0 || index >= pixels.length) {
                return 0;
            }
            int color = isBitSet(pixels[index] & 0xff, shift) ? 0x00000000 : 0x00ffffff;
            int a = 0xff;

            if (alpha != null && index < alpha.length) {
                a = isBitSet(alpha[index] & 0xff, shift) ? 0xff : 0x00;
            }
            return (a << 24) | color;
        }

        private boolean isBitSet(int value, int shift) {
            return ((value >> shift) & 0x01) != 0;
        }

        private int rgb332ToArgb(int value) {
            int red = ((value >> 5) & 0x07) * 255 / 7;
            int green = ((value >> 2) & 0x07) * 255 / 7;
            int blue = (value & 0x03) * 255 / 3;
            return (red << 16) | (green << 8) | blue;
        }

        private int grayToArgb(int gray) {
            return (gray << 16) | (gray << 8) | gray;
        }

        private int[] transformArgb(int[] source, int width, int height, int manipulation) {
            if (manipulation == 0) {
                return source;
            }
            int dstWidth = transformedWidth(width, height, manipulation);
            int dstHeight = transformedHeight(width, height, manipulation);
            int[] dst = new int[dstWidth * dstHeight];
            int rotation = rotationPart(manipulation);
            int row;
            int col;

            for (row = 0; row < height; row++) {
                for (col = 0; col < width; col++) {
                    int dx;
                    int dy;
                    int value = source[row * width + col];

                    if (rotation == ROTATE_90) {
                        dx = row;
                        dy = width - 1 - col;
                    } else if (rotation == ROTATE_180) {
                        dx = width - 1 - col;
                        dy = height - 1 - row;
                    } else if (rotation == ROTATE_270) {
                        dx = height - 1 - row;
                        dy = col;
                    } else {
                        dx = col;
                        dy = row;
                    }
                    if ((manipulation & FLIP_HORIZONTAL) != 0) {
                        dx = dstWidth - 1 - dx;
                    }
                    if ((manipulation & FLIP_VERTICAL) != 0) {
                        dy = dstHeight - 1 - dy;
                    }
                    dst[dy * dstWidth + dx] = value;
                }
            }
            return dst;
        }

        private int transformedWidth(int width, int height, int manipulation) {
            int rotation = rotationPart(manipulation);
            return (rotation == ROTATE_90 || rotation == ROTATE_270) ? height : width;
        }

        private int transformedHeight(int width, int height, int manipulation) {
            int rotation = rotationPart(manipulation);
            return (rotation == ROTATE_90 || rotation == ROTATE_270) ? width : height;
        }

        private int resolveAnchorX(int x, int anchor, int width) {
            if ((anchor & Graphics.RIGHT) != 0) {
                return x - width;
            }
            if ((anchor & Graphics.HCENTER) != 0) {
                return x - width / 2;
            }
            return x;
        }

        private int resolveAnchorY(int y, int anchor, int height) {
            if ((anchor & Graphics.BOTTOM) != 0) {
                return y - height;
            }
            if ((anchor & Graphics.VCENTER) != 0) {
                return y - height / 2;
            }
            return y;
        }

        private int rotationPart(int manipulation) {
            int rotation = manipulation & 0xff;
            if (rotation == ROTATE_90 || rotation == ROTATE_180 || rotation == ROTATE_270) {
                return rotation;
            }
            if (manipulation == ROTATE_90 || manipulation == ROTATE_180 || manipulation == ROTATE_270) {
                return manipulation;
            }
            return 0;
        }

        private int toLcduiTransform(int manipulation) {
            int rotation = rotationPart(manipulation);
            int transform = 0;

            if (rotation == ROTATE_90) {
                transform = 5;
            } else if (rotation == ROTATE_180) {
                transform = 3;
            } else if (rotation == ROTATE_270) {
                transform = 6;
            }
            if ((manipulation & FLIP_HORIZONTAL) != 0) {
                transform ^= 2;
            }
            if ((manipulation & FLIP_VERTICAL) != 0) {
                transform ^= 1;
            }
            return transform;
        }

        private void logOnce(String name, int width, int height, int format, int manipulation) {
            if (loggedCalls < 12) {
                loggedCalls++;
                System.out.println("Nokia DirectGraphics." + name + " "
                        + width + "x" + height + " fmt=" + format
                        + " manip=" + manipulation);
            }
        }
    }
}
