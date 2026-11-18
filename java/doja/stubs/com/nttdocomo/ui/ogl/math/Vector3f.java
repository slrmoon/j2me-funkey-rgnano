package com.nttdocomo.ui.ogl.math;

public final class Vector3f extends Tuple3f {
    public Vector3f() {
    }

    public Vector3f(float x, float y, float z) {
        super(x, y, z);
    }

    public void normalize() {
        float l = (float) Math.sqrt(x * x + y * y + z * z);
        if (l > 0f) {
            x /= l;
            y /= l;
            z /= l;
        }
    }

    public float dot(Vector3f v) {
        return x * v.x + y * v.y + z * v.z;
    }

    /** this = left x right */
    public void cross(Vector3f l, Vector3f r) {
        float cx = l.y * r.z - l.z * r.y;
        float cy = l.z * r.x - l.x * r.z;
        float cz = l.x * r.y - l.y * r.x;
        x = cx;
        y = cy;
        z = cz;
    }
}
