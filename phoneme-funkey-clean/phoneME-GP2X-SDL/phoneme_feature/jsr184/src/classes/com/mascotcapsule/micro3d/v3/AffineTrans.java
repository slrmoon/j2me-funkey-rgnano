package com.mascotcapsule.micro3d.v3;

import com.funkey.mc3.Fx;

/**
 * 3x4 affine matrix: rotation/scale part 4096 = 1.0, translation in
 * model units. Angles: 4096 = 360 degrees.
 */
public class AffineTrans {
    public int m00;
    public int m01;
    public int m02;
    public int m03;
    public int m10;
    public int m11;
    public int m12;
    public int m13;
    public int m20;
    public int m21;
    public int m22;
    public int m23;

    /** all elements 0 */
    public AffineTrans() {
    }

    public AffineTrans(int m00, int m01, int m02, int m03, int m10, int m11, int m12, int m13,
            int m20, int m21, int m22, int m23) {
        set(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23);
    }

    public AffineTrans(AffineTrans a) {
        set(a);
    }

    public AffineTrans(int[][] a) {
        set(a);
    }

    public AffineTrans(int[] a) {
        set(a, 0);
    }

    public AffineTrans(int[] a, int offset) {
        set(a, offset);
    }

    private static void check(int[] a, int offset) {
        if (a == null) {
            throw new NullPointerException();
        }
        if (offset < 0 || a.length - offset < 12) {
            throw new IllegalArgumentException();
        }
    }

    public final void get(int[] a) {
        get(a, 0);
    }

    public final void get(int[] a, int offset) {
        check(a, offset);
        a[offset] = m00; a[offset + 1] = m01; a[offset + 2] = m02; a[offset + 3] = m03;
        a[offset + 4] = m10; a[offset + 5] = m11; a[offset + 6] = m12; a[offset + 7] = m13;
        a[offset + 8] = m20; a[offset + 9] = m21; a[offset + 10] = m22; a[offset + 11] = m23;
    }

    public final void set(int[] a) {
        set(a, 0);
    }

    public final void set(int[] a, int offset) {
        check(a, offset);
        set(a[offset], a[offset + 1], a[offset + 2], a[offset + 3],
                a[offset + 4], a[offset + 5], a[offset + 6], a[offset + 7],
                a[offset + 8], a[offset + 9], a[offset + 10], a[offset + 11]);
    }

    /** a[row][column], 3 rows of 4 */
    public final void set(int[][] a) {
        if (a == null) {
            throw new NullPointerException();
        }
        if (a.length < 3) {
            throw new IllegalArgumentException();
        }
        for (int r = 0; r < 3; r++) {
            if (a[r] == null) {
                throw new NullPointerException();
            }
            if (a[r].length < 4) {
                throw new IllegalArgumentException();
            }
        }
        set(a[0][0], a[0][1], a[0][2], a[0][3], a[1][0], a[1][1], a[1][2], a[1][3],
                a[2][0], a[2][1], a[2][2], a[2][3]);
    }

    public final void set(int m00, int m01, int m02, int m03, int m10, int m11, int m12, int m13,
            int m20, int m21, int m22, int m23) {
        this.m00 = m00; this.m01 = m01; this.m02 = m02; this.m03 = m03;
        this.m10 = m10; this.m11 = m11; this.m12 = m12; this.m13 = m13;
        this.m20 = m20; this.m21 = m21; this.m22 = m22; this.m23 = m23;
    }

    public final void set(AffineTrans a) {
        if (a == null) {
            throw new NullPointerException();
        }
        set(a.m00, a.m01, a.m02, a.m03, a.m10, a.m11, a.m12, a.m13, a.m20, a.m21, a.m22, a.m23);
    }

    public final void setIdentity() {
        set(4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0);
    }

    /* the rotations keep the fourth column */
    public final void rotationX(int r) {
        int c = Fx.cos(r);
        int s = Fx.sin(r);
        m00 = 4096; m01 = 0; m02 = 0;
        m10 = 0; m11 = c; m12 = -s;
        m20 = 0; m21 = s; m22 = c;
    }

