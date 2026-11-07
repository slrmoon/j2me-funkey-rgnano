package com.mascotcapsule.micro3d.v3;

/** 3D vector; 4096 = 1.0. */
public class Vector3D {
    public int x;
    public int y;
    public int z;

    public Vector3D() {
    }

    public Vector3D(Vector3D v) {
        set(v);
    }

    public Vector3D(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public final int getX() {
        return x;
    }

    public final int getY() {
        return y;
    }

    public final int getZ() {
        return z;
    }

    public final void setX(int x) {
        this.x = x;
    }

    public final void setY(int y) {
        this.y = y;
    }

    public final void setZ(int z) {
        this.z = z;
    }

    public final void set(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public final void set(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        x = v.x;
        y = v.y;
        z = v.z;
    }

    /** scales to length 4096; a zero vector stays as it is */
    public final void unit() {
        long sq = (long) x * x + (long) y * y + (long) z * z;
        long len = (long) (Math.sqrt((double) sq) + 0.5);
        if (len == 0) {
            return;
        }
        x = (int) (((long) x << 12) / len);
        y = (int) (((long) y << 12) / len);
        z = (int) (((long) z << 12) / len);
    }

    public final int innerProduct(Vector3D v) {
        return innerProduct(this, v);
    }

    public static final int innerProduct(Vector3D v1, Vector3D v2) {
        if (v1 == null || v2 == null) {
            throw new NullPointerException();
        }
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    /** this = this x v */
    public final void outerProduct(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        int nx = y * v.z - z * v.y;
        int ny = z * v.x - x * v.z;
        int nz = x * v.y - y * v.x;
        x = nx;
        y = ny;
        z = nz;
    }

    public static final Vector3D outerProduct(Vector3D v1, Vector3D v2) {
        if (v1 == null || v2 == null) {
            throw new NullPointerException();
        }
        Vector3D r = new Vector3D(v1);
        r.outerProduct(v2);
        return r;
    }
}
