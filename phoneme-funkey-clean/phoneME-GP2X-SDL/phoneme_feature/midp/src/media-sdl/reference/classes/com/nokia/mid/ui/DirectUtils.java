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
        Image image = Image.createImage(width, height);
        Graphics graphics = image.getGraphics();
        graphics.setColor(argb & 0x00ffffff);
        graphics.fillRect(0, 0, width, height);
        graphics.setColor(0);
        return image;
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
            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col] & 0xff;
                    int a = alpha == null ? 0xff : alpha[src + col] & 0xff;
                    int gray = expandGray(value, format);
                    argb[out++] = (a << 24) | (gray << 16) | (gray << 8) | gray;
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
        }

        public void getPixels(byte[] pixels, byte[] alpha, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
        }

        public void getPixels(int[] pixels, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
        }

        public void drawImage(Image image, int x, int y, int anchor, int manipulation) {
            if (image == null) {
                return;
            }
            graphics.drawRegion(image, 0, 0, image.getWidth(), image.getHeight(),
                    toLcduiTransform(manipulation), x, y, anchor);
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
                return value;
            }
            return value & 0xff;
        }

        private int[] transformArgb(int[] source, int width, int height, int manipulation) {
            if (manipulation == 0) {
                return source;
            }
            /* The Nokia flags are close enough to MIDP sprite transforms for drawRegion,
             * but drawRGB needs a small local transform path. */
            int dstWidth = transformedWidth(width, height, manipulation);
            int dstHeight = transformedHeight(width, height, manipulation);
            int[] dst = new int[dstWidth * dstHeight];
            int row;
            int col;

            for (row = 0; row < height; row++) {
                for (col = 0; col < width; col++) {
                    int dx = ((manipulation & FLIP_HORIZONTAL) != 0) ? width - 1 - col : col;
                    int dy = ((manipulation & FLIP_VERTICAL) != 0) ? height - 1 - row : row;
                    int value = source[row * width + col];

                    switch (rotationPart(manipulation)) {
                    case ROTATE_90:
                        dst[dx * dstWidth + (dstWidth - 1 - dy)] = value;
                        break;
                    case ROTATE_180:
                        dst[(dstHeight - 1 - dy) * dstWidth + (dstWidth - 1 - dx)] = value;
                        break;
                    case ROTATE_270:
                        dst[(dstHeight - 1 - dx) * dstWidth + dy] = value;
                        break;
                    default:
                        dst[dy * dstWidth + dx] = value;
                        break;
                    }
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
            boolean mirror = (manipulation & FLIP_HORIZONTAL) != 0;

            if (mirror && rotation == ROTATE_90) {
                return 7;
            }
            if (mirror && rotation == ROTATE_180) {
                return 2;
            }
            if (mirror && rotation == ROTATE_270) {
                return 4;
            }
            if (mirror) {
                return 2;
            }
            if (rotation == ROTATE_90) {
                return 5;
            }
            if (rotation == ROTATE_180) {
                return 3;
            }
            if (rotation == ROTATE_270) {
                return 6;
            }
            return 0;
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
