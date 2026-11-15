package com.nttdocomo.ui.graphics3d;

import com.nttdocomo.ui.util3d.Vector3D;

public class Light extends Object3D {
    public static final int AMBIENT = 128;
    public static final int DIRECTIONAL = 129;
    public static final int OMNI = 130;
    public static final int SPOT = 131;

    private Vector3D position = new Vector3D();
    private Vector3D vector = new Vector3D(0f, 0f, 1f);
    int fkMode = AMBIENT;
    float fkIntensity = 1f;
    int fkColor = 0xffffff;

    public Light() {
        super(TYPE_LIGHT);
    }

    public void setPosition(Vector3D position) {
        this.position = position == null ? new Vector3D() : new Vector3D(position);
    }

    public Vector3D getPosition() {
        return new Vector3D(position);
    }

    public void setVector(Vector3D vector) {
        this.vector = vector == null ? new Vector3D(0f, 0f, 1f) : new Vector3D(vector);
    }

    public Vector3D getVector() {
        return new Vector3D(vector);
    }

    public void setMode(int mode) {
        if (mode < AMBIENT || mode > SPOT) {
            throw new IllegalArgumentException("mode");
        }
        fkMode = mode;
    }

    public void setIntensity(float intensity) {
        fkIntensity = intensity;
    }

    public void setColor(int color) {
        fkColor = color;
    }

    public void setSpotAngle(float spotAngle) {
    }

    public void setSpotExponent(float spotExponent) {
    }

    public void setAttenuation(float constant, float linear, float quadratic) {
    }

    public static int getMaxLights() {
        return 8;
    }
}