    public final void rotationY(int r) {
        int c = Fx.cos(r);
        int s = Fx.sin(r);
        m00 = c; m01 = 0; m02 = s;
        m10 = 0; m11 = 4096; m12 = 0;
        m20 = -s; m21 = 0; m22 = c;
    }

    public final void rotationZ(int r) {
        int c = Fx.cos(r);
        int s = Fx.sin(r);
        m00 = c; m01 = -s; m02 = 0;
        m10 = s; m11 = c; m12 = 0;
        m20 = 0; m21 = 0; m22 = 4096;
    }

    /** @deprecated */
    public final void setRotationX(int r) {
        rotationX(r);
    }

    /** @deprecated */
    public final void setRotationY(int r) {
        rotationY(r);
    }

    /** @deprecated */
    public final void setRotationZ(int r) {
        rotationZ(r);
    }

    /** rotation by r about the axis v (normalized here) */
    public final void setRotation(Vector3D v, int r) {
        if (v == null) {
            throw new NullPointerException();
        }
        Vector3D n = new Vector3D(v);
        if (n.x == 0 && n.y == 0 && n.z == 0) {
            m00 = 4096; m01 = 0; m02 = 0;
            m10 = 0; m11 = 4096; m12 = 0;
            m20 = 0; m21 = 0; m22 = 4096;
            return;
        }
        n.unit();
        int c = Fx.cos(r);
        int s = Fx.sin(r);
        int t = 4096 - c;
        int x = n.x;
        int y = n.y;
        int z = n.z;
        m00 = c + Fx.mul(Fx.mul(x, x), t);
        m01 = Fx.mul(Fx.mul(x, y), t) - Fx.mul(z, s);
        m02 = Fx.mul(Fx.mul(x, z), t) + Fx.mul(y, s);
        m10 = Fx.mul(Fx.mul(y, x), t) + Fx.mul(z, s);
        m11 = c + Fx.mul(Fx.mul(y, y), t);
        m12 = Fx.mul(Fx.mul(y, z), t) - Fx.mul(x, s);
        m20 = Fx.mul(Fx.mul(z, x), t) - Fx.mul(y, s);
        m21 = Fx.mul(Fx.mul(z, y), t) + Fx.mul(x, s);
        m22 = c + Fx.mul(Fx.mul(z, z), t);
    }

    /** @deprecated */
    public final void rotationV(Vector3D v, int r) {
        setRotation(v, r);
    }

    /** this = this x a */
    public final void mul(AffineTrans a) {
        mul(this, a);
    }

    /** this = a1 x a2 */
    public final void mul(AffineTrans a1, AffineTrans a2) {
        if (a1 == null || a2 == null) {
            throw new NullPointerException();
        }
        int a00 = Fx.mul(a1.m00, a2.m00) + Fx.mul(a1.m01, a2.m10) + Fx.mul(a1.m02, a2.m20);
        int a01 = Fx.mul(a1.m00, a2.m01) + Fx.mul(a1.m01, a2.m11) + Fx.mul(a1.m02, a2.m21);
        int a02 = Fx.mul(a1.m00, a2.m02) + Fx.mul(a1.m01, a2.m12) + Fx.mul(a1.m02, a2.m22);
        int a03 = Fx.mul(a1.m00, a2.m03) + Fx.mul(a1.m01, a2.m13) + Fx.mul(a1.m02, a2.m23) + a1.m03;
        int a10 = Fx.mul(a1.m10, a2.m00) + Fx.mul(a1.m11, a2.m10) + Fx.mul(a1.m12, a2.m20);
        int a11 = Fx.mul(a1.m10, a2.m01) + Fx.mul(a1.m11, a2.m11) + Fx.mul(a1.m12, a2.m21);
        int a12 = Fx.mul(a1.m10, a2.m02) + Fx.mul(a1.m11, a2.m12) + Fx.mul(a1.m12, a2.m22);
        int a13 = Fx.mul(a1.m10, a2.m03) + Fx.mul(a1.m11, a2.m13) + Fx.mul(a1.m12, a2.m23) + a1.m13;
        int a20 = Fx.mul(a1.m20, a2.m00) + Fx.mul(a1.m21, a2.m10) + Fx.mul(a1.m22, a2.m20);
        int a21 = Fx.mul(a1.m20, a2.m01) + Fx.mul(a1.m21, a2.m11) + Fx.mul(a1.m22, a2.m21);
        int a22 = Fx.mul(a1.m20, a2.m02) + Fx.mul(a1.m21, a2.m12) + Fx.mul(a1.m22, a2.m22);
        int a23 = Fx.mul(a1.m20, a2.m03) + Fx.mul(a1.m21, a2.m13) + Fx.mul(a1.m22, a2.m23) + a1.m23;
        set(a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23);
    }

