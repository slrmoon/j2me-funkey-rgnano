package com.nttdocomo.opt.ui.j3d;

/** MascotCapsule fixed point: angles 4096 = 360 degrees, results x4096. */
public final class Math {
    protected Math() {
    }

    public static int sqrt(int value) {
        if (value < 0) {
            throw new IllegalArgumentException("value");
        }
        return (int) (java.lang.Math.sqrt(value) + 0.5);
    }

    public static int sin(int angle) {
        double v = java.lang.Math.sin(angle * java.lang.Math.PI / 2048.0) * 4096.0;
        return (int) java.lang.Math.floor(v + 0.5);
    }

    public static int cos(int angle) {
        return sin(angle + 1024);
    }

    public static int atan2(int y, int x) {
        double v = com.funkey.doja.FMath.atan2(y, x) * 2048.0 / java.lang.Math.PI;
        /* 0..4095: games index tables with the angle (FIFA 2006) */
        return ((int) java.lang.Math.floor(v + 0.5)) & 4095;
    }

}
