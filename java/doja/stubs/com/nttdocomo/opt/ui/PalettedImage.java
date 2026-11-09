package com.nttdocomo.opt.ui;

import java.io.IOException;

import com.funkey.doja.IndexedImage;
import com.nttdocomo.lang.UnsupportedOperationException;
import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.Graphics;
import com.nttdocomo.ui.Image;
import com.nttdocomo.ui.UIException;

/** opt.ui paletted image: the palette is shared with the caller. */
public class PalettedImage extends Image {
    private int width;
    private int height;
    private byte[] pixels;
    private Palette palette;
    private int transparentIndex = -1;
    private Palette shownPalette;
    private int shownVersion = -1;
    private boolean dirty = true;

    protected PalettedImage() {
    }

    public static PalettedImage createPalettedImage(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        IndexedImage ix;
        try {
            ix = IndexedImage.decode(data);
        } catch (IOException e) {
            throw new UIException(UIException.UNSUPPORTED_FORMAT, e.getMessage());
        }
        PalettedImage p = new PalettedImage();
        p.width = ix.width;
        p.height = ix.height;
        p.pixels = ix.pixels;
        int[] rgb = new int[ix.palette.length];
        for (int i = 0; i < rgb.length; i++) {
            rgb[i] = ix.palette[i] & 0xffffff;
        }
        p.palette = new Palette(rgb);
        p.transparentIndex = ix.transparentIndex;
        return p;
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
        if (FkBridge.isDisposed(this) || pixels == null || palette == null) {
            return;
        }
        if (!dirty && shownPalette == palette && shownVersion == palette.version) {
            return;
        }
        int[] argb = IndexedImage.render(pixels, palette.argb(), transparentIndex);
        FkBridge.setPixels(this, javax.microedition.lcdui.Image.createRGBImage(
                argb, width, height, transparentIndex >= 0));
        shownPalette = palette;
        shownVersion = palette.version;
        dirty = false;
    }
}
