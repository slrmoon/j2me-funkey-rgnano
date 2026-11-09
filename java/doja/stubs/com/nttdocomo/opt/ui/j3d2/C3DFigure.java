package com.nttdocomo.opt.ui.j3d2;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * C3D figure. The format of these models is not known here, so a figure
 * keeps its data and draws nothing (games that use it still run).
 */
public final class C3DFigure {
    byte[] data;
    C3DTexture texture;

    public C3DFigure(byte[] source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = source;
    }

    public C3DFigure(InputStream source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = readAll(source);
    }

    static byte[] readAll(InputStream in) {
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] b = new byte[2048];
            int n;
            while ((n = in.read(b)) > 0) {
                out.write(b, 0, n);
            }
            return out.toByteArray();
        } catch (IOException e) {
            return new byte[0];
        }
    }

    public void setTexture(C3DTexture t) {
        texture = t;
    }

    public void setPostureByTime(C3DActionTable actionTable, int action, int timeMillis) {
    }

    public int getAnimationTime() {
        return 0;
    }

    void render(C3DGraphics g) {
        com.funkey.doja.Stub.log("C3DFigure.render");
    }

    public void dispose() {
        data = null;
    }

    protected C3DFigure() {
    }
}
