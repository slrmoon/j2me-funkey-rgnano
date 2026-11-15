package com.nttdocomo.ui.util3d;

import com.funkey.doja.FMath;

/**
 * The handset's 3D arithmetic: values are fixed point inside (1.0 = 4096),
 * angles are degrees.
 */
public class FastMath {
    private static final float ONE = 4096f;

    protected FastMath() {
    }

    public static int floatToInnerInt(float v) {
        return (int) (v * ONE);
    }

    public static float innerIntToFloat(int v) {
        return v / ONE;
    }

    public static float add(float x, float y) {
        return innerIntToFloat(floatToInnerInt(x) + floatToInnerInt(y));
    }

    public static float sub(float x, float y) {
        return innerIntToFloat(floatToInnerInt(x) - floatToInnerInt(y));
    }

    public static float mul(float x, float y) {
        return innerIntToFloat((int) (((long) floatToInnerInt(x) * floatToInnerInt(y)) >> 12));
    }

    public static float div(float x, float y) {
        int d = floatToInnerInt(y);
        if (d == 0) {
            throw new ArithmeticException("/ by zero");
        }
        return innerIntToFloat((int) ((((long) floatToInnerInt(x)) << 12) / d));
    }

    public static float sqrt(float x) {
        if (x < 0) {
            throw new ArithmeticException("sqrt");
        }
        return innerIntToFloat(floatToInnerInt((float) java.lang.Math.sqrt(x)));
    }

    public static float sin(float a) {
        return innerIntToFloat(floatToInnerInt((float) java.lang.Math.sin(java.lang.Math.toRadians(a))));
    }

    public static float cos(float a) {
        return innerIntToFloat(floatToInnerInt((float) java.lang.Math.cos(java.lang.Math.toRadians(a))));
    }

    public static float tan(float a) {
        return (float) java.lang.Math.tan(java.lang.Math.toRadians(a));
    }

    public static float asin(float a) {
        if (a < -1 || a > 1) {
            throw new ArithmeticException("asin");
        }
        return (float) java.lang.Math.toDegrees(
                FMath.atan2(a, java.lang.Math.sqrt(1 - (double) a * a)));
    }

    public static float acos(float a) {
        if (a < -1 || a > 1) {
            throw new ArithmeticException("acos");
        }
        return (float) java.lang.Math.toDegrees(
                FMath.atan2(java.lang.Math.sqrt(1 - (double) a * a), a));
    }

    public static float atan(float a) {
        return (float) java.lang.Math.toDegrees(FMath.atan(a));
    }

    public static float atan2(float a, float b) {
        return (float) java.lang.Math.toDegrees(FMath.atan2(a, b));
    }

    public static float abs(float a) {
        return a < 0 ? -a : a;
    }
}
