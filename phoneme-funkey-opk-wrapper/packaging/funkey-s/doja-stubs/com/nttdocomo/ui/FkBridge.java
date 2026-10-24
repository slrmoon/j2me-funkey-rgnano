package com.nttdocomo.ui;


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

    public static void snapshot(Frame f) {
        f.fkSnapshot();
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

    /** binds a Graphics subclass created outside this package to a canvas surface */
    public static void initOn(Graphics g, Canvas c) {
        g.init(c.surface(), c, null);
    }

    /** true when the canvas surface was replaced since g was bound */
    public static boolean stale(Graphics g, Canvas c) {
        return g.target != c.surface();
    }

    public static com.funkey.doja.gl.FkGL glOf(Graphics g) {
        return gl(g);
    }

    /** the GL state of the surface a Graphics draws on (kept across getGraphics) */
    static com.funkey.doja.gl.FkGL gl(final Graphics g) {
        com.funkey.doja.gl.FkGL gl = null;
        if (g.canvas != null) {
            gl = (com.funkey.doja.gl.FkGL) g.canvas.fkgl;
            if (gl == null || gl.target() != g.target) {
                final Canvas c = g.canvas;
                gl = new com.funkey.doja.gl.FkGL(g.target);
                gl.setChangeListener(new Runnable() {
                    public void run() {
                        c.drawn();
                    }
                });
                c.fkgl = gl;
            }
        } else if (g.image != null) {
            gl = (com.funkey.doja.gl.FkGL) g.image.fkgl;
            if (gl == null || gl.target() != g.target) {
                final Image im = g.image;
                gl = new com.funkey.doja.gl.FkGL(g.target);
                gl.setChangeListener(new Runnable() {
                    public void run() {
                        im.version++;
                    }
                });
                im.fkgl = gl;
            }
        } else {
            gl = (com.funkey.doja.gl.FkGL) g.r3dgl;
            if (gl == null) {
                gl = new com.funkey.doja.gl.FkGL(g.target);
                g.r3dgl = gl;
            }
        }
        return gl;
    }

    static com.nttdocomo.opt.ui.j3d.FkRender render3d(final Graphics g) {
        com.nttdocomo.opt.ui.j3d.FkRender r = (com.nttdocomo.opt.ui.j3d.FkRender) g.r3d;
        if (r == null) {
            r = new com.nttdocomo.opt.ui.j3d.FkRender(g.target, new Runnable() {
                public void run() {
                    g.changed();
                }
            });
            g.r3d = r;
        }
        return r;
    }

    /** MIDP image behind a DoJa image, as drawn (transparency applied). */
    /** sets the pixels of a runtime-made image (paletted, transparent) */
    public static void setPixels(Image img, javax.microedition.lcdui.Image pixels) {
        img.img = pixels;
        img.mutable = false;
        img.disposed = false;
        img.touched();
    }

    public static boolean isDisposed(Image img) {
        return img.disposed;
    }

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
