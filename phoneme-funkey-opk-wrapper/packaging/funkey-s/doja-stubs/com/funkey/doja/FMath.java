package com.funkey.doja;

/** Math missing from CLDC 1.1 (atan, atan2, log, exp). */
public final class FMath {
    private FMath() {
    }

    public static double atan2(double y, double x) {
        /* CLDC has no Math.atan2 */
        if (x == 0) {
            return y > 0 ? java.lang.Math.PI / 2 : y < 0 ? -java.lang.Math.PI / 2 : 0;
        }
        double a = atan(y / x);
        if (x < 0) {
            a += y >= 0 ? java.lang.Math.PI : -java.lang.Math.PI;
        }
        return a;
    }

    public static double atan(double x) {
        boolean neg = x < 0;
        if (neg) x = -x;
        boolean inv = x > 1;
        if (inv) x = 1 / x;
        /* reduce to |x| <= tan(pi/12) */
        boolean red = x > 0.2679491924311227;
        if (red) x = (x * 1.7320508075688772 - 1) / (x + 1.7320508075688772);
        double x2 = x * x, r = x, t = x;
        for (int k = 3; k < 40; k += 2) {
            t *= -x2;
            r += t / k;
        }
        if (red) r += java.lang.Math.PI / 6;
        if (inv) r = java.lang.Math.PI / 2 - r;
        return neg ? -r : r;
    }
    public static double ln(double x) {
        if (x <= 0) return Double.NEGATIVE_INFINITY;
        int e = 0;
        while (x > 2) { x /= 2; e++; }
        while (x < 1) { x *= 2; e--; }
        double y = (x - 1) / (x + 1), y2 = y * y, r = 0, t = y;
        for (int k = 1; k < 60; k += 2) {
            r += t / k;
            t *= y2;
        }
        return 2 * r + e * 0.6931471805599453;
    }

    public static double exp(double x) {
        int n = (int) java.lang.Math.floor(x / 0.6931471805599453);
        double r = x - n * 0.6931471805599453, s = 1, t = 1;
        for (int k = 1; k < 30; k++) {
            t *= r / k;
            s += t;
        }
        while (n > 0) { s *= 2; n--; }
        while (n < 0) { s /= 2; n++; }
        return s;
    }

}
