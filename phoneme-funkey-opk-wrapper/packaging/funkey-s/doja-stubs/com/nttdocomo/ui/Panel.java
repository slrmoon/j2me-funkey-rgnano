package com.nttdocomo.ui;

import java.util.Vector;

import com.funkey.doja.DoJaRuntime;

/**
 * High-level UI frame: components laid out top to bottom, left to right.
 */
public class Panel extends Frame {
    private final Vector comps = new Vector();
    private ComponentListener componentListener;
    private KeyListener keyListener;
    private SoftKeyListener softKeyListener;
    private LayoutManager layoutManager;
    private boolean layoutSet;
    private FocusManager focusManager;
    private String title;
    boolean explicitLayout;
    private int focus = -1;
    private int scrollY;
    private java.util.Timer tickTimer;

    public Panel() {
    }

    public void add(Component c) {
        if (c == null) {
            throw new NullPointerException("component");
        }
        if (c.panel != null) {
            throw new IllegalArgumentException("component already added");
        }
        c.panel = this;
        comps.addElement(c);
        if (focus < 0 && c.focusable()) {
            focus = comps.size() - 1;
        }
        changed();
    }

    public FocusManager getFocusManager() {
        return focusManager;
    }

    public void setFocusManager(FocusManager fm) {
        focusManager = fm;
    }

    public void setBackground(int color) {
        super.setBackground(color);
        changed();
    }

    public void setComponentListener(ComponentListener l) {
        componentListener = l;
    }

    public void setKeyListener(KeyListener l) {
        keyListener = l;
    }

    public void setLayoutManager(LayoutManager lm) {
        layoutManager = lm;
        layoutSet = true;
        changed();
    }

    public void setSoftLabelVisible(boolean visible) {
        super.setSoftLabelVisible(visible);
        changed();
    }

    public void setSoftKeyListener(SoftKeyListener l) {
        softKeyListener = l;
    }

    public void processSoftKeyEvent(int type, int key) {
    }

    public void setTitle(String t) {
        title = t;
        changed();
    }

    public void setSoftLabel(int key, String caption) {
        super.setSoftLabel(key, caption);
        changed();
    }

    /* ------------------------------------------------------------------ */

    void changed() {
        if (Display.getCurrent() == this) {
            DoJaRuntime.frameChanged();
        }
    }

    void fire(Component c, int type, int param) {
        if (componentListener != null) {
            componentListener.componentAction(c, type, param);
        }
    }

    void focus(Component c) {
        int i = comps.indexOf(c);
        if (i >= 0 && c.focusable()) {
            focus = i;
            changed();
        }
    }

    private int titleHeight() {
        return title == null ? 0 : Font.getDefaultFont().getHeight() + 4;
    }

    private int softHeight() {
        if (!softLabelVisible || (softLabels[0] == null && softLabels[1] == null)) {
            return 0;
        }
        return Font.getDefaultFont().getHeight() + 4;
    }

    private void layout() {
        if (explicitLayout || (layoutSet && layoutManager == null)) {
            return;
        }
        int width = getWidth();
        int cx = 2;
        int cy = 2;
        int lineH = 0;
        for (int i = 0; i < comps.size(); i++) {
            Component c = (Component) comps.elementAt(i);
            if (!c.visible) {
                continue;
            }
            int cw = c.getWidth();
            int ch = c.getHeight();
            if (cx > 2 && cx + cw > width - 2) {
                cx = 2;
                cy += lineH + 2;
                lineH = 0;
            }
            c.x = cx;
            c.y = cy;
            cx += cw + 2;
            if (ch > lineH) {
                lineH = ch;
            }
        }
    }

