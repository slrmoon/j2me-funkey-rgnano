package com.nttdocomo.ui.util3d;

/**
 * A 4x4 matrix, row by row (elements 3, 7 and 11 are the translation).
 * scale, rotate and translate multiply from the right.
 */
public class Transform {
    private final float[] m = new float[16];

    public Transform() {
        setIdentity();
    }

    public Transform(Transform transform) {
        set(transform);
    }

    public void setIdentity() {
        for (int i = 0; i < 16; i++) {
            m[i] = (i % 5) == 0 ? 1 : 0;
        }
    }

    public void set(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        System.arraycopy(transform.m, 0, m, 0, 16);
    }

    public void set(float[] matrix) {
        if (matrix == null) {
            throw new NullPointerException();
        }
        if (matrix.length < 16) {
            throw new IllegalArgumentException();
        }
        System.arraycopy(matrix, 0, m, 0, 16);
    }

    public void get(float[] matrix) {
        if (matrix == null) {
            throw new NullPointerException();
        }
        if (matrix.length < 16) {
            throw new IllegalArgumentException();
        }
        System.arraycopy(m, 0, matrix, 0, 16);
    }

    public void set(int index, float value) {
        if (index < 0 || index > 15) {
            throw new IllegalArgumentException();
        }
        m[index] = value;
    }

    public float get(int index) {
        if (index < 0 || index > 15) {
            throw new IllegalArgumentException();
        }
        return m[index];
    }

