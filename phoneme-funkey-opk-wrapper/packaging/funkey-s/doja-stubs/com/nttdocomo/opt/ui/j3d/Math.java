package com.nttdocomo.opt.ui.j3d;

public final class Math {
    private Math() {
    }

    public static int sin(int value) {
        return (int)(java.lang.Math.sin(value * java.lang.Math.PI / 180.0) * 65536.0);
    }

    public static int cos(int value) {
        return (int)(java.lang.Math.cos(value * java.lang.Math.PI / 180.0) * 65536.0);
    }
}
