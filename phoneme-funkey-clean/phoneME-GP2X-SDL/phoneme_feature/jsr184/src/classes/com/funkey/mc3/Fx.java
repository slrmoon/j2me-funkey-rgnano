package com.funkey.mc3;

/**
 * MascotCapsule fixed point: 4096 = 1.0, angles 4096 = 360 degrees.
 */
public final class Fx {
    private Fx() {
    }

    /** a * b in 4096 = 1.0, rounded */
    public static int mul(int a, int b) {
        return (int) (((long) a * b + 2048) >> 12);
    }

    public static int sin(int angle) {
        double v = Math.sin(angle * Math.PI / 2048.0) * 4096.0;
        return (int) Math.floor(v + 0.5);
    }

    public static int cos(int angle) {
        return sin(angle + 1024);
    }

    /** rounded square root of |value| */
    public static int sqrt(int value) {
        if (value < 0) {
            value = -value;
        }
        return (int) (Math.sqrt(value) + 0.5);
    }

    /**
     * Scales v[off..off+2] to length 4096; a zero vector stays as it is.
     * Returns false for the zero vector.
     */
    public static boolean normalize(int[] v, int off) {
        long x = v[off];
        long y = v[off + 1];
        long z = v[off + 2];
        long len = (long) (Math.sqrt((double) (x * x + y * y + z * z)) + 0.5);
        if (len == 0) {
            return false;
        }
        v[off] = (int) ((x << 12) / len);
        v[off + 1] = (int) ((y << 12) / len);
        v[off + 2] = (int) ((z << 12) / len);
        return true;
    }
}
