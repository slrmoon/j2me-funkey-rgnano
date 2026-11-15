package com.nttdocomo.ui.util3d;

public class Vector3D {
    private float x;
    private float y;
    private float z;

    public Vector3D() {
    }

    public Vector3D(Vector3D v) {
        set(v);
    }

    public Vector3D(float x, float y, float z) {
        set(x, y, z);
    }

    public void set(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public void set(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        x = v.x;
        y = v.y;
        z = v.z;
    }

    public void add(float x, float y, float z) {
        this.x += x;
        this.y += y;
        this.z += z;
    }

    public void add(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        add(v.x, v.y, v.z);
    }

    public void setX(float x) {
        this.x = x;
    }

    public void setY(float y) {
        this.y = y;
    }

    public void setZ(float z) {
        this.z = z;
    }

    public float getX() {
        return x;
    }

    public float getY() {
        return y;
    }

    public float getZ() {
        return z;
    }

    public void normalize() {
        double len = java.lang.Math.sqrt((double) x * x + (double) y * y + (double) z * z);
        if (len == 0) {
            throw new ArithmeticException("zero vector");
        }
        x = (float) (x / len);
        y = (float) (y / len);
        z = (float) (z / len);
    }

    public float dot(Vector3D v) {
        return dot(this, v);
    }

    public static float dot(Vector3D v1, Vector3D v2) {
        if (v1 == null || v2 == null) {
            throw new NullPointerException();
        }
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    /** this = this x v */
    public void cross(Vector3D v) {
        cross(this, v);
    }

    /** this = u x v */
    public void cross(Vector3D u, Vector3D v) {
        if (u == null || v == null) {
            throw new NullPointerException();
        }
        float cx = u.y * v.z - u.z * v.y;
        float cy = u.z * v.x - u.x * v.z;
        float cz = u.x * v.y - u.y * v.x;
        x = cx;
        y = cy;
        z = cz;
    }
}
