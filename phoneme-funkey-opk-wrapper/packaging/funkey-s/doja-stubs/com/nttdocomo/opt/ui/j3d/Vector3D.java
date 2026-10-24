package com.nttdocomo.opt.ui.j3d;

/** Integer vector; normalize() scales to length 4096. */
public class Vector3D {
    public int x;
    public int y;
    public int z;

    public Vector3D() {
    }

    public Vector3D(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public void normalize() {
        long sq = (long) x * x + (long) y * y + (long) z * z;
        long len = (long) (java.lang.Math.sqrt((double) sq) + 0.5);
        if (len == 0) {
            return;     /* games pass zero vectors; leave them as they are */
        }
        x = (int) (((long) x << 12) / len);
        y = (int) (((long) y << 12) / len);
        z = (int) (((long) z << 12) / len);
    }

    public int dot(Vector3D v) {
        return dot(this, v);
    }

    public static int dot(Vector3D v1, Vector3D v2) {
        if (v1 == null || v2 == null) {
            throw new NullPointerException();
        }
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    public void cross(Vector3D v) {
        cross(this, v);
    }

    public void cross(Vector3D u, Vector3D v) {
        if (u == null || v == null) {
            throw new NullPointerException();
        }
        int nx = u.y * v.z - u.z * v.y;
        int ny = u.z * v.x - u.x * v.z;
        int nz = u.x * v.y - u.y * v.x;
        x = nx;
        y = ny;
        z = nz;
    }
}
