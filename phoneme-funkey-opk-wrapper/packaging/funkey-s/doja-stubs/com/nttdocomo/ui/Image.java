package com.nttdocomo.ui;

import java.io.IOException;

import com.funkey.doja.GifDecoder;
import com.funkey.doja.ResourceLoader;

/**
 * DoJa image. Wraps a MIDP image; transparency color and alpha are applied
 * through a cached derived image.
 */
public abstract class Image {
    javax.microedition.lcdui.Image img;
    boolean mutable;
    int transparentColor;
    boolean transparentEnabled;
    int alpha = 255;
    boolean disposed;
    /** incremented whenever the pixels change (drawing into a mutable image) */
    int version;

    private javax.microedition.lcdui.Image derived;
    private int derivedKeyColor;
    private boolean derivedKeyEnabled;
    private int derivedAlpha = 255;
    private int derivedVersion = -1;

    protected Image() {
    }

    Image(javax.microedition.lcdui.Image image, boolean isMutable) {
        img = image;
        mutable = isMutable;
    }

    /* ------------------------------------------------------------------ */

    public static Image createImage(int width, int height) {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("size");
        }
        return new FkImage(javax.microedition.lcdui.Image.createImage(width, height), true);
    }

    public static Image createImage(int width, int height, int[] pixels, int offset) {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("size");
        }
        int[] argb = new int[width * height];
        for (int i = 0; i < argb.length; i++) {
            argb[i] = 0xff000000 | pixels[offset + i];
        }
        javax.microedition.lcdui.Image rgb = javax.microedition.lcdui.Image.createRGBImage(
                argb, width, height, false);
        javax.microedition.lcdui.Image m = javax.microedition.lcdui.Image.createImage(width, height);
        m.getGraphics().drawImage(rgb, 0, 0, 20);
        return new FkImage(m, true);
    }

    /** Decodes GIF, PNG or JPEG data (used by MediaManager). */
    static Image decode(byte[] data) throws IOException {
        if (data == null || data.length < 4) {
            throw new UIException(UIException.UNSUPPORTED_FORMAT, "image data");
        }
        if (data[0] == 'G' && data[1] == 'I' && data[2] == 'F') {
            GifDecoder.DecodedImage d = GifDecoder.decode(data);
            return new FkImage(javax.microedition.lcdui.Image.createRGBImage(
                    d.argb, d.width, d.height, d.hasAlpha), false);
        }
        try {
            return new FkImage(javax.microedition.lcdui.Image.createImage(data, 0, data.length), false);
        } catch (IllegalArgumentException e) {
            StringBuffer sb = new StringBuffer("image data (");
            sb.append(data.length).append(" bytes:");
            for (int i = 0; i < 8 && i < data.length; i++) {
                sb.append(' ').append(Integer.toHexString(data[i] & 0xff));
            }
            throw new UIException(UIException.UNSUPPORTED_FORMAT, sb.append(')').toString());
        }
    }

    static Image load(String name) throws IOException {
        return decode(new ResourceLoader().read(name));
    }

    /* ------------------------------------------------------------------ */

    public Graphics getGraphics() {
        checkDisposed();
        if (!mutable) {
            /* handsets refuse this; converting keeps games that probe it alive */
            javax.microedition.lcdui.Image m = javax.microedition.lcdui.Image.createImage(
                    img.getWidth(), img.getHeight());
            m.getGraphics().drawImage(img, 0, 0, 20);
            img = m;
            mutable = true;
            version++;
        }
        return FkBridge.newGraphics(this);
    }

    public int getWidth() {
        checkDisposed();
        return img.getWidth();
    }

    public int getHeight() {
        checkDisposed();
        return img.getHeight();
    }

    public void dispose() {
        disposed = true;
        derived = null;
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

    public void setAlpha(int a) {
        if (a < 0) {
            a = 0;
        } else if (a > 255) {
            a = 255;
        }
        alpha = a;
    }

    public int getAlpha() {
        return alpha;
    }

    /* ------------------------------------------------------------------ */

    void checkDisposed() {
        if (disposed || img == null) {
            throw new UIException(UIException.ILLEGAL_STATE, "image disposed");
        }
    }

    /** The MIDP image holding the pixels. */
    javax.microedition.lcdui.Image raw() {
        checkDisposed();
        return img;
    }

    /** The MIDP image to draw: transparency color and alpha applied. */
    javax.microedition.lcdui.Image drawable() {
        checkDisposed();
        if (!transparentEnabled && alpha >= 255) {
            return img;
        }
        if (derived != null && derivedVersion == version && derivedAlpha == alpha
                && derivedKeyEnabled == transparentEnabled
                && (!transparentEnabled || derivedKeyColor == transparentColor)) {
            return derived;
        }
        int w = img.getWidth();
        int h = img.getHeight();
        int[] px = new int[w * h];
        img.getRGB(px, 0, w, 0, 0, w, h);
        int key = transparentColor & 0xffffff;
        boolean keyOn = transparentEnabled;
        int a = alpha;
        for (int i = 0; i < px.length; i++) {
            int p = px[i];
            if (keyOn && (p & 0xffffff) == key) {
                px[i] = 0;
            } else if (a < 255) {
                int pa = (p >>> 24) * a / 255;
                px[i] = (pa << 24) | (p & 0xffffff);
            }
        }
        derived = javax.microedition.lcdui.Image.createRGBImage(px, w, h, true);
        derivedVersion = version;
        derivedAlpha = a;
        derivedKeyEnabled = keyOn;
        derivedKeyColor = transparentColor;
        return derived;
    }

    void touched() {
        version++;
    }
}

final class FkImage extends Image {
    FkImage(javax.microedition.lcdui.Image image, boolean isMutable) {
        super(image, isMutable);
    }

    public void dispose() {
        super.dispose();
    }
}
