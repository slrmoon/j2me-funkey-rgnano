package com.nttdocomo.ui.ogl.math;

/** float math for the DoJa OpenGL ES API (radians) */
public final class FloatMath {
    private FloatMath() {
    }

    public static float sin(float value) {
        return (float) Math.sin(value);
    }

    public static float cos(float value) {
        return (float) Math.cos(value);
    }

    public static float tan(float value) {
        return (float) Math.tan(value);
    }

    public static float sqrt(float value) {
        return (float) Math.sqrt(value);
    }

    public static float atan(float value) {
        return atan2(value, 1f);
    }

    /* CLDC has no Math.atan: a polynomial with |error| < 1e-5 */
    public static float atan2(float y, float x) {
        if (x == 0f && y == 0f) {
            return 0f;
        }
        float ax = Math.abs(x), ay = Math.abs(y);
        float a = Math.min(ax, ay) / Math.max(ax, ay);
        float s = a * a;
        float r = ((((-0.0040540580f * s + 0.0218612288f) * s - 0.0559098861f) * s
                + 0.0964200441f) * s - 0.1390853351f) * s;
        r = (((r + 0.1994653599f) * s - 0.3332985605f) * s + 0.9999993329f) * a;
        if (ay > ax) {
            r = 1.5707963268f - r;
        }
        if (x < 0f) {
            r = 3.1415926536f - r;
        }
        return y < 0f ? -r : r;
    }
}
