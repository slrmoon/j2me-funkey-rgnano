package com.nttdocomo.ui.graphics3d.collision;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class Collision {

    public Collision() {
    super();
    }

    public boolean isHit(com.nttdocomo.ui.graphics3d.collision.BVFigure p0, com.nttdocomo.ui.graphics3d.collision.BVFigure p1, boolean p2, boolean p3, boolean p4) {
    return false;
    }

    public boolean isHit(com.nttdocomo.ui.graphics3d.collision.Shape p0, com.nttdocomo.ui.graphics3d.collision.BVFigure p1, boolean p2, boolean p3) {
    return false;
    }

    public boolean isHit(com.nttdocomo.ui.graphics3d.collision.Shape p0, com.nttdocomo.ui.graphics3d.collision.Sphere p1, com.nttdocomo.ui.util3d.Vector3D p2, boolean p3) {
    return false;
    }

    public void setObserver(com.nttdocomo.ui.graphics3d.collision.CollisionObserver p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}