    /** @deprecated */
    public final void multiply(AffineTrans a) {
        mul(a);
    }

    /** @deprecated */
    public final void multiply(AffineTrans a1, AffineTrans a2) {
        mul(a1, a2);
    }

    /**
     * View transformation for a camera at pos looking at the point look:
     * right-handed, the line of sight is +z (rows: x right, y down, z
     * forward). The up vector is corrected when not perpendicular.
     */
    public final void lookAt(Vector3D pos, Vector3D look, Vector3D up) {
        if (pos == null || look == null || up == null) {
            throw new NullPointerException();
        }
        Vector3D f = new Vector3D(look.x - pos.x, look.y - pos.y, look.z - pos.z);
        if (f.x == 0 && f.y == 0 && f.z == 0) {
            f.z = 4096;
        }
        f.unit();
        Vector3D u = new Vector3D(up);
        if (u.x == 0 && u.y == 0 && u.z == 0) {
            u.y = 4096;
        }
        u.unit();
        Vector3D s = cross(f, u);
        if (s.x == 0 && s.y == 0 && s.z == 0) {
            /* up parallel to the line of sight: pick another up */
            u.x = 0;
            u.y = f.z != 0 || f.x != 0 ? 4096 : 0;
            u.z = u.y == 0 ? 4096 : 0;
            s = cross(f, u);
            if (s.x == 0 && s.y == 0 && s.z == 0) {
                s.x = 4096;
            }
        }
        s.unit();
        Vector3D v = cross(f, s);
        v.unit();
        m00 = s.x; m01 = s.y; m02 = s.z;
        m10 = v.x; m11 = v.y; m12 = v.z;
        m20 = f.x; m21 = f.y; m22 = f.z;
        m03 = -(Fx.mul(s.x, pos.x) + Fx.mul(s.y, pos.y) + Fx.mul(s.z, pos.z));
        m13 = -(Fx.mul(v.x, pos.x) + Fx.mul(v.y, pos.y) + Fx.mul(v.z, pos.z));
        m23 = -(Fx.mul(f.x, pos.x) + Fx.mul(f.y, pos.y) + Fx.mul(f.z, pos.z));
    }

    private static Vector3D cross(Vector3D a, Vector3D b) {
        return new Vector3D(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    /** @deprecated */
    public final void setViewTrans(Vector3D pos, Vector3D look, Vector3D up) {
        lookAt(pos, look, up);
    }

    /** the point v transformed by this matrix (a new vector) */
    public final Vector3D transform(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        int x = v.x;
        int y = v.y;
        int z = v.z;
        return new Vector3D(Fx.mul(x, m00) + Fx.mul(y, m01) + Fx.mul(z, m02) + m03,
                Fx.mul(x, m10) + Fx.mul(y, m11) + Fx.mul(z, m12) + m13,
                Fx.mul(x, m20) + Fx.mul(y, m21) + Fx.mul(z, m22) + m23);
    }

    /** @deprecated */
    public final Vector3D transPoint(Vector3D v) {
        return transform(v);
    }
}
