package com.nttdocomo.ui;

import com.nttdocomo.lang.XString;

public final class Button extends Component implements Interactable {
    String label = "";

    public Button() {
    }

    public Button(String l) {
        label = l == null ? "" : l;
    }

    public Button(XString l) {
        this(l == null ? null : l.toString());
    }

    public void setLabel(String l) {
        label = l == null ? "" : l;
        changed();
    }

    public void setLabel(XString l) {
        setLabel(l == null ? null : l.toString());
    }

    public void setEnabled(boolean e) {
        enabledChanged(e);
    }

    public void requestFocus() {
        focusRequested();
    }

    public void setFont(Font f) {
        super.setFont(f);
    }

    boolean focusable() {
        return visible && enabled;
    }

    int prefWidth() {
        return font().stringWidth(label) + 10;
    }

    int prefHeight() {
        return font().getHeight() + 6;
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        int cw = getWidth();
        int ch = getHeight();
        g.setColor(focused ? 0x3060c0 : (background >= 0 ? background : 0xe0e0e0));
        g.fillRect(px, py, cw, ch);
        g.setColor(0x404040);
        g.drawRect(px, py, cw - 1, ch - 1);
        int tw = font().stringWidth(label);
        text(g, font(), label, px + (cw - tw) / 2, py + 3,
                focused ? 0xffffff : (enabled ? foreground : 0x808080));
    }

    boolean key(int type, int key) {
        if (type == Display.KEY_PRESSED_EVENT && key == Display.KEY_SELECT) {
            if (panel != null) {
                panel.fire(this, ComponentListener.BUTTON_PRESSED, 0);
            }
            return true;
        }
        return false;
    }
}
