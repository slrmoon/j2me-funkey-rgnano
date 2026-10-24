package com.nttdocomo.opt.ui.j3d2;

import com.nttdocomo.ui.Canvas;
import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.Graphics;

/** A canvas whose paint gets the C3D graphics of its surface. */
public abstract class C3DCanvas extends Canvas {
    private C3DGraphics c3d;

    protected C3DCanvas() {
    }

    public final C3DGraphics getC3DGraphics() {
        if (c3d == null || FkBridge.stale(c3d, this)) {
            c3d = new C3DGraphics(this);
        }
        return c3d;
    }

    public abstract void paint(C3DGraphics graphics);

    public final void paint(Graphics graphics) {
        paint(getC3DGraphics());
    }
}
