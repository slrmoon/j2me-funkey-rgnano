package com.nttdocomo.ui.ogl.math;

public abstract class Tuple4f {
    public float x;
    public float y;
    public float z;
    public float w;

    public Tuple4f() {
    }

    public Tuple4f(float x, float y, float z, float w) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }

    public void add(Tuple4f t) {
        x += t.x;
        y += t.y;
        z += t.z;
        w += t.w;
    }

    /** this = matrix * this */
    public void mul(Matrix4f matrix) {
        matrix.transform(this);
    }
}
