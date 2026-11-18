package com.nttdocomo.ui.ogl.math;

public abstract class Tuple3f {
    public float x;
    public float y;
    public float z;

    public Tuple3f() {
    }

    public Tuple3f(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public void add(Tuple3f t) {
        x += t.x;
        y += t.y;
        z += t.z;
    }
}
