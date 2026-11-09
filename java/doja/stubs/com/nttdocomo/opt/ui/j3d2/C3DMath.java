package com.nttdocomo.opt.ui.j3d2;

/** 16.16 fixed point; angles in fixed point degrees. */
public final class C3DMath {
    private static final double ONE = 65536.0;

    public C3DMath() {
    }

    private static double d(int v) {
        return v / ONE;
    }

    private static int f(double v) {
        return (int) (v * ONE);
    }

    private static double rad(int deg) {
        return d(deg) * java.lang.Math.PI / 180.0;
    }

    private static int deg(double rad) {
        return f(rad / java.lang.Math.PI * 180.0);
    }

    public static int acos(int value) {
        return deg(java.lang.Math.PI / 2 - asinD(d(value)));
    }

    public static int abs(int value) {
        return value < 0 ? -value : value;
    }

    public static int asin(int value) {
        return deg(asinD(d(value)));
    }

    private static double asinD(double x) {
        if (x >= 1) return java.lang.Math.PI / 2;
        if (x <= -1) return -java.lang.Math.PI / 2;
        return com.funkey.doja.FMath.atan(x / java.lang.Math.sqrt(1 - x * x));
    }

    public static int atan(int value) {
        return deg(com.funkey.doja.FMath.atan(d(value)));
    }

    public static int atan2(int y, int x) {
        return deg(com.funkey.doja.FMath.atan2(d(y), d(x)));
    }

    public static int cos(int value) {
        return f(java.lang.Math.cos(rad(value)));
    }

    public static int div(int a, int b) {
        return (int) (((long) a << 16) / b);
    }

    public static int log(int value) {
        return f(com.funkey.doja.FMath.ln(d(value)));
    }

    public static int mul(int a, int b) {
        return (int) (((long) a * b) >> 16);
    }

    public static int pow(int a, int b) {
        double x = d(a), y = d(b);
        if (x == 0) return 0;
        if (x < 0) {
            double v = com.funkey.doja.FMath.exp(com.funkey.doja.FMath.ln(-x) * y);
            return f(((int) y & 1) != 0 ? -v : v);
        }
        return f(com.funkey.doja.FMath.exp(com.funkey.doja.FMath.ln(x) * y));
    }

    public static int sin(int value) {
        return f(java.lang.Math.sin(rad(value)));
    }

    public static int sqrt(int value) {
        return f(java.lang.Math.sqrt(d(value)));
    }

    public static int tan(int value) {
        return f(java.lang.Math.tan(rad(value)));
    }
}
