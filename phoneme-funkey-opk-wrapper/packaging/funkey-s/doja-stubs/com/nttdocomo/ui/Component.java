package com.nttdocomo.ui;

/**
 * High-level UI component placed on a Panel.
 */
public abstract class Component {
    int x;
    int y;
    int w = -1;
    int h = -1;
    boolean visible = true;
    boolean enabled = true;
    int background = -1;
    int foreground = 0x000000;
    Font font;
    Panel panel;
    int htmlBreaks;   /* HTMLLayout: line breaks before this component */
    int htmlAlign;    /* HTMLLayout: 0 left, 1 center, 2 right */

    public Component() {
    }

    public final int getHeight() {
        return h >= 0 ? h : prefHeight();
    }

    public final int getWidth() {
        return w >= 0 ? w : prefWidth();
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public void setBackground(int color) {
        background = color;
        changed();
    }

    public void setForeground(int color) {
        foreground = color;
        changed();
    }

    public void setLocation(int px, int py) {
        x = px;
        y = py;
        if (panel != null) {
            panel.explicitLayout = true;
        }
        changed();
    }

    public void setSize(int width, int height) {
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("size");
        }
        w = width;
        h = height;
        changed();
    }

    public void setFont(Font f) {
        font = f;
        changed();
    }

    public void setVisible(boolean v) {
        visible = v;
        changed();
    }

    /* ------------------------------------------------------------------ */

    Font font() {
        return font != null ? font : Font.getDefaultFont();
    }

    boolean focusable() {
        return false;
    }

    void changed() {
        if (panel != null) {
            panel.changed();
        }
    }

    int prefWidth() {
        return 0;
    }

    int prefHeight() {
        return 0;
    }

    /** Paints at (px, py) in host coordinates. */
    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
    }

    /** Returns true when the key was used. */
    boolean key(int type, int key) {
        return false;
    }

    void enabledChanged(boolean e) {
        enabled = e;
        changed();
    }

    void focusRequested() {
        if (panel != null) {
            panel.focus(this);
        }
    }

    static void text(javax.microedition.lcdui.Graphics g, Font f, String s, int px, int top, int rgb) {
        if (s != null && s.length() > 0) {
            f.draw(g, s, px, top + f.getAscent(), rgb);
        }
    }
}
