package com.mascotcapsule.micro3d.v3;

import java.io.IOException;

import com.funkey.mc3.Model;
import com.funkey.mc3.Pose;
import com.funkey.mc3.TexData;

/** A model (MBAC) with its posture, textures and pattern. */
public class Figure {
    Pose pose;
    private Texture[] textures;
    private int selected;

    public Figure(byte[] b) {
        if (b == null) {
            throw new NullPointerException();
        }
        init(b);
    }

    public Figure(String name) throws IOException {
        init(Res.load(name));
    }

    private void init(byte[] b) {
        try {
            pose = new Pose(Model.load(b));
        } catch (IOException e) {
            throw Res.bad(e);
        }
    }

    public final void dispose() {
        pose = null;
        textures = null;
    }

    public final void setPosture(ActionTable act, int action, int frame) {
        if (act == null) {
            throw new NullPointerException();
        }
        if (act.data == null || action < 0 || action >= act.data.numActions) {
            throw new IllegalArgumentException("action");
        }
        if (pose == null) {
            return;
        }
        pose.pose(act.data, action, frame);
        pose.pattern = act.data.patternAt(action, frame, pose.pattern);
    }

    public final Texture getTexture() {
        if (textures == null || textures.length == 0) {
            return null;
        }
        return textures[selected];
    }

    public final void setTexture(Texture t) {
        if (t == null) {
            throw new NullPointerException();
        }
        if (!t.isForModel()) {
            throw new IllegalArgumentException("not a model texture");
        }
        textures = new Texture[] { t };
        selected = 0;
        apply();
    }

    public final void setTexture(Texture[] t) {
        if (t == null) {
            throw new NullPointerException();
        }
        for (int i = 0; i < t.length; i++) {
            if (t[i] == null) {
                throw new NullPointerException();
            }
            if (!t[i].isForModel()) {
                throw new IllegalArgumentException("not a model texture");
            }
        }
        textures = t;
        selected = 0;
        apply();
    }

    public final int getNumTextures() {
        return textures == null ? 0 : textures.length;
    }

    public final void selectTexture(int idx) {
        if (textures == null || idx < 0 || idx >= textures.length) {
            throw new IllegalArgumentException("idx");
        }
        selected = idx;
        apply();
    }

    public final int getNumPattern() {
        return pose == null ? 0 : pose.model.numPatterns;
    }

    public final void setPattern(int idx) {
        if (pose != null) {
            pose.pattern = idx;
        }
    }

    /**
     * Engine textures: a model made for several textures takes them by
     * its polygons' texture numbers, any other the selected one.
     */
    void apply() {
        if (pose == null) {
            return;
        }
        Texture[] t = textures;
        TexData[] d = new TexData[t == null ? 0 : t.length];
        for (int i = 0; i < d.length; i++) {
            d[i] = t[i].data;
        }
        pose.textures = d;
        boolean multi = pose.model.numTextures > 1 && d.length >= pose.model.numTextures;
        pose.selected = multi ? -1 : selected;
    }
}