    void fkShown() {
        boolean anim = false;
        for (int i = 0; i < comps.size(); i++) {
            if (comps.elementAt(i) instanceof Ticker) {
                anim = true;
            }
        }
        if (anim && tickTimer == null) {
            tickTimer = new java.util.Timer();
            tickTimer.schedule(new java.util.TimerTask() {
                public void run() {
                    if (Display.getCurrent() == Panel.this) {
                        DoJaRuntime.frameChanged();
                    } else {
                        cancel();
                        tickTimer = null;
                    }
                }
            }, 80, 80);
        }
        DoJaRuntime.frameChanged();
    }

    void fkPaint(javax.microedition.lcdui.Graphics g) {
        int width = getWidth();
        int height = getHeight();
        g.setColor(background & 0xffffff);
        g.fillRect(0, 0, width, height);
        layout();
        int th = titleHeight();
        int sh = softHeight();
        int areaH = height - th - sh;
        /* keep the focused component visible */
        if (focus >= 0 && focus < comps.size()) {
            Component f = (Component) comps.elementAt(focus);
            if (f.y - scrollY < 0) {
                scrollY = f.y;
            } else if (f.y + f.getHeight() - scrollY > areaH) {
                scrollY = f.y + f.getHeight() - areaH;
            }
        }
        if (scrollY < 0) {
            scrollY = 0;
        }
        g.setClip(0, th, width, areaH);
        for (int i = 0; i < comps.size(); i++) {
            Component c = (Component) comps.elementAt(i);
            if (!c.visible) {
                continue;
            }
            c.paint(g, c.x, c.y + th - scrollY, i == focus);
        }
        g.setClip(0, 0, width, height);
        Font df = Font.getDefaultFont();
        if (th > 0) {
            g.setColor(0x203060);
            g.fillRect(0, 0, width, th);
            Component.text(g, df, title, 3, 2, 0xffffff);
        }
        if (sh > 0) {
            paintSoftLabels(g, width, height - sh, sh, softLabels);
        }
    }

    static void paintSoftLabels(javax.microedition.lcdui.Graphics g, int width, int y, int h,
            String[] labels) {
        Font df = Font.getDefaultFont();
        g.setColor(0x202020);
        g.fillRect(0, y, width, h);
        if (labels[0] != null) {
            Component.text(g, df, labels[0], 3, y + 2, 0xffffff);
        }
        if (labels[1] != null) {
            Component.text(g, df, labels[1], width - 3 - df.stringWidth(labels[1]), y + 2, 0xffffff);
        }
    }

    void fkKey(int type, int key) {
        if (key == Display.KEY_SOFT1 || key == Display.KEY_SOFT2) {
            int sk = key == Display.KEY_SOFT1 ? SOFT_KEY_1 : SOFT_KEY_2;
            processSoftKeyEvent(type, sk);
            if (softKeyListener != null) {
                if (type == Display.KEY_PRESSED_EVENT) {
                    softKeyListener.softKeyPressed(sk);
                } else {
                    softKeyListener.softKeyReleased(sk);
                }
            }
            return;
        }
        boolean used = false;
        if (focus >= 0 && focus < comps.size()) {
            used = ((Component) comps.elementAt(focus)).key(type, key);
        }
        if (!used && type == Display.KEY_PRESSED_EVENT
                && (key == Display.KEY_UP || key == Display.KEY_DOWN)) {
            int dir = key == Display.KEY_DOWN ? 1 : -1;
            int i = focus;
            for (int n = 0; n < comps.size(); n++) {
                i += dir;
                if (i < 0 || i >= comps.size()) {
                    break;
                }
                if (((Component) comps.elementAt(i)).focusable()) {
                    focus = i;
                    changed();
                    used = true;
                    break;
                }
            }
            if (!used) {
                /* nothing focusable that way: scroll */
                scrollY += dir * (Font.getDefaultFont().getHeight() + 2);
                changed();
            }
        }
        if (keyListener != null) {
            if (type == Display.KEY_PRESSED_EVENT) {
                keyListener.keyPressed(this, key);
            } else {
                keyListener.keyReleased(this, key);
            }
        }
        changed();
    }
}
