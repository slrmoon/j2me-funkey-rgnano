package com.nttdocomo.ui;

import com.nttdocomo.lang.XString;

public final class Label extends Component {
    public static final int LEFT = 0;
    public static final int CENTER = 1;
    public static final int RIGHT = 2;

    String text = "";
    int alignment = LEFT;

    public Label() {
    }

    public Label(XString t) {
        this(t == null ? null : t.toString());
    }

    public Label(String t) {
        text = t == null ? "" : t;
    }

    public Label(XString t, int align) {
        this(t == null ? null : t.toString(), align);
    }

    public Label(String t, int align) {
        this(t);
        setAlignment(align);
    }

    public void setText(XString t) {
        setText(t == null ? null : t.toString());
    }

    public void setText(String t) {
        text = t == null ? "" : t;
        changed();
    }

    public void setAlignment(int align) {
        if (align < LEFT || align > RIGHT) {
            throw new IllegalArgumentException("alignment");
        }
        alignment = align;
        changed();
    }

    public void setFont(Font f) {
        super.setFont(f);
    }

    int prefWidth() {
        return font().stringWidth(text) + 2;
    }

    int prefHeight() {
        return font().getHeight() + 2;
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        int cw = getWidth();
        if (background >= 0) {
            g.setColor(background);
            g.fillRect(px, py, cw, getHeight());
        }
        int tw = font().stringWidth(text);
        int tx = px + 1;
        if (alignment == CENTER) {
            tx = px + (cw - tw) / 2;
        } else if (alignment == RIGHT) {
            tx = px + cw - tw - 1;
        }
        text(g, font(), text, tx, py + 1, foreground);
    }
}
