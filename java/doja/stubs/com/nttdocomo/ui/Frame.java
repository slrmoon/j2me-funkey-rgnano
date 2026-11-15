package com.nttdocomo.ui;

/**
 * Base of Canvas, Panel and Dialog. Frames are not MIDP displayables: the
 * runtime host canvas shows the current frame.
 */
public abstract class Frame {
    public static final int SOFT_KEY_1 = 0;
    public static final int SOFT_KEY_2 = 1;

    int background = 0xffffff;
    String[] softLabels = new String[2];
    boolean softLabelVisible = true;

    public Frame() {
    }

    public final int getWidth() {
        return Display.getWidth();
    }

    public final int getHeight() {
        return Display.getHeight();
    }

    public void setBackground(int color) {
        background = color;
    }

    public void setSoftLabel(int key, String caption) {
        if (key != SOFT_KEY_1 && key != SOFT_KEY_2) {
            throw new IllegalArgumentException("key");
        }
        softLabels[key] = caption;
        fkSoftChanged();
    }

    public void setSoftLabelVisible(boolean visible) {
        softLabelVisible = visible;
        fkSoftChanged();
    }

    /**
     * A Canvas has its soft key labels outside the drawing area (the
     * handset's own bar under the screen): the launcher is told the labels
     * of the current frame. A Panel draws its own.
     */
    void fkSoftChanged() {
        if (Display.getCurrent() != this) {
            return;
        }
        boolean show = softLabelVisible && this instanceof Canvas;
        com.funkey.doja.DoJaRuntime.softLabels(show ? softLabels[0] : null,
                show ? softLabels[1] : null);
    }

    public String getSoftLabel(int key) {
        if (key != SOFT_KEY_1 && key != SOFT_KEY_2) {
            throw new IllegalArgumentException("key");
        }
        return softLabels[key];
    }

    public boolean isSoftLabelVisible() {
        return softLabelVisible;
    }

    public void processSoftKeyEvent(int type, int key) {
    }

    /* ------------------------------------------------------------------ */
    /* runtime hooks                                                       */

    /** Paints the frame onto the host canvas (MIDP event thread). */
    abstract void fkPaint(javax.microedition.lcdui.Graphics g);

    /** Key event on the DoJa event thread; type 0 pressed, 1 released. */
    abstract void fkKey(int type, int key);

    /** Called on the DoJa event thread when the frame becomes current. */
    /** A finished frame: copy what the host should show (Canvas). */
    void fkSnapshot() {
    }

    void fkShown() {
    }

    /** Called on the DoJa event thread for a queued repaint. */
    void fkRepaint() {
    }
}
