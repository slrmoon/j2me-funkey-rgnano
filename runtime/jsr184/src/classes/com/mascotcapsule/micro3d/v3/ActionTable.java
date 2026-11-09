package com.mascotcapsule.micro3d.v3;

import java.io.IOException;

import com.funkey.mc3.Action;

/** Bone animations of a figure (MTRA). Frames are 16.16 fixed point. */
public class ActionTable {
    Action data;

    public ActionTable(byte[] b) {
        if (b == null) {
            throw new NullPointerException();
        }
        try {
            data = Action.load(b);
        } catch (IOException e) {
            throw Res.bad(e);
        }
    }

    public ActionTable(String name) throws IOException {
        byte[] b = Res.load(name);
        try {
            data = Action.load(b);
        } catch (IOException e) {
            throw Res.bad(e);
        }
    }

    public final void dispose() {
        data = null;
    }

    public final int getNumActions() {
        return data == null ? 0 : data.numActions;
    }

    /** @deprecated */
    public final int getNumAction() {
        return getNumActions();
    }

    /** frames of the action, 16.16 fixed point (as setPosture takes them) */
    public final int getNumFrames(int idx) {
        if (data == null || idx < 0 || idx >= data.numActions) {
            throw new IllegalArgumentException("idx");
        }
        return data.maxFrame[idx] << 16;
    }

    /** @deprecated */
    public final int getNumFrame(int idx) {
        return getNumFrames(idx);
    }
}
