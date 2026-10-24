package com.nttdocomo.opt.ui.j3d;

/**
 * 3x4 affine matrix: rotation/scale part with 4096 = 1.0, translation in
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

    public AffineTrans() {
    }

    public AffineTrans(int a00, int a01, int a02, int a03, int a10, int a11, int a12, int a13,
            int a20, int a21, int a22, int a23) {
        setElement(a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23);
    }

    static int mul(int a, int b) {
        return (int) (((long) a * b + 2048) >> 12);
    }

    public void setElement(int row, int column, int value) {
        if (row < 0 || row > 2 || column < 0 || column > 3) {
            throw new IllegalArgumentException("index");
        }
        switch (row * 4 + column) {
        case 0: m00 = value; break;
        case 1: m01 = value; break;
        case 2: m02 = value; break;
        case 3: m03 = value; break;
        case 4: m10 = value; break;
        case 5: m11 = value; break;
        case 6: m12 = value; break;
        case 7: m13 = value; break;
        case 8: m20 = value; break;
        case 9: m21 = value; break;
        case 10: m22 = value; break;
        default: m23 = value; break;
        }
    }

    public void setElement(int a00, int a01, int a02, int a03, int a10, int a11, int a12, int a13,
            int a20, int a21, int a22, int a23) {
        m00 = a00; m01 = a01; m02 = a02; m03 = a03;
        m10 = a10; m11 = a11; m12 = a12; m13 = a13;
        m20 = a20; m21 = a21; m22 = a22; m23 = a23;
    }

    public void setRow(int row, int x, int y, int z, int w) {
        switch (row) {
        case 0: m00 = x; m01 = y; m02 = z; m03 = w; break;
        case 1: m10 = x; m11 = y; m12 = z; m13 = w; break;
        case 2: m20 = x; m21 = y; m22 = z; m23 = w; break;
        default: throw new IllegalArgumentException("row");
        }
    }

    public void setColumn(int column, int x, int y, int z) {
        switch (column) {
        case 0: m00 = x; m10 = y; m20 = z; break;
        case 1: m01 = x; m11 = y; m21 = z; break;
        case 2: m02 = x; m12 = y; m22 = z; break;
        case 3: m03 = x; m13 = y; m23 = z; break;
        default: throw new IllegalArgumentException("column");
        }
    }

    public void setIdentity() {
        setElement(4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0);
    }

    public void mul(AffineTrans t) {
        mul(this, t);
    }

    /** this = t1 * t2 */
    public void mul(AffineTrans t1, AffineTrans t2) {
        if (t1 == null || t2 == null) {
            throw new NullPointerException();
        }
        int a00 = mul(t1.m00, t2.m00) + mul(t1.m01, t2.m10) + mul(t1.m02, t2.m20);
        int a01 = mul(t1.m00, t2.m01) + mul(t1.m01, t2.m11) + mul(t1.m02, t2.m21);
        int a02 = mul(t1.m00, t2.m02) + mul(t1.m01, t2.m12) + mul(t1.m02, t2.m22);
        int a03 = mul(t1.m00, t2.m03) + mul(t1.m01, t2.m13) + mul(t1.m02, t2.m23) + t1.m03;
        int a10 = mul(t1.m10, t2.m00) + mul(t1.m11, t2.m10) + mul(t1.m12, t2.m20);
        int a11 = mul(t1.m10, t2.m01) + mul(t1.m11, t2.m11) + mul(t1.m12, t2.m21);
        int a12 = mul(t1.m10, t2.m02) + mul(t1.m11, t2.m12) + mul(t1.m12, t2.m22);
        int a13 = mul(t1.m10, t2.m03) + mul(t1.m11, t2.m13) + mul(t1.m12, t2.m23) + t1.m13;
        int a20 = mul(t1.m20, t2.m00) + mul(t1.m21, t2.m10) + mul(t1.m22, t2.m20);
        int a21 = mul(t1.m20, t2.m01) + mul(t1.m21, t2.m11) + mul(t1.m22, t2.m21);
        int a22 = mul(t1.m20, t2.m02) + mul(t1.m21, t2.m12) + mul(t1.m22, t2.m22);
        int a23 = mul(t1.m20, t2.m03) + mul(t1.m21, t2.m13) + mul(t1.m22, t2.m23) + t1.m23;
        setElement(a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23);
    }

    /* the rotations keep the translation column */
    public void setRotateX(int a) {
        int c = Math.cos(a);
        int s = Math.sin(a);
        m00 = 4096; m01 = 0; m02 = 0;
        m10 = 0; m11 = c; m12 = -s;
        m20 = 0; m21 = s; m22 = c;
    }

    public void setRotateY(int a) {
        int c = Math.cos(a);
        int s = Math.sin(a);
        m00 = c; m01 = 0; m02 = s;
        m10 = 0; m11 = 4096; m12 = 0;
        m20 = -s; m21 = 0; m22 = c;
    }

    public void setRotateZ(int a) {
        int c = Math.cos(a);
        int s = Math.sin(a);
        m00 = c; m01 = -s; m02 = 0;
        m10 = s; m11 = c; m12 = 0;
        m20 = 0; m21 = 0; m22 = 4096;
    }

    public void setRotateV(Vector3D v, int a) {
        if (v == null) {
            throw new NullPointerException("v");
        }
        Vector3D n = new Vector3D(v.x, v.y, v.z);
        if (n.x == 0 && n.y == 0 && n.z == 0) {
            m00 = 4096; m01 = 0; m02 = 0;
            m10 = 0; m11 = 4096; m12 = 0;
            m20 = 0; m21 = 0; m22 = 4096;
            return;
        }
        n.normalize();
        int c = Math.cos(a);
        int s = Math.sin(a);
        int t = 4096 - c;
        int x = n.x;
        int y = n.y;
        int z = n.z;
        m00 = c + mul(mul(x, x), t);
        m01 = mul(mul(x, y), t) - mul(z, s);
        m02 = mul(mul(x, z), t) + mul(y, s);
        m10 = mul(mul(y, x), t) + mul(z, s);
        m11 = c + mul(mul(y, y), t);
        m12 = mul(mul(y, z), t) - mul(x, s);
        m20 = mul(mul(z, x), t) - mul(y, s);
        m21 = mul(mul(z, y), t) + mul(x, s);
        m22 = c + mul(mul(z, z), t);
    }

    /**
     * View matrix for a camera at position looking at look (a point) with
     * the given up vector: rows are the camera axes (x right, y down,
     * z forward).
     */
    public void lookAt(Vector3D position, Vector3D look, Vector3D up) {
        if (position == null || look == null || up == null) {
            throw new NullPointerException();
        }
        Vector3D f = new Vector3D(look.x - position.x, look.y - position.y, look.z - position.z);
        if (f.x == 0 && f.y == 0 && f.z == 0) {
            f.z = 4096;
        }
        f.normalize();
        Vector3D u = new Vector3D(up.x, up.y, up.z);
        if (u.x == 0 && u.y == 0 && u.z == 0) {
            u.y = 4096;
        }
        u.normalize();
        Vector3D s = new Vector3D();
        s.cross(f, u);
        if (s.x == 0 && s.y == 0 && s.z == 0) {
            /* up parallel to the view direction: pick another up */
            u.x = 0;
            u.y = f.z != 0 || f.x != 0 ? 4096 : 0;
            u.z = u.y == 0 ? 4096 : 0;
            s.cross(f, u);
            if (s.x == 0 && s.y == 0 && s.z == 0) {
                s.x = 4096;
            }
        }
        s.normalize();
        Vector3D v = new Vector3D();
        v.cross(f, s);
        v.normalize();
        m00 = s.x; m01 = s.y; m02 = s.z;
        m10 = v.x; m11 = v.y; m12 = v.z;
        m20 = f.x; m21 = f.y; m22 = f.z;
        m03 = -(mul(s.x, position.x) + mul(s.y, position.y) + mul(s.z, position.z));
        m13 = -(mul(v.x, position.x) + mul(v.y, position.y) + mul(v.z, position.z));
        m23 = -(mul(f.x, position.x) + mul(f.y, position.y) + mul(f.z, position.z));
    }

    public void transform(Vector3D v, Vector3D result) {
        if (v == null || result == null) {
            throw new NullPointerException();
        }
        int x = v.x;
        int y = v.y;
        int z = v.z;
        result.x = mul(x, m00) + mul(y, m01) + mul(z, m02) + m03;
        result.y = mul(x, m10) + mul(y, m11) + mul(z, m12) + m13;
        result.z = mul(x, m20) + mul(y, m21) + mul(z, m22) + m23;
    }

    void get(int[] d) {
        d[0] = m00; d[1] = m01; d[2] = m02; d[3] = m03;
        d[4] = m10; d[5] = m11; d[6] = m12; d[7] = m13;
        d[8] = m20; d[9] = m21; d[10] = m22; d[11] = m23;
    }
}
