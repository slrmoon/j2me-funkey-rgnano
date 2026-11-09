package com.nttdocomo.opt.ui.j3d;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/** Bone animations of a figure (MTRA file). Frames are 16.16 fixed point. */
public class ActionTable {
    FkAction data;

    protected ActionTable() {
    }

    public ActionTable(byte[] bytes) {
        if (bytes == null) {
            throw new NullPointerException("data");
        }
        try {
            data = FkAction.load(bytes);
        } catch (IOException e) {
            throw new IllegalArgumentException(e.getMessage());
        }
    }

    public ActionTable(InputStream in) throws IOException {
        this(Figure.readAll(in));
    }

    public int getNumAction() {
        return data.numActions;
    }

    public int getMaxFrame(int action) {
        if (action < 0 || action >= data.numActions) {
            throw new IllegalArgumentException("action");
        }
        return data.maxFrame[action] << 16;
    }
}
