package com.nttdocomo.opt.ui.j3d2;

import java.io.IOException;
import java.io.InputStream;

/** Texture resource for C3D figures (BMP data kept for the figure renderer). */
public final class C3DTexture {
    byte[] data;

    public C3DTexture(byte[] source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = source;
    }

    public C3DTexture(InputStream source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = C3DFigure.readAll(source);
    }

    public void dispose() {
        data = null;
    }

    protected C3DTexture() {
    }
}
