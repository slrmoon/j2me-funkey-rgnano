package com.nttdocomo.opt.ui;

import com.nttdocomo.lang.UnsupportedOperationException;
import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.Graphics;
import com.nttdocomo.ui.Image;

/**
 * A copy of an image that is drawn with its own transparent color and
 * alpha (applied by the runtime when the image is drawn).
 */
public class TransparentImage extends Image {
    protected TransparentImage() {
    }

    public static TransparentImage createTransparentImage(Image image) {
        if (image == null) {
            throw new NullPointerException("image");
        }
        javax.microedition.lcdui.Image src = FkBridge.rawOf(image);
        int w = src.getWidth();
        int h = src.getHeight();
        int[] px = new int[w * h];
        src.getRGB(px, 0, w, 0, 0, w, h);
        TransparentImage t = new TransparentImage();
        FkBridge.setPixels(t, javax.microedition.lcdui.Image.createRGBImage(px, w, h, true));
        return t;
    }

    public void setTransparentEnabled(boolean enabled) {
        super.setTransparentEnabled(enabled);
    }

    public void setTransparentColor(int color) {
        super.setTransparentColor(color);
    }

    public int getTransparentColor() {
        return super.getTransparentColor();
    }

    public void setAlpha(int alpha) {
        super.setAlpha(alpha);
    }

    public int getAlpha() {
        return super.getAlpha();
    }

    public final Graphics getGraphics() {
        throw new UnsupportedOperationException("TransparentImage");
    }

    public void dispose() {
        super.dispose();
    }

    public int getWidth() {
        return super.getWidth();
    }

    public int getHeight() {
        return super.getHeight();
    }
}
