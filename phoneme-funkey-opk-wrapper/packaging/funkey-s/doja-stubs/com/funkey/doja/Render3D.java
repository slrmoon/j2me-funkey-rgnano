package com.funkey.doja;

import com.nttdocomo.opt.ui.j3d.AffineTrans;
import com.nttdocomo.opt.ui.j3d.Figure;
import com.nttdocomo.opt.ui.j3d.PrimitiveArray;
import com.nttdocomo.opt.ui.j3d.Texture;
import com.nttdocomo.opt.ui.j3d.Vector3D;

/**
 * MascotCapsule v3 (com.nttdocomo.opt.ui.j3d) renderer state of one
 * Graphics. Rendering is not implemented yet: calls are logged.
 */
public final class Render3D {
    private final javax.microedition.lcdui.Image target;
    private final Object owner;

    public Render3D(javax.microedition.lcdui.Image t, Object g) {
        target = t;
        owner = g;
    }

    public void setViewTrans(AffineTrans t) {
        Stub.log("Graphics3D.setViewTrans");
    }

    public void setViewTransArray(AffineTrans[] t) {
        Stub.log("Graphics3D.setViewTransArray");
    }

    public void setViewTrans(int index) {
        Stub.log("Graphics3D.setViewTrans(int)");
    }

    public void setScreenCenter(int x, int y) {
        Stub.log("Graphics3D.setScreenCenter");
    }

    public void setScreenScale(int x, int y) {
        Stub.log("Graphics3D.setScreenScale");
    }

    public void setScreenView(int x, int y) {
        Stub.log("Graphics3D.setScreenView");
    }

    public void setPerspective(int near, int far, int w) {
        Stub.log("Graphics3D.setPerspective");
    }

    public void setPerspective(int near, int far, int w, int h) {
        Stub.log("Graphics3D.setPerspective");
    }

    public void drawFigure(Figure f) {
        Stub.log("Graphics3D.drawFigure");
    }

    public void renderFigure(Figure f) {
        Stub.log("Graphics3D.renderFigure");
    }

    public void flush() {
        Stub.log("Graphics3D.flush");
    }

    public void enableSphereMap(boolean e) {
    }

    public void setSphereTexture(Texture t) {
    }

    public void enableLight(boolean e) {
    }

    public void setAmbientLight(int c) {
    }

    public void setDirectionLight(Vector3D d, int c) {
    }

    public void enableSemiTransparent(boolean e) {
    }

    public void setClipRect3D(int x, int y, int w, int h) {
    }

    public void enableToonShader(boolean e) {
    }

    public void setToonParam(int hi, int mid, int sh) {
    }

    public void setPrimitiveTextureArray(Texture t) {
    }

    public void setPrimitiveTextureArray(Texture[] t) {
    }

    public void setPrimitiveTexture(int i) {
    }

    public void renderPrimitives(PrimitiveArray p, int start, int count, int attr) {
        Stub.log("Graphics3D.renderPrimitives");
    }

    public void executeCommandList(int[] cmds) {
        Stub.log("Graphics3D.executeCommandList");
    }
}