    public void invert() {
        /* Gauss-Jordan on [m | I] */
        double[] a = new double[32];
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                a[r * 8 + c] = m[r * 4 + c];
            }
            a[r * 8 + 4 + r] = 1;
        }
        for (int c = 0; c < 4; c++) {
            int p = c;
            for (int r = c + 1; r < 4; r++) {
                if (java.lang.Math.abs(a[r * 8 + c]) > java.lang.Math.abs(a[p * 8 + c])) {
                    p = r;
                }
            }
            if (a[p * 8 + c] == 0) {
                throw new ArithmeticException("singular");
            }
            if (p != c) {
                for (int k = 0; k < 8; k++) {
                    double t = a[p * 8 + k];
                    a[p * 8 + k] = a[c * 8 + k];
                    a[c * 8 + k] = t;
                }
            }
            double d = a[c * 8 + c];
            for (int k = 0; k < 8; k++) {
                a[c * 8 + k] /= d;
            }
            for (int r = 0; r < 4; r++) {
                if (r != c) {
                    double f = a[r * 8 + c];
                    if (f != 0) {
                        for (int k = 0; k < 8; k++) {
                            a[r * 8 + k] -= f * a[c * 8 + k];
                        }
                    }
                }
            }
        }
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                m[r * 4 + c] = (float) a[r * 8 + 4 + c];
            }
        }
    }

    public void transpose() {
        for (int r = 0; r < 4; r++) {
            for (int c = r + 1; c < 4; c++) {
                float t = m[r * 4 + c];
                m[r * 4 + c] = m[c * 4 + r];
                m[c * 4 + r] = t;
            }
        }
    }

    public void multiply(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        mulRight(transform.m);
    }

    private void mulRight(float[] b) {
        float[] r = new float[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r[i * 4 + j] = m[i * 4] * b[j] + m[i * 4 + 1] * b[4 + j]
                        + m[i * 4 + 2] * b[8 + j] + m[i * 4 + 3] * b[12 + j];
            }
        }
        System.arraycopy(r, 0, m, 0, 16);
    }

    public void scale(float x, float y, float z) {
        for (int r = 0; r < 4; r++) {
            m[r * 4] *= x;
            m[r * 4 + 1] *= y;
            m[r * 4 + 2] *= z;
        }
    }

    public void scale(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        scale(v.getX(), v.getY(), v.getZ());
    }

    public void rotate(float x, float y, float z, float angle) {
        if (angle == 0) {
            return;
        }
        double len = java.lang.Math.sqrt((double) x * x + (double) y * y + (double) z * z);
        if (len == 0) {
            throw new IllegalArgumentException();
        }
        double ax = x / len;
        double ay = y / len;
        double az = z / len;
        double rad = java.lang.Math.toRadians(angle);
        double c = java.lang.Math.cos(rad);
        double s = java.lang.Math.sin(rad);
        double t = 1 - c;
        float[] r = new float[16];
        r[0] = (float) (t * ax * ax + c);
        r[1] = (float) (t * ax * ay - s * az);
        r[2] = (float) (t * ax * az + s * ay);
        r[4] = (float) (t * ax * ay + s * az);
        r[5] = (float) (t * ay * ay + c);
        r[6] = (float) (t * ay * az - s * ax);
        r[8] = (float) (t * ax * az - s * ay);
        r[9] = (float) (t * ay * az + s * ax);
        r[10] = (float) (t * az * az + c);
        r[15] = 1;
        mulRight(r);
    }

    public void rotate(Vector3D v, float angle) {
        if (v == null) {
            throw new NullPointerException();
        }
        rotate(v.getX(), v.getY(), v.getZ(), angle);
    }

    public void rotateQuat(float x, float y, float z, float w) {
        double n = java.lang.Math.sqrt((double) x * x + (double) y * y + (double) z * z + (double) w * w);
        if (n == 0) {
            throw new IllegalArgumentException();
        }
        double qx = x / n;
        double qy = y / n;
        double qz = z / n;
        double qw = w / n;
        float[] r = new float[16];
        r[0] = (float) (1 - 2 * (qy * qy + qz * qz));
        r[1] = (float) (2 * (qx * qy - qz * qw));
        r[2] = (float) (2 * (qx * qz + qy * qw));
        r[4] = (float) (2 * (qx * qy + qz * qw));
        r[5] = (float) (1 - 2 * (qx * qx + qz * qz));
        r[6] = (float) (2 * (qy * qz - qx * qw));
        r[8] = (float) (2 * (qx * qz - qy * qw));
        r[9] = (float) (2 * (qy * qz + qx * qw));
        r[10] = (float) (1 - 2 * (qx * qx + qy * qy));
        r[15] = 1;
        mulRight(r);
    }

    public void rotateQuat(Vector3D v, float w) {
        if (v == null) {
            throw new NullPointerException();
        }
        rotateQuat(v.getX(), v.getY(), v.getZ(), w);
    }

    public void translate(float x, float y, float z) {
        for (int r = 0; r < 4; r++) {
            m[r * 4 + 3] += m[r * 4] * x + m[r * 4 + 1] * y + m[r * 4 + 2] * z;
        }
    }

    public void translate(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        translate(v.getX(), v.getY(), v.getZ());
    }

    /**
     * The view matrix of a camera at position looking at look: its rows
     * are the camera's side, up and forward (+z into the screen).
     */
    public void lookAt(Vector3D position, Vector3D look, Vector3D up) {
        if (position == null || look == null || up == null) {
            throw new NullPointerException();
        }
        Vector3D f = new Vector3D(look.getX() - position.getX(),
                look.getY() - position.getY(), look.getZ() - position.getZ());
        Vector3D s = new Vector3D();
        Vector3D u = new Vector3D();
        try {
            f.normalize();
            s.cross(f, up);
            s.normalize();
            u.cross(f, s);
            u.normalize();
        } catch (ArithmeticException e) {
            throw new IllegalArgumentException();
        }
        m[0] = s.getX();
        m[1] = s.getY();
        m[2] = s.getZ();
        m[3] = -s.dot(position);
        m[4] = u.getX();
        m[5] = u.getY();
        m[6] = u.getZ();
        m[7] = -u.dot(position);
        m[8] = f.getX();
        m[9] = f.getY();
        m[10] = f.getZ();
        m[11] = -f.dot(position);
        m[12] = 0;
        m[13] = 0;
        m[14] = 0;
        m[15] = 1;
    }

    public void transVector(Vector3D v, Vector3D result) {
        if (v == null || result == null) {
            throw new NullPointerException();
        }
        float x = v.getX();
        float y = v.getY();
        float z = v.getZ();
        result.set(m[0] * x + m[1] * y + m[2] * z + m[3],
                m[4] * x + m[5] * y + m[6] * z + m[7],
                m[8] * x + m[9] * y + m[10] * z + m[11]);
    }
}
