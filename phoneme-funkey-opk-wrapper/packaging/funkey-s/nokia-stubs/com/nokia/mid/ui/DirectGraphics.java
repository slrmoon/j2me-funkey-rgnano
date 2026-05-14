package com.nokia.mid.ui;

public interface DirectGraphics {
    int TYPE_BYTE_1_GRAY = 1;
    int TYPE_BYTE_1_GRAY_VERTICAL = -1;
    int TYPE_BYTE_2_GRAY = 2;
    int TYPE_BYTE_4_GRAY = 4;
    int TYPE_BYTE_8_GRAY = 8;
    int TYPE_BYTE_332_RGB = 332;
    int TYPE_USHORT_4444_ARGB = 4444;
    int TYPE_USHORT_4444 = 4444;
    int TYPE_USHORT_444_RGB = 444;
    int TYPE_USHORT_555_RGB = 555;
    int TYPE_USHORT_1555_ARGB = 1555;
    int TYPE_USHORT_565_RGB = 565;
    int TYPE_INT_888_RGB = 888;
    int TYPE_INT_8888_ARGB = 8888;
    int FLIP_HORIZONTAL = 8192;
    int FLIP_VERTICAL = 16384;
    int ROTATE_90 = 90;
    int ROTATE_180 = 180;
    int ROTATE_270 = 270;

    void drawPixels(short[] pixels, boolean transparency, int offset,
            int scanlength, int x, int y, int width, int height,
            int manipulation, int format);

    void drawPixels(byte[] pixels, byte[] alpha, int offset,
            int scanlength, int x, int y, int width, int height,
            int manipulation, int format);

    void drawPixels(int[] pixels, boolean transparency, int offset,
            int scanlength, int x, int y, int width, int height,
            int manipulation, int format);

    void getPixels(short[] pixels, int offset, int scanlength,
            int x, int y, int width, int height, int format);

    void getPixels(byte[] pixels, byte[] alpha, int offset, int scanlength,
            int x, int y, int width, int height, int format);

    void getPixels(int[] pixels, int offset, int scanlength,
            int x, int y, int width, int height, int format);

    void drawImage(javax.microedition.lcdui.Image image, int x, int y,
            int anchor, int manipulation);

    void drawPolygon(int[] xPoints, int xOffset, int[] yPoints, int yOffset,
            int nPoints, int argbColor);

    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
            int argbColor);

    void fillPolygon(int[] xPoints, int xOffset, int[] yPoints, int yOffset,
            int nPoints, int argbColor);

    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
            int argbColor);

    void setARGBColor(int argb);

    int getAlphaComponent();

    int getNativePixelFormat();
}
