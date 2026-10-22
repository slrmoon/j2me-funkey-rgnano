package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;

/**
 * Low-level drawing frame. The canvas owns an off-screen surface the size
 * of the draw area; paint() and getGraphics() draw into it and the host
 * canvas shows it after paint() returns or when a lock is released.
 */
public abstract class Canvas extends Frame {
    public static final int IME_COMMITTED = 0;
    public static final int IME_CANCELED = 1;

    private javax.microedition.lcdui.Image surface;
    int lockCount;
    private boolean inPaint;
    private boolean flushInPaint;
    private volatile boolean repaintQueued;
    private boolean everPainted;
    /* the last presented frame: the host paints it while the game already
     * draws the next one into the surface */
    private javax.microedition.lcdui.Image front;
    private javax.microedition.lcdui.Graphics frontG;
    private final Object frontLock = new Object();

    public Canvas() {
    }

    javax.microedition.lcdui.Image surface() {
        if (surface == null) {
            int w = Display.getWidth();
            int h = Display.getHeight();
            surface = javax.microedition.lcdui.Image.createImage(w, h);
            javax.microedition.lcdui.Graphics g = surface.getGraphics();
            g.setColor(background & 0xffffff);
            g.fillRect(0, 0, w, h);
        }
        return surface;
    }

    public Graphics getGraphics() {
        return FkBridge.newGraphics(this);
    }

    public abstract void paint(Graphics g);

    public void repaint() {
        if (!repaintQueued) {
            repaintQueued = true;
            DoJaRuntime.postRepaint(this);
        }
    }

    public void repaint(int x, int y, int width, int height) {
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("size");
        }
        if (width == 0 || height == 0) {
            return;
        }
        repaint();
    }

    public void processEvent(int type, int param) {
    }

    public int getKeypadState() {
        return DoJaRuntime.keypadState();
    }

    public int getKeypadState(int group) {
        if (group < 0) {
            throw new IllegalArgumentException("group");
        }
        return DoJaRuntime.keypadState(group);
    }

    public void imeOn(String text, int displayMode, int inputMode) {
        imeOn(text, displayMode, inputMode, 0);
    }

    public void imeOn(final String text, int displayMode, int inputMode, int inputSize) {
        /* no text input on the handheld: hand back the initial text */
        DoJaRuntime.postRun(new Runnable() {
            public void run() {
                if (text != null && text.length() > 0) {
                    processIMEEvent(IME_COMMITTED, text);
                } else {
                    processIMEEvent(IME_CANCELED, null);
                }
            }
        });
    }

    public void processIMEEvent(int type, String text) {
    }

    public void setBackground(int color) {
        super.setBackground(color);
    }

    /* ------------------------------------------------------------------ */

    void fkPaint(javax.microedition.lcdui.Graphics g) {
        synchronized (frontLock) {
            g.drawImage(front != null ? front : surface(), 0, 0, Graphics.TL);
        }
    }

    void fkSnapshot() {
        javax.microedition.lcdui.Image s = surface();
        synchronized (frontLock) {
            if (front == null) {
                front = javax.microedition.lcdui.Image.createImage(s.getWidth(), s.getHeight());
                frontG = front.getGraphics();
            }
            frontG.drawImage(s, 0, 0, Graphics.TL);
        }
    }

    void fkKey(int type, int key) {
        processEvent(type, key);
    }

    void fkShown() {
        if (!everPainted) {
            repaint();
        } else {
            DoJaRuntime.present();
        }
    }

    void fkRepaint() {
        repaintQueued = false;
        everPainted = true;
        Graphics g = FkBridge.newGraphics(this);
        inPaint = true;
        flushInPaint = false;
        try {
            paint(g);
        } finally {
            inPaint = false;
        }
        if (Display.getCurrent() == this) {
            DoJaRuntime.present();
        }
    }

    /** unlock() reached zero or was forced. */
    void flushFromGraphics() {
        everPainted = true;
        if (inPaint) {
            flushInPaint = true;
            return;
        }
        if (Display.getCurrent() == this) {
            DoJaRuntime.present();
        }
    }

    /** Something was drawn into the surface. */
    void drawn() {
        if (lockCount == 0 && !inPaint && Display.getCurrent() == this) {
            DoJaRuntime.requestAutoPresent();
        }
    }
}
