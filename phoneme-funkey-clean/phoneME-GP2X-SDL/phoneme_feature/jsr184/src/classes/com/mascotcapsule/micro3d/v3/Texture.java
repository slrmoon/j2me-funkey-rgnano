package com.mascotcapsule.micro3d.v3;

import java.io.IOException;

import com.funkey.mc3.TexData;

/**
 * Model or environment (sphere map) texture from an 8-bit BMP. Palette
 * index 0 is the color key.
 */
public class Texture {
    TexData data;

    public Texture(byte[] b, boolean isForModel) {
        if (b == null) {
            throw new NullPointerException();
        }
        try {
            data = TexData.load(b, isForModel);
        } catch (IOException e) {
            throw Res.bad(e);
        }
    }

    public Texture(String name, boolean isForModel) throws IOException {
        byte[] b = Res.load(name);
        try {
            data = TexData.load(b, isForModel);
        } catch (IOException e) {
            throw Res.bad(e);
        }
    }

    boolean isForModel() {
        return data != null && data.forModel;
    }

    public final void dispose() {
        data = null;
    }
}
