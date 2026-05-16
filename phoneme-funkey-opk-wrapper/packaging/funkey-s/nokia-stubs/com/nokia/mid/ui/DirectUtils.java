package com.nokia.mid.ui;

import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public final class DirectUtils {
    private static final int SHADOW_WIDTH = 256;
    private static final int SHADOW_HEIGHT = 256;
    private static final int[] shadowArgb = new int[SHADOW_WIDTH * SHADOW_HEIGHT];

    private DirectUtils() {
    }

    public static DirectGraphics getDirectGraphics(Graphics graphics) {
        return new DirectGraphicsImpl(graphics);
    }

    public static Image createImage(byte[] imageData, int imageOffset, int imageLength) {
        Image source = Image.createImage(imageData, imageOffset, imageLength);
        Image target = Image.createImage(source.getWidth(), source.getHeight());

        target.getGraphics().drawImage(source, 0, 0, 0);
        return target;
    }

    public static Image createImage(int width, int height, int argb) {
        return Image.createARGBImage(width, height, argb);
    }

    private static final class DirectGraphicsImpl implements DirectGraphics {
        private final Graphics graphics;
        private static int drawCalls = 0;
        private static int drawFailures = 0;
        private int argbColor;

        DirectGraphicsImpl(Graphics graphics) {
            this.graphics = graphics;
            this.argbColor = 0xff000000 | (graphics.getColor() & 0x00ffffff);
        }

        public void drawPixels(short[] pixels, boolean transparency, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            try {
                drawCalls++;
                if (drawCalls <= 8) {
                    System.out.println("Nokia DirectGraphics.drawPixels call=" + drawCalls
                            + " len=" + (pixels == null ? -1 : pixels.length)
                            + " trans=" + transparency
                            + " off=" + offset
                            + " scan=" + scanlength
                            + " xy=" + x + "," + y
                            + " size=" + width + "x" + height
                            + " fmt=" + format
                            + " manip=" + manipulation);
                }

                if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                    return;
                }
                if (format != TYPE_USHORT_4444 && format != TYPE_USHORT_4444_ARGB) {
                    return;
                }

                draw4444(pixels, transparency, offset, scanlength, x, y,
                        width, height, manipulation);
            } catch (Throwable t) {
                drawFailures++;
                if (drawFailures <= 8) {
                    System.out.println("Nokia DirectGraphics.drawPixels failed: "
                            + t.getClass().getName() + ": " + t.getMessage());
                    t.printStackTrace();
                }
            }
        }

        public void getPixels(short[] pixels, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int row;
            int col;

            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }
            if (format != TYPE_USHORT_4444 && format != TYPE_USHORT_4444_ARGB) {
                return;
            }

            for (row = 0; row < height; row++) {
                int dst = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    pixels[dst + col] = argbTo4444(readShadow(x + col, y + row));
                }
            }
        }

        public void drawPixels(byte[] pixels, byte[] alpha, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb;

            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }

            if (format == TYPE_BYTE_1_GRAY || format == TYPE_BYTE_1_GRAY_VERTICAL) {
                drawPackedGrayPixels(pixels, alpha, offset, scanlength, x, y, width, height, format);
                return;
            }

            argb = decodeBytePixels(pixels, alpha, offset, scanlength, width, height, format);
            if (argb == null) {
                return;
            }
            drawArgb(argb, true, x, y, width, height, manipulation);
        }

        public void drawPixels(int[] pixels, boolean transparency, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation, int format) {
            int[] argb;
            int row;
            int col;
            int out;

            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }
            if (format != TYPE_INT_8888_ARGB && format != TYPE_INT_888_RGB) {
                return;
            }

            argb = new int[width * height];
            out = 0;
            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col];
                    if (format == TYPE_INT_888_RGB) {
                        value = 0xff000000 | (value & 0x00ffffff);
                    } else if (!transparency) {
                        value = 0xff000000 | (value & 0x00ffffff);
                    }
                    argb[out++] = value;
                }
            }

            drawArgb(argb, transparency || format == TYPE_INT_8888_ARGB,
                    x, y, width, height, manipulation);
        }

        public void getPixels(byte[] pixels, byte[] alpha, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int row;
            int col;

            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }

            for (row = 0; row < height; row++) {
                int dst = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int argb = readShadow(x + col, y + row);
                    writeBytePixel(pixels, alpha, dst + col, argb, format);
                }
            }
        }

        public void getPixels(int[] pixels, int offset, int scanlength,
                int x, int y, int width, int height, int format) {
            int row;
            int col;

            if (pixels == null || width <= 0 || height <= 0 || scanlength <= 0) {
                return;
            }

            for (row = 0; row < height; row++) {
                int dst = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int argb = readShadow(x + col, y + row);
                    pixels[dst + col] = (format == TYPE_INT_888_RGB)
                            ? (argb & 0x00ffffff) : argb;
                }
            }
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
            writeShadow(drawX, drawY, transformed, drawWidth, drawHeight);
        }

        public void drawPolygon(int[] xPoints, int xOffset, int[] yPoints, int yOffset,
                int nPoints, int argb) {
            int i;

            if (xPoints == null || yPoints == null || nPoints <= 0) {
                return;
            }
            setARGBColor(argb);
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
            this.argbColor = argb;
            graphics.setColor(argb & 0x00ffffff);
        }

        public int getAlphaComponent() {
            return (argbColor >>> 24) & 0xff;
        }

        public int getNativePixelFormat() {
            return TYPE_BYTE_1_GRAY;
        }

        private void draw4444(short[] pixels, boolean transparency, int offset,
                int scanlength, int x, int y, int width, int height,
                int manipulation) {
            int[] argb;

            argb = decode4444(pixels, transparency, offset, scanlength, width, height);
            drawArgb(argb, transparency, x, y, width, height, manipulation);
        }

        private void drawArgb(int[] argb, boolean transparency, int x, int y, int width,
                int height, int manipulation) {
            int[] transformed = transformArgb(argb, width, height, manipulation);
            int drawWidth = transformedWidth(width, height, manipulation);
            int drawHeight = transformedHeight(width, height, manipulation);

            graphics.drawRGB(transformed, 0, drawWidth, x, y, drawWidth, drawHeight, transparency);
            writeShadow(x, y, transformed, drawWidth, drawHeight);
        }

        private int[] decode4444(short[] pixels, boolean transparency, int offset,
                int scanlength, int width, int height) {
            int[] argb = new int[width * height];
            int out = 0;
            int row;
            int col;

            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col] & 0xffff;
                    int alpha = transparency ? ((value >> 12) & 0x0f) * 17 : 0xff;
                    int red = ((value >> 8) & 0x0f) * 17;
                    int green = ((value >> 4) & 0x0f) * 17;
                    int blue = (value & 0x0f) * 17;
                    argb[out++] = (alpha << 24) | (red << 16) | (green << 8) | blue;
                }
            }

            return argb;
        }

        private int[] decodeBytePixels(byte[] pixels, byte[] alpha, int offset,
                int scanlength, int width, int height, int format) {
            int[] argb = new int[width * height];
            int out = 0;
            int row;
            int col;

            for (row = 0; row < height; row++) {
                int src = offset + row * scanlength;
                for (col = 0; col < width; col++) {
                    int value = pixels[src + col] & 0xff;
                    int a = alpha == null ? 0xff : alpha[src + col] & 0xff;
                    int color;

                    switch (format) {
                    case TYPE_BYTE_2_GRAY:
                    case TYPE_BYTE_4_GRAY:
                    case TYPE_BYTE_8_GRAY:
                        color = expandGray(value, format);
                        argb[out++] = (a << 24) | (color << 16) | (color << 8) | color;
                        break;
                    case TYPE_BYTE_332_RGB:
                        argb[out++] = (a << 24) | rgb332ToArgb(value);
                        break;
                    default:
                        return null;
                    }
                }
            }

            return argb;
        }

        private void drawPackedGrayPixels(byte[] pixels, byte[] alpha, int offset,
                int scanlength, int x, int y, int width, int height, int format) {
            int row;
            int col;
            int bitShift;

            if (format == TYPE_BYTE_1_GRAY) {
                for (row = 0; row < height; row++) {
                    int src = offset + row * scanlength;
                    bitShift = 7;
                    for (col = 0; col < width; col++) {
                        int argb = doAlpha(pixels, alpha, src + (col / 8), bitShift);

                        if (((argb >>> 24) & 0xff) != 0) {
                            graphics.setColor(argb & 0x00ffffff);
                            graphics.drawLine(x + col, y + row, x + col, y + row);
                            writeShadow(x + col, y + row, argb);
                        }

                        bitShift--;
                        if (bitShift < 0) {
                            bitShift = 7;
                        }
                    }
                }
                return;
            }

            bitShift = 0;
            for (row = 0; row < height; row++) {
                int srcBase = ((offset / scanlength) + row) / 8 * scanlength + (offset % scanlength);
                for (col = 0; col < width; col++) {
                    int argb = doAlpha(pixels, alpha, srcBase + col, bitShift);

                    if (((argb >>> 24) & 0xff) != 0) {
                        graphics.setColor(argb & 0x00ffffff);
                        graphics.drawLine(x + col, y + row, x + col, y + row);
                        writeShadow(x + col, y + row, argb);
                    }
                }

                bitShift++;
                if (bitShift > 7) {
                    bitShift = 0;
                }
            }
        }

        private int[] transformArgb(int[] source, int width, int height, int manipulation) {
            int dstWidth = transformedWidth(width, height, manipulation);
            int dstHeight = transformedHeight(width, height, manipulation);
            int[] transformed = new int[dstWidth * dstHeight];
            int transform = toLcduiTransform(manipulation);
            int row;
            int col;

            for (row = 0; row < height; row++) {
                for (col = 0; col < width; col++) {
                    int dx = ((transform & 2) != 0) ? width - 1 - col : col;
                    int dy = ((transform & 1) != 0) ? height - 1 - row : row;
                    int value = source[row * width + col];

                    if ((transform & 4) != 0) {
                        transformed[dx * dstWidth + dy] = value;
                    } else {
                        transformed[dy * dstWidth + dx] = value;
                    }
                }
            }

            return transformed;
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

        private int resolveAnchorX(int x, int anchor, int width) {
            if ((anchor & Graphics.RIGHT) != 0) {
                return x - width;
            }
            if ((anchor & Graphics.HCENTER) != 0) {
                return x - (width >> 1);
            }
            return x;
        }

        private int resolveAnchorY(int y, int anchor, int height) {
            if ((anchor & Graphics.BOTTOM) != 0) {
                return y - height;
            }
            if ((anchor & Graphics.VCENTER) != 0) {
                return y - (height >> 1);
            }
            return y;
        }

        private short argbTo4444(int argb) {
            int alpha = (argb >>> 24) & 0xff;
            int red = (argb >>> 16) & 0xff;
            int green = (argb >>> 8) & 0xff;
            int blue = argb & 0xff;
            return (short)(((alpha >> 4) << 12) | ((red >> 4) << 8)
                    | ((green >> 4) << 4) | (blue >> 4));
        }

        private int expandGray(int value, int format) {
            switch (format) {
            case TYPE_BYTE_1_GRAY:
            case TYPE_BYTE_1_GRAY_VERTICAL:
                return value == 0 ? 0 : 255;
            case TYPE_BYTE_2_GRAY:
                return (value & 0x03) * 85;
            case TYPE_BYTE_4_GRAY:
                return (value & 0x0f) * 17;
            default:
                return value & 0xff;
            }
        }

        private int rgb332ToArgb(int value) {
            int red = ((value >> 5) & 0x07) * 255 / 7;
            int green = ((value >> 2) & 0x07) * 255 / 7;
            int blue = (value & 0x03) * 255 / 3;
            return (red << 16) | (green << 8) | blue;
        }

        private int doAlpha(byte[] pixels, byte[] alpha, int index, int shift) {
            int pixelByte = pixels[index] & 0xff;
            int color = isBitSet(pixelByte, shift) ? 0x00ffffff : 0x00000000;
            int a;

            if (alpha == null) {
                a = (color == 0) ? 0 : 0xff;
            } else {
                int alphaByte = alpha[index] & 0xff;
                a = isBitSet(alphaByte, shift) ? 0xff : 0x00;
            }

            return (a << 24) | color;
        }

        private boolean isBitSet(int value, int shift) {
            return ((value >> shift) & 0x01) != 0;
        }

        private void writeBytePixel(byte[] pixels, byte[] alpha, int index, int argb, int format) {
            int red = (argb >>> 16) & 0xff;
            int green = (argb >>> 8) & 0xff;
            int blue = argb & 0xff;
            int gray = (red * 30 + green * 59 + blue * 11) / 100;

            if (alpha != null) {
                alpha[index] = (byte)((argb >>> 24) & 0xff);
            }

            switch (format) {
            case TYPE_BYTE_1_GRAY:
            case TYPE_BYTE_1_GRAY_VERTICAL:
                pixels[index] = (byte)(gray >= 128 ? 1 : 0);
                break;
            case TYPE_BYTE_2_GRAY:
                pixels[index] = (byte)(gray / 85);
                break;
            case TYPE_BYTE_4_GRAY:
                pixels[index] = (byte)(gray / 17);
                break;
            case TYPE_BYTE_8_GRAY:
                pixels[index] = (byte)gray;
                break;
            case TYPE_BYTE_332_RGB:
                pixels[index] = (byte)(((red * 7 / 255) << 5)
                        | ((green * 7 / 255) << 2)
                        | (blue * 3 / 255));
                break;
            default:
                pixels[index] = (byte)gray;
                break;
            }
        }

        private void writeShadow(int x, int y, int argb) {
            if (x < 0 || y < 0 || x >= SHADOW_WIDTH || y >= SHADOW_HEIGHT) {
                return;
            }
            shadowArgb[y * SHADOW_WIDTH + x] =
                    blendArgb(shadowArgb[y * SHADOW_WIDTH + x], argb);
        }

        private void writeShadow(int x, int y, int[] argb, int width, int height) {
            int row;
            int col;

            for (row = 0; row < height; row++) {
                int sy = y + row;
                if (sy < 0 || sy >= SHADOW_HEIGHT) {
                    continue;
                }
                for (col = 0; col < width; col++) {
                    int sx = x + col;
                    int value;

                    if (sx < 0 || sx >= SHADOW_WIDTH) {
                        continue;
                    }
                    value = argb[row * width + col];
                    shadowArgb[sy * SHADOW_WIDTH + sx] =
                            blendArgb(shadowArgb[sy * SHADOW_WIDTH + sx], value);
                }
            }
        }

        private int blendArgb(int dst, int src) {
            int alpha = (src >>> 24) & 0xff;
            int invAlpha;
            int srcRed;
            int srcGreen;
            int srcBlue;
            int dstRed;
            int dstGreen;
            int dstBlue;
            int red;
            int green;
            int blue;

            if (alpha <= 0) {
                return dst;
            }
            if (alpha >= 255) {
                return src;
            }

            invAlpha = 255 - alpha;
            srcRed = (src >>> 16) & 0xff;
            srcGreen = (src >>> 8) & 0xff;
            srcBlue = src & 0xff;
            dstRed = (dst >>> 16) & 0xff;
            dstGreen = (dst >>> 8) & 0xff;
            dstBlue = dst & 0xff;

            red = (srcRed * alpha + dstRed * invAlpha) / 255;
            green = (srcGreen * alpha + dstGreen * invAlpha) / 255;
            blue = (srcBlue * alpha + dstBlue * invAlpha) / 255;
            return 0xff000000 | (red << 16) | (green << 8) | blue;
        }

        private int readShadow(int x, int y) {
            if (x < 0 || y < 0 || x >= SHADOW_WIDTH || y >= SHADOW_HEIGHT) {
                return 0;
            }
            return shadowArgb[y * SHADOW_WIDTH + x];
        }
    }
}
