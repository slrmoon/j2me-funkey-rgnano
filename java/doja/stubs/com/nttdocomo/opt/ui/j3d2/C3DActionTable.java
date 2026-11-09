package com.nttdocomo.opt.ui.j3d2;

import java.io.InputStream;

/** Action data for C3D figures. Only the header is used. */
public final class C3DActionTable {
    byte[] data;

    public C3DActionTable(byte[] source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = source;
    }

    public C3DActionTable(InputStream source) {
        if (source == null) {
            throw new NullPointerException();
        }
        data = C3DFigure.readAll(source);
    }

    public int getNumAction() {
        return data == null ? 0 : 1;
    }

    public int getMaxTime(int action) {
        return 0;
    }

    public void dispose() {
        data = null;
    }

    protected C3DActionTable() {
    }
}
