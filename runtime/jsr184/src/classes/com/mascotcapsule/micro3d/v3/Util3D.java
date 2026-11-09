package com.mascotcapsule.micro3d.v3;

import com.funkey.mc3.Fx;

/**
 * Fixed-point helpers: angles 4096 = 360 degrees, sin/cos 4096 = 1.0
 * (the scale every AffineTrans element uses; one translation of the
 * handset documentation says 1024).
 */
public class Util3D {
    private Util3D() {
    }

    /** square root of |p| */
    public static final int sqrt(int p) {
        return Fx.sqrt(p);
    }

    public static final int sin(int p) {
        return Fx.sin(p);
    }

    public static final int cos(int p) {
        return Fx.cos(p);
    }
}
