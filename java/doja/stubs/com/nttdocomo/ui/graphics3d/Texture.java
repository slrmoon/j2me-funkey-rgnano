package com.nttdocomo.ui.graphics3d;

import com.funkey.doja.IndexedImage;

/** A texture from a BMP with a palette; palette entry 0 is the colour key. */
public class Texture extends Object3D {
    public static final int MODULATE = 0;
    public static final int REPLACE = 1;

    int fkWidth;
    int fkHeight;
    /** r, g, b, a per pixel; a = 0 for palette entry 0 */
    byte[] fkRgba;
    int fkBlend = MODULATE;
    /* the GL texture made from it, and the GL it lives in */
    Object fkGl;
    int fkId;

    Texture(byte[] data) {
        super(TYPE_TEXTURE);
        IndexedImage ix;
        try {
            ix = IndexedImage.decode(data);
        } catch (java.io.IOException e) {
            throw new IllegalArgumentException("texture: " + e.getMessage());
        }
        fkWidth = ix.width;
        fkHeight = ix.height;
        int n = fkWidth * fkHeight;
        fkRgba = new byte[n * 4];
        for (int i = 0; i < n; i++) {
            int idx = ix.pixels[i] & 255;
            int c = idx < ix.palette.length ? ix.palette[idx] : 0;
            fkRgba[i * 4] = (byte) (c >> 16);
            fkRgba[i * 4 + 1] = (byte) (c >> 8);
            fkRgba[i * 4 + 2] = (byte) c;
            fkRgba[i * 4 + 3] = (byte) (idx == 0 ? 0 : 255);
        }
    }

    protected Texture() {
        super(TYPE_TEXTURE);
    }

    Texture(int width, int height, byte[] rgba) {
        super(TYPE_TEXTURE);
        fkWidth = width;
        fkHeight = height;
        fkRgba = rgba;
    }

    public void setBlendMode(int blendMode) {
        fkBlend = blendMode;
    }

    public void setEnvironmentMapEnabled(boolean enabled) {
    }

    public void setEnvironmentMapTexture(Texture texture) {
    }
}
