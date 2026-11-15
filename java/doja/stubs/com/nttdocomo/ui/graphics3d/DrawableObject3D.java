package com.nttdocomo.ui.graphics3d;

import com.nttdocomo.ui.util3d.Transform;

public abstract class DrawableObject3D extends Object3D {
    public static final int BLEND_NORMAL = 0;
    public static final int BLEND_ALPHA = 32;
    public static final int BLEND_ADD = 64;

    protected DrawableObject3D(int type) {
        super(type);
    }

    protected DrawableObject3D() {
    }

    public boolean isCross(DrawableObject3D other, Transform thisTransform, Transform otherTransform) {
        return false;
    }

    public abstract void setPerspectiveCorrectionEnabled(boolean enabled);

    public abstract void setBlendMode(int blendMode);

    public abstract void setTransparency(float transparency);
}
