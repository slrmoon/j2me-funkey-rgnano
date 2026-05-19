package com.nttdocomo.opt.ui.j3d;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class AffineTrans {

    public int m00;
    public int m01;
    public int m02;
    public int m03;
    public int m10;
    public int m11;
    public int m12;
    public int m13;
    public int m20;
    public int m21;
    public int m22;
    public int m23;

    public AffineTrans() {
    super();
    }

    public AffineTrans(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11) {
    super();
    }

    public void lookAt(com.nttdocomo.opt.ui.j3d.Vector3D p0, com.nttdocomo.opt.ui.j3d.Vector3D p1, com.nttdocomo.opt.ui.j3d.Vector3D p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void mul(com.nttdocomo.opt.ui.j3d.AffineTrans p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void mul(com.nttdocomo.opt.ui.j3d.AffineTrans p0, com.nttdocomo.opt.ui.j3d.AffineTrans p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setColumn(int p0, int p1, int p2, int p3) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setElement(int p0, int p1, int p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setElement(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setIdentity() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setRotateV(com.nttdocomo.opt.ui.j3d.Vector3D p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setRotateX(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setRotateY(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setRotateZ(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setRow(int p0, int p1, int p2, int p3, int p4) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void transform(com.nttdocomo.opt.ui.j3d.Vector3D p0, com.nttdocomo.opt.ui.j3d.Vector3D p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}