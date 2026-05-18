package com.nttdocomo.ui;

import java.io.IOException;

import com.funkey.doja.GifDecoder;
import com.funkey.doja.ResourceLoader;

public final class Image {
    private javax.microedition.lcdui.Image image;
    private int alpha = 255;
    private int transparentColor;
    private boolean transparentEnabled;

    Image(javax.microedition.lcdui.Image img) {
        image = img;
    }

    javax.microedition.lcdui.Image midpImage() {
        return image;
    }

    public static Image createImage(String name) throws IOException {
        byte[] data = new ResourceLoader().read(name);
        return createImage(data);
    }

    public static Image createImage(byte[] data) throws IOException {
        try {
            return new Image(javax.microedition.lcdui.Image.createImage(data, 0, data.length));
        } catch (IllegalArgumentException e) {
            GifDecoder.DecodedImage decoded = GifDecoder.decode(data);
            return new Image(javax.microedition.lcdui.Image.createRGBImage(
                    decoded.argb, decoded.width, decoded.height, decoded.hasAlpha));
        }
    }

    public static Image createImage(int width, int height) {
        return new Image(javax.microedition.lcdui.Image.createImage(width, height));
    }

    public Graphics getGraphics() {
        return new Graphics(image.getGraphics());
    }

    public int getWidth() {
        return image.getWidth();
    }

    public int getHeight() {
        return image.getHeight();
    }

    public void setAlpha(int value) {
        if (value < 0) {
            value = 0;
        } else if (value > 255) {
            value = 255;
        }
        alpha = value;
    }

    public int getAlpha() {
        return alpha;
    }

    public void setTransparentColor(int color) {
        transparentColor = color;
    }

    public int getTransparentColor() {
        return transparentColor;
    }

    public void setTransparentEnabled(boolean enabled) {
        transparentEnabled = enabled;
    }

    public boolean isTransparentEnabled() {
        return transparentEnabled;
    }

    public void dispose() {
        image = null;
    }
}
