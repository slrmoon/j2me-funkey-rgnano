package com.nttdocomo.ui;

import com.nttdocomo.lang.XString;

public final class Ticker extends Component {
    private String text = "";
    private final long start = System.currentTimeMillis();

    public Ticker() {
    }

    public Ticker(String t) {
        text = t == null ? "" : t;
    }

    public Ticker(XString t) {
        this(t == null ? null : t.toString());
    }

    public void setText(String t) {
        text = t == null ? "" : t;
        changed();
    }

    public void setText(XString t) {
        setText(t == null ? null : t.toString());
    }

    public void setFont(Font f) {
        super.setFont(f);
    }

    int prefWidth() {
        return Display.getWidth();
    }

    int prefHeight() {
        return font().getHeight() + 2;
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        int cw = getWidth();
        int ch = getHeight();
        if (background >= 0) {
            g.setColor(background);
            g.fillRect(px, py, cw, ch);
        }
        int tw = font().stringWidth(text);
        int span = tw + cw;
        int off = (int) ((System.currentTimeMillis() - start) / 40 % (span <= 0 ? 1 : span));
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cwid = g.getClipWidth();
        int chei = g.getClipHeight();
        g.clipRect(px, py, cw, ch);
        text(g, font(), text, px + cw - off, py + 1, foreground);
        g.setClip(cx, cy, cwid, chei);
    }

    boolean animated() {
        return true;
    }
}
