package com.funkey.doja.gl;

/** 4x4 float matrices, column-major like OpenGL. */
public final class Mat {
    private Mat() {
    }

    public static void identity(float[] m) {
        for (int i = 0; i < 16; i++) {
            m[i] = 0f;
        }
        m[0] = m[5] = m[10] = m[15] = 1f;
    }

    /** r = a * b (r may alias a or b) */
    public static void mul(float[] r, float[] a, float[] b) {
        float[] t = new float[16];
        mulTo(t, a, b);
        System.arraycopy(t, 0, r, 0, 16);
    }

    static void mulTo(float[] t, float[] a, float[] b) {
        for (int c = 0; c < 4; c++) {
            float b0 = b[c * 4];
            float b1 = b[c * 4 + 1];
            float b2 = b[c * 4 + 2];
            float b3 = b[c * 4 + 3];
            for (int r = 0; r < 4; r++) {
                t[c * 4 + r] = a[r] * b0 + a[4 + r] * b1 + a[8 + r] * b2 + a[12 + r] * b3;
            }
        }
    }

    private static final float[] tmp = new float[16];
    private static final float[] res = new float[16];

    /** m = m * t */
    public static synchronized void post(float[] m, float[] t) {
        mulTo(res, m, t);
        System.arraycopy(res, 0, m, 0, 16);
    }

    public static synchronized void translate(float[] m, float x, float y, float z) {
        identity(tmp);
        tmp[12] = x;
        tmp[13] = y;
        tmp[14] = z;
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
    }

    public static synchronized void scale(float[] m, float x, float y, float z) {
        identity(tmp);
        tmp[0] = x;
        tmp[5] = y;
        tmp[10] = z;
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
    }

    /** angle in degrees about (x, y, z) */
    public static synchronized void rotate(float[] m, float angle, float x, float y, float z) {
        float len = (float) Math.sqrt(x * x + y * y + z * z);
        if (len == 0f) {
            return;
        }
        x /= len;
        y /= len;
        z /= len;
        double a = angle * Math.PI / 180.0;
        float c = (float) Math.cos(a);
        float s = (float) Math.sin(a);
        float t = 1f - c;
        identity(tmp);
        tmp[0] = x * x * t + c;
        tmp[1] = y * x * t + z * s;
        tmp[2] = x * z * t - y * s;
        tmp[4] = x * y * t - z * s;
        tmp[5] = y * y * t + c;
        tmp[6] = y * z * t + x * s;
        tmp[8] = x * z * t + y * s;
        tmp[9] = y * z * t - x * s;
        tmp[10] = z * z * t + c;
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
    }

    public static synchronized void frustum(float[] m, float l, float r, float b, float t, float n, float f) {
        identity(tmp);
        tmp[0] = 2f * n / (r - l);
        tmp[5] = 2f * n / (t - b);
        tmp[8] = (r + l) / (r - l);
        tmp[9] = (t + b) / (t - b);
        tmp[10] = -(f + n) / (f - n);
        tmp[11] = -1f;
        tmp[14] = -2f * f * n / (f - n);
        tmp[15] = 0f;
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
    }

    public static synchronized void ortho(float[] m, float l, float r, float b, float t, float n, float f) {
        identity(tmp);
        tmp[0] = 2f / (r - l);
        tmp[5] = 2f / (t - b);
        tmp[10] = -2f / (f - n);
        tmp[12] = -(r + l) / (r - l);
        tmp[13] = -(t + b) / (t - b);
        tmp[14] = -(f + n) / (f - n);
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
    }

    public static synchronized void lookAt(float[] m, float ex, float ey, float ez,
            float cx, float cy, float cz, float ux, float uy, float uz) {
        float fx = cx - ex;
        float fy = cy - ey;
        float fz = cz - ez;
        float fl = (float) Math.sqrt(fx * fx + fy * fy + fz * fz);
        if (fl == 0f) {
            return;
        }
        fx /= fl;
        fy /= fl;
        fz /= fl;
        float sx = fy * uz - fz * uy;
        float sy = fz * ux - fx * uz;
        float sz = fx * uy - fy * ux;
        float sl = (float) Math.sqrt(sx * sx + sy * sy + sz * sz);
        if (sl == 0f) {
            return;
        }
        sx /= sl;
        sy /= sl;
        sz /= sl;
        float vx = sy * fz - sz * fy;
        float vy = sz * fx - sx * fz;
        float vz = sx * fy - sy * fx;
        identity(tmp);
        tmp[0] = sx;
        tmp[4] = sy;
        tmp[8] = sz;
        tmp[1] = vx;
        tmp[5] = vy;
        tmp[9] = vz;
        tmp[2] = -fx;
        tmp[6] = -fy;
        tmp[10] = -fz;
        mulTo(res, m, tmp);
        System.arraycopy(res, 0, m, 0, 16);
        translate(m, -ex, -ey, -ez);
    }

    /** row-major copy for M3G Transform.set */
    public static void toRowMajor(float[] m, float[] out) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                out[r * 4 + c] = m[c * 4 + r];
            }
        }
    }

    /** out = m * (x, y, z, w) */
    public static void transform(float[] m, float x, float y, float z, float w, float[] out) {
        out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
        out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
        out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
        out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
    }
}
