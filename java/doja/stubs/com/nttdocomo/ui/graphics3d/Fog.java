package com.nttdocomo.ui.graphics3d;

public class Fog extends Object3D {
    public static final int EXPONENTIAL = 80;
    public static final int LINEAR = 81;

    int fkMode = LINEAR;
    float fkNear;
    float fkFar = 1f;
    float fkDensity = 1f;
    int fkColor = 0xffffff;

    public Fog() {
        super(TYPE_FOG);
    }

    public void setMode(int mode) {
        if (mode != EXPONENTIAL && mode != LINEAR) {
            throw new IllegalArgumentException("mode");
        }
        fkMode = mode;
    }

    public void setLinear(float near, float far) {
        fkNear = near;
        fkFar = far;
    }

    public void setDensity(float density) {
        fkDensity = density;
    }

    public void setColor(int color) {
        fkColor = color;
    }
}
