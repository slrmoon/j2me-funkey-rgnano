package com.docomostar.opt.ui.j3d;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class Vector3D {

    public int x;
    public int y;
    public int z;

    public Vector3D() {
    super();
    }

    public Vector3D(int p0, int p1, int p2) {
    super();
    }

    public void cross(com.docomostar.opt.ui.j3d.Vector3D p0, com.docomostar.opt.ui.j3d.Vector3D p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public int dot(com.docomostar.opt.ui.j3d.Vector3D p0, com.docomostar.opt.ui.j3d.Vector3D p1) {
    return 0;
    }

    public void normalize() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}