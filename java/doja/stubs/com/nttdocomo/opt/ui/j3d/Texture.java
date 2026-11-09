package com.nttdocomo.opt.ui.j3d;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import com.funkey.doja.IndexedImage;

/**
 * MascotCapsule texture: an 8-bit BMP (GIF works too). Palette index 0
 * is the color key. A texture not made for models is a sphere-map
 * (environment) texture.
 */
public class Texture {
    int width;
    int height;
    byte[] pixels;
    int[] palette;
    boolean forModel;
    boolean toon;
    int toonHighlight = 128;
    int toonMid = 255;
    int toonShadow = 96;
    /** M3G texture, made on first use */
    Object m3g;

    protected Texture() {
    }

    public Texture(byte[] data, boolean isForModel) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        load(data, isForModel);
    }

    public Texture(InputStream in, boolean isForModel) throws IOException {
        if (in == null) {
            throw new NullPointerException("inputStream");
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[2048];
        int n;
        while ((n = in.read(buf)) > 0) {
            out.write(buf, 0, n);
        }
        load(out.toByteArray(), isForModel);
    }

    private void load(byte[] data, boolean isForModel) {
        IndexedImage ix;
        try {
            ix = IndexedImage.decode(data);
        } catch (IOException e) {
            throw new IllegalArgumentException("texture: " + e.getMessage());
        }
        width = ix.width;
        height = ix.height;
        pixels = ix.pixels;
        palette = ix.palette;
        forModel = isForModel;
    }

    public void setNormalShader() {
        toon = false;
    }

    public void setToonShader(int highlight, int mid, int shadow) {
        if (highlight < 0 || highlight > 255 || mid < 0 || mid > 255 || shadow < 0 || shadow > 255) {
            throw new IllegalArgumentException("toon");
        }
        toon = true;
        toonHighlight = highlight;
        toonMid = mid;
        toonShadow = shadow;
    }
}
