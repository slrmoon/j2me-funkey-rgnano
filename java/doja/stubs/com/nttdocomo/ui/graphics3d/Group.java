package com.nttdocomo.ui.graphics3d;

import com.nttdocomo.ui.util3d.Transform;
import java.util.Vector;

/** Objects drawn together, under one more transform. */
public class Group extends DrawableObject3D {
    final Vector fkElements = new Vector();
    final Transform fkTransform = new Transform();

    public Group() {
        super(TYPE_GROUP);
    }

    public void dispose() {
        fkElements.removeAllElements();
        super.dispose();
    }

    public int getNumElements() {
        return fkElements.size();
    }

    public Object3D getElement(int index) {
        if (index < 0 || index >= fkElements.size()) {
            throw new ArrayIndexOutOfBoundsException(index);
        }
        return (Object3D) fkElements.elementAt(index);
    }

    public void removeElement(int index) {
        if (index < 0 || index >= fkElements.size()) {
            throw new ArrayIndexOutOfBoundsException(index);
        }
        fkElements.removeElementAt(index);
    }

    public void addElement(Object3D object) {
        if (object == null) {
            throw new NullPointerException("object");
        }
        fkElements.addElement(object);
    }

    public void setTransform(Transform transform) {
        if (transform == null) {
            throw new NullPointerException("transform");
        }
        fkTransform.set(transform);
    }

    public void getTransform(Transform transform) {
        if (transform == null) {
            throw new NullPointerException("transform");
        }
        transform.set(fkTransform);
    }

    public void setTime(int time) {
        super.setTime(time);
        for (int i = 0; i < fkElements.size(); i++) {
            ((Object3D) fkElements.elementAt(i)).setTime(time);
        }
    }

    public void setPerspectiveCorrectionEnabled(boolean enabled) {
    }

    public void setBlendMode(int blendMode) {
        for (int i = 0; i < fkElements.size(); i++) {
            Object o = fkElements.elementAt(i);
            if (o instanceof DrawableObject3D) {
                ((DrawableObject3D) o).setBlendMode(blendMode);
            }
        }
    }

    public void setTransparency(float transparency) {
        for (int i = 0; i < fkElements.size(); i++) {
            Object o = fkElements.elementAt(i);
            if (o instanceof DrawableObject3D) {
                ((DrawableObject3D) o).setTransparency(transparency);
            }
        }
    }
}
