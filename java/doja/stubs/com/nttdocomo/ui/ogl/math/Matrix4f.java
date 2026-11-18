package com.nttdocomo.ui.ogl.math;

/**
 * 4x4 matrix as OpenGL keeps it: column-major, m[col * 4 + row], so m can go
 * straight to glLoadMatrixf / glMultMatrixf. A new matrix is the identity.
 * translate / rotate / scale / lookAt multiply on the right, as the GL calls.
 */
public final class Matrix4f {
    public float[] m = new float[16];

    public Matrix4f() {
        setIdentity();
    }

    public Matrix4f(Matrix4f matrix) {
        System.arraycopy(matrix.m, 0, m, 0, 16);
    }

    /** this = this * matrix */
    public void mul(Matrix4f matrix) {
        mul(this, matrix);
    }

    /** this = left * right */
    public void mul(Matrix4f left, Matrix4f right) {
        float[] a = left.m, b = right.m, r = new float[16];
        for (int c = 0; c < 4; c++) {
            for (int row = 0; row < 4; row++) {
                r[c * 4 + row] = a[row] * b[c * 4] + a[4 + row] * b[c * 4 + 1]
                        + a[8 + row] * b[c * 4 + 2] + a[12 + row] * b[c * 4 + 3];
            }
        }
        m = r;
    }

    /** tuple = this * tuple */
    public void transform(Tuple4f t) {
        float x = t.x, y = t.y, z = t.z, w = t.w;
        t.x = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
        t.y = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
        t.z = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
        t.w = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
    }

    public void invert() {
        float[] a = m, inv = new float[16];
        inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15]
                + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
        inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15]
                - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
        inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15]
                + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
        inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14]
                - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
        inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15]
                - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
        inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15]
                + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
        inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15]
                - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
        inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14]
                + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
        inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15]
                + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
        inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15]
                - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
        inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15]
                + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
        inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14]
                - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
        inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11]
                - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
        inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11]
                + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
        inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11]
                - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
        inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10]
                + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];
        float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
        if (det == 0f) {
            return;
        }
        det = 1f / det;
        for (int i = 0; i < 16; i++) {
            inv[i] *= det;
        }
        m = inv;
    }

    public void setIdentity() {
        if (m == null) {
            m = new float[16];
        }
        for (int i = 0; i < 16; i++) {
            m[i] = (i % 5 == 0) ? 1f : 0f;
        }
    }

    public void translate(float x, float y, float z) {
        Matrix4f t = new Matrix4f();
        t.m[12] = x;
        t.m[13] = y;
        t.m[14] = z;
        mul(t);
    }

    /** angle in degrees, as glRotatef */
    public void rotate(float angle, float x, float y, float z) {
        float l = (float) Math.sqrt(x * x + y * y + z * z);
        if (l == 0f) {
            return;
        }
        x /= l;
        y /= l;
        z /= l;
        double rad = angle * Math.PI / 180.0;
        float c = (float) Math.cos(rad), s = (float) Math.sin(rad), k = 1f - c;
        Matrix4f r = new Matrix4f();
        float[] a = r.m;
        a[0] = x * x * k + c;     a[4] = x * y * k - z * s; a[8] = x * z * k + y * s;
        a[1] = y * x * k + z * s; a[5] = y * y * k + c;     a[9] = y * z * k - x * s;
        a[2] = x * z * k - y * s; a[6] = y * z * k + x * s; a[10] = z * z * k + c;
        mul(r);
    }

    public void scale(float x, float y, float z) {
        Matrix4f t = new Matrix4f();
        t.m[0] = x;
        t.m[5] = y;
        t.m[10] = z;
        mul(t);
    }

    /** as gluLookAt */
    public void lookAt(Point3f eye, Point3f center, Vector3f up) {
        Vector3f f = new Vector3f(center.x - eye.x, center.y - eye.y, center.z - eye.z);
        f.normalize();
        Vector3f u = new Vector3f(up.x, up.y, up.z);
        u.normalize();
        Vector3f s = new Vector3f();
        s.cross(f, u);
        s.normalize();
        u.cross(s, f);
        Matrix4f r = new Matrix4f();
        float[] a = r.m;
        a[0] = s.x;  a[4] = s.y;  a[8] = s.z;
        a[1] = u.x;  a[5] = u.y;  a[9] = u.z;
        a[2] = -f.x; a[6] = -f.y; a[10] = -f.z;
        mul(r);
        translate(-eye.x, -eye.y, -eye.z);
    }
}
