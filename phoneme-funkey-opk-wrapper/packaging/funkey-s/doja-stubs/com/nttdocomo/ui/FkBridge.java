package com.nttdocomo.ui;

import com.funkey.doja.Render3D;

/**
 * Entry points of com.nttdocomo.ui used by the runtime package
 * (com.funkey.doja). Not part of the DoJa API.
 */
public final class FkBridge {
    private FkBridge() {
    }

    public static void hostPaint(Frame f, javax.microedition.lcdui.Graphics g) {
        f.fkPaint(g);
        Dialog d = Dialog.showing;
        if (d != null) {
            d.paintOverlay(g);
        }
    }

    public static void dispatchKey(Frame f, int type, int key) {
        Dialog d = Dialog.showing;
        if (d != null) {
            d.fkKey(type, key);
            return;
        }
        f.fkKey(type, key);
    }

    public static void dispatchPaint(Frame f) {
        f.fkRepaint();
    }

    static Graphics allocGraphics() {
        return new com.nttdocomo.opt.ui.Graphics2();
    }

    static Graphics newGraphics(Image img) {
        Graphics g = allocGraphics();
        g.init(img.raw(), null, img);
        return g;
    }

    static Graphics newGraphics(Canvas c) {
        Graphics g = allocGraphics();
        g.init(c.surface(), c, null);
        return g;
    }

    static Render3D render3d(Graphics g) {
        Render3D r = (Render3D) g.r3d;
        if (r == null) {
            r = new Render3D(g.target, g);
            g.r3d = r;
        }
        return r;
    }

    /** MIDP image behind a DoJa image, as drawn (transparency applied). */
    public static javax.microedition.lcdui.Image drawableOf(Image img) {
        return img.drawable();
    }

    public static javax.microedition.lcdui.Image rawOf(Image img) {
        return img.raw();
    }

    public static Image wrap(javax.microedition.lcdui.Image img, boolean mutable) {
        return new FkImage(img, mutable);
    }

    public static javax.microedition.lcdui.Graphics midpGraphics(Graphics g) {
        return g.g;
    }

    public static int originX(Graphics g) {
        return g.originX;
    }

    public static int originY(Graphics g) {
        return g.originY;
    }

    public static void changed(Graphics g) {
        g.changed();
    }
}
