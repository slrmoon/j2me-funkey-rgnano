package com.nttdocomo.ui;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import com.funkey.doja.IndexedImage;
import com.nttdocomo.lang.UnsupportedOperationException;

/**
 * Image of palette indices; drawing uses the current palette, so changing
 * an entry recolors the image (fades, color cycling).
 */
public class PalettedImage extends Image {
    private int width;
    private int height;
    private byte[] pixels;
    private Palette palette;
    private int transparentIndex = -1;
    private boolean indexTransparent;
    private Palette shownPalette;
    private int shownVersion = -1;
    private boolean dirty = true;

    protected PalettedImage() {
    }

    public static PalettedImage createPalettedImage(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        PalettedImage p = new PalettedImage();
        p.load(data);
        return p;
    }

    public static PalettedImage createPalettedImage(InputStream in) throws IOException {
        return createPalettedImage(readAll(in));
    }

    public static PalettedImage createPalettedImage(int width, int height) {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("size");
        }
        PalettedImage p = new PalettedImage();
        p.width = width;
        p.height = height;
        p.pixels = new byte[width * height];
        p.palette = new Palette(256);
        return p;
    }

    private void load(byte[] data) {
        IndexedImage ix;
        try {
            ix = IndexedImage.decode(data);
        } catch (IOException e) {
            throw new UIException(UIException.UNSUPPORTED_FORMAT, e.getMessage());
        }
        width = ix.width;
        height = ix.height;
        pixels = ix.pixels;
        int[] rgb = new int[ix.palette.length];
        for (int i = 0; i < rgb.length; i++) {
            rgb[i] = ix.palette[i] & 0xffffff;
        }
        palette = new Palette(rgb);
        transparentIndex = ix.transparentIndex;
        indexTransparent = transparentIndex >= 0;
        disposed = false;
        dirty = true;
    }

    static byte[] readAll(InputStream in) throws IOException {
        if (in == null) {
            throw new NullPointerException("inputStream");
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[2048];
        int n;
        while ((n = in.read(buf)) > 0) {
            out.write(buf, 0, n);
        }
        return out.toByteArray();
    }

    public void changeData(InputStream in) {
        try {
            changeData(readAll(in));
        } catch (IOException e) {
            throw new UIException(UIException.UNSUPPORTED_FORMAT, e.getMessage());
        }
    }

    public void changeData(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        load(data);
    }

    public void setPalette(Palette p) {
        if (p == null) {
            throw new NullPointerException("palette");
        }
        palette = p;
        dirty = true;
    }

    public Palette getPalette() {
        return palette;
    }

    public final Graphics getGraphics() {
        throw new UnsupportedOperationException("PalettedImage");
    }

    public final void setTransparentEnabled(boolean enabled) {
        indexTransparent = enabled;
        if (enabled && transparentIndex < 0) {
            transparentIndex = 0;
        }
        dirty = true;
    }

    public void setTransparentIndex(int index) {
        if (index < 0 || index > 255) {
            throw new ArrayIndexOutOfBoundsException(index);
        }
        transparentIndex = index;
        dirty = true;
    }

    public int getTransparentIndex() {
        return transparentIndex < 0 ? 0 : transparentIndex;
    }

    public void setAlpha(int a) {
        super.setAlpha(a);
    }

    public int getAlpha() {
        return super.getAlpha();
    }

    public final int getTransparentColor() {
        throw new UnsupportedOperationException("PalettedImage");
    }

    public final void setTransparentColor(int color) {
        throw new UnsupportedOperationException("PalettedImage");
    }

    public void dispose() {
        super.dispose();
        pixels = null;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    protected void fkPrepare() {
        if (disposed || pixels == null || palette == null) {
            return;
        }
        if (!dirty && shownPalette == palette && shownVersion == palette.version && img != null) {
            return;
        }
        int[] argb = IndexedImage.render(pixels, palette.argb(),
                indexTransparent ? transparentIndex : -1);
        FkBridge.setPixels(this, javax.microedition.lcdui.Image.createRGBImage(
                argb, width, height, indexTransparent));
        shownPalette = palette;
        shownVersion = palette.version;
        dirty = false;
    }
}
