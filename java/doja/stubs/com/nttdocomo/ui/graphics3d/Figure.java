package com.nttdocomo.ui.graphics3d;

import com.nttdocomo.opt.ui.j3d.FkMesh;

/** A model with bones (MBAC data), posed by an action and a time. */
public class Figure extends DrawableObject3D {
    com.nttdocomo.opt.ui.j3d.Figure fkFigure;
    Texture[] fkTextures;
    ActionTable fkActions;
    int fkAction;
    int fkBlend = BLEND_NORMAL;
    float fkAlpha = 1f;

    Figure(byte[] data) {
        super(TYPE_FIGURE);
        fkFigure = FkMesh.load(data);
    }

    public void setTexture(Texture texture) {
        fkTextures = texture == null ? null : new Texture[] { texture };
    }

    public void setTextures(Texture[] textures) {
        if (textures == null) {
            throw new NullPointerException("textures");
        }
        Texture[] copy = new Texture[textures.length];
        for (int i = 0; i < textures.length; i++) {
            if (textures[i] == null) {
                throw new NullPointerException("textures");
            }
            copy[i] = textures[i];
        }
        fkTextures = copy;
    }

    public void setAction(ActionTable actionTable, int action) {
        if (actionTable == null) {
            throw new NullPointerException("actionTable");
        }
        fkActions = actionTable;
        fkAction = action;
        pose();
    }

    public ActionTable getActionTable() {
        return fkActions;
    }

    public void setTime(int time) {
        super.setTime(time);
        pose();
    }

    private void pose() {
        if (fkActions != null) {
            FkMesh.pose(fkFigure, fkActions.fkTable, fkAction, getTime());
        }
    }

    public int getNumPatterns() {
        return FkMesh.numPatterns(fkFigure);
    }

    public void setPattern(int pattern) {
        FkMesh.setPattern(fkFigure, pattern);
    }

    public void setPerspectiveCorrectionEnabled(boolean enabled) {
    }

    public void setBlendMode(int blendMode) {
        fkBlend = blendMode;
    }

    public void setTransparency(float transparency) {
        /* games give 0..1 or a percentage */
        float t = transparency > 1f ? transparency / 100f : transparency;
        fkAlpha = t < 0 ? 0 : t > 1 ? 1 : t;
    }
}
