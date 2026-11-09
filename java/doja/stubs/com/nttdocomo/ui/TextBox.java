package com.nttdocomo.ui;

import com.nttdocomo.lang.XString;

/**
 * Text field. There is no text entry on the handheld: the text is shown
 * and can only be changed by the application.
 */
public final class TextBox extends Component implements Interactable {
    public static final int ALPHA = 1;
    public static final int KANA = 2;
    public static final int NUMBER = 0;
    public static final int DISPLAY_ANY = 0;
    public static final int DISPLAY_PASSWORD = 1;
    public static final int INPUTSIZE_UNLIMITED = 0;

    private String text;
    private final int columns;
    private final int rows;
    private final int displayMode;
    private boolean editable = true;

    public TextBox(XString t, int cols, int rws, int mode) {
        this(t == null ? null : t.toString(), cols, rws, mode);
    }

    public TextBox(String t, int cols, int rws, int mode) {
        if (cols < 0 || rws < 0) {
            throw new IllegalArgumentException("size");
        }
        text = t == null ? "" : t;
        columns = cols <= 0 ? 10 : cols;
        rows = rws <= 0 ? 1 : rws;
        displayMode = mode;
    }

    public String getText() {
        return text;
    }

    public XString getXText() {
        return new XString(text);
    }

    public void setText(XString t) {
        setText(t == null ? null : t.toString());
    }

    public void setText(String t) {
        text = t == null ? "" : t;
        changed();
    }

    public void setEditable(boolean e) {
        editable = e;
    }

    public void setInputMode(int mode) {
    }

    public void setInputSize(int size) {
    }

    public void setEnabled(boolean e) {
        enabledChanged(e);
    }

    public void requestFocus() {
        focusRequested();
    }

    public void setVisible(boolean v) {
        super.setVisible(v);
    }

    public void setFont(Font f) {
        super.setFont(f);
    }

    public void setLocation(int px, int py) {
        super.setLocation(px, py);
    }

    boolean focusable() {
        return visible && enabled;
    }

    int prefWidth() {
        int pw = columns * (font().getHeight() / 2) + 6;
        int dw = Display.getWidth() - 4;
        return pw > dw ? dw : pw;
    }

    int prefHeight() {
        return rows * font().getHeight() + 4;
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        int cw = getWidth();
        int ch = getHeight();
        g.setColor(background >= 0 ? background : 0xffffff);
        g.fillRect(px, py, cw, ch);
        g.setColor(focused ? 0x3060c0 : 0x808080);
        g.drawRect(px, py, cw - 1, ch - 1);
        String s = text;
        if (displayMode == DISPLAY_PASSWORD) {
            StringBuffer b = new StringBuffer();
            for (int i = 0; i < s.length(); i++) {
                b.append('*');
            }
            s = b.toString();
        }
        Font f = font();
        int lineH = f.getHeight();
        int pos = 0;
        for (int r = 0; r < rows && pos < s.length(); r++) {
            int end = f.getLineBreak(s, pos, s.length() - pos, cw - 6);
            if (end <= pos) {
                end = pos + 1;
            }
            text(g, f, s.substring(pos, end), px + 3, py + 2 + r * lineH, enabled ? foreground : 0x808080);
            pos = end;
        }
    }
}
