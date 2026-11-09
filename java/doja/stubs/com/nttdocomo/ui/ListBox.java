package com.nttdocomo.ui;

import java.util.Vector;

import com.nttdocomo.lang.XString;

public final class ListBox extends Component implements Interactable {
    public static final int SINGLE_SELECT = 0;
    public static final int RADIO_BUTTON = 1;
    public static final int CHECK_BOX = 2;
    public static final int NUMBERED_LIST = 3;
    public static final int MULTIPLE_SELECT = 4;
    public static final int CHOICE = 5;

    private final int type;
    private final int rows;
    private final Vector items = new Vector();
    private boolean[] selected = new boolean[0];
    private int cursor;
    private int top;

    public ListBox(int t) {
        this(t, 0);
    }

    public ListBox(int t, int visibleRows) {
        if (t < SINGLE_SELECT || t > CHOICE) {
            throw new IllegalArgumentException("type");
        }
        if (visibleRows < 0) {
            throw new IllegalArgumentException("rows");
        }
        type = t;
        rows = visibleRows;
    }

    public void append(XString item) {
        append(item == null ? null : item.toString());
    }

    public void append(String item) {
        items.addElement(item == null ? "" : item);
        boolean[] n = new boolean[items.size()];
        System.arraycopy(selected, 0, n, 0, selected.length);
        selected = n;
        if (items.size() == 1 && (type == SINGLE_SELECT || type == RADIO_BUTTON
                || type == CHOICE || type == NUMBERED_LIST)) {
            selected[0] = true;
        }
        changed();
    }

    public void setItems(String[] list) {
        removeAll();
        if (list != null) {
            for (int i = 0; i < list.length; i++) {
                append(list[i]);
            }
        }
    }

    public void setItems(XString[] list) {
        removeAll();
        if (list != null) {
            for (int i = 0; i < list.length; i++) {
                append(list[i]);
            }
        }
    }

    public void deselect(int index) {
        check(index);
        selected[index] = false;
        changed();
    }

    public String getItem(int index) {
        check(index);
        return (String) items.elementAt(index);
    }

    public XString getXItem(int index) {
        return new XString(getItem(index));
    }

    public int getItemCount() {
        return items.size();
    }

    public int getSelectedIndex() {
        for (int i = 0; i < selected.length; i++) {
            if (selected[i]) {
                return i;
            }
        }
        return -1;
    }

    public boolean isIndexSelected(int index) {
        check(index);
        return selected[index];
    }

    public void removeAll() {
        items.removeAllElements();
        selected = new boolean[0];
        cursor = 0;
        top = 0;
        changed();
    }

    public void select(int index) {
        check(index);
        if (!multi()) {
            for (int i = 0; i < selected.length; i++) {
                selected[i] = false;
            }
            cursor = index;
        }
        selected[index] = true;
        changed();
    }

    public void setFont(Font f) {
        super.setFont(f);
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

    public void setLocation(int px, int py) {
        super.setLocation(px, py);
    }

    /* ------------------------------------------------------------------ */

    private void check(int index) {
        if (index < 0 || index >= items.size()) {
            throw new ArrayIndexOutOfBoundsException(index);
        }
    }

    private boolean multi() {
        return type == CHECK_BOX || type == MULTIPLE_SELECT;
    }

    private int visibleRows() {
        if (type == CHOICE) {
            return 1;
        }
        int n = rows > 0 ? rows : items.size();
        return n <= 0 ? 1 : n;
    }

    boolean focusable() {
        return visible && enabled;
    }

    int prefWidth() {
        int max = 0;
        Font f = font();
        for (int i = 0; i < items.size(); i++) {
            int iw = f.stringWidth(label(i));
            if (iw > max) {
                max = iw;
            }
        }
        int dw = Display.getWidth() - 4;
        int pw = max + f.getHeight() + 8;
        return pw > dw ? dw : pw;
    }

    int prefHeight() {
        return visibleRows() * (font().getHeight() + 2) + 2;
    }

    private String label(int i) {
        String s = (String) items.elementAt(i);
        if (type == NUMBERED_LIST) {
            return (i + 1) + ". " + s;
        }
        return s;
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        int cw = getWidth();
        int ch = getHeight();
        Font f = font();
        int rh = f.getHeight() + 2;
        g.setColor(background >= 0 ? background : 0xffffff);
        g.fillRect(px, py, cw, ch);
        g.setColor(focused ? 0x3060c0 : 0x808080);
        g.drawRect(px, py, cw - 1, ch - 1);
        if (type == CHOICE) {
            int sel = getSelectedIndex();
            String s = sel >= 0 ? label(sel) : "";
            text(g, f, "<", px + 2, py + 1, foreground);
            text(g, f, s, px + 2 + f.stringWidth("< "), py + 1, foreground);
            text(g, f, ">", px + cw - 2 - f.stringWidth(">"), py + 1, foreground);
            return;
        }
        int vr = visibleRows();
        if (cursor < top) {
            top = cursor;
        }
        if (cursor >= top + vr) {
            top = cursor - vr + 1;
        }
        for (int r = 0; r < vr && top + r < items.size(); r++) {
            int i = top + r;
            int ry = py + 1 + r * rh;
            if (focused && i == cursor) {
                g.setColor(0x3060c0);
                g.fillRect(px + 1, ry, cw - 2, rh);
            }
            int fg = focused && i == cursor ? 0xffffff : foreground;
            int tx = px + 3;
            if (type == RADIO_BUTTON || type == CHECK_BOX || type == MULTIPLE_SELECT) {
                int box = f.getHeight() - 4;
                g.setColor(fg);
                if (type == RADIO_BUTTON) {
                    g.drawArc(tx, ry + 2, box, box, 0, 360);
                    if (selected[i]) {
                        g.fillArc(tx + 2, ry + 4, box - 4, box - 4, 0, 360);
                    }
                } else {
                    g.drawRect(tx, ry + 2, box, box);
                    if (selected[i]) {
                        g.fillRect(tx + 2, ry + 4, box - 3, box - 3);
                    }
                }
                tx += box + 4;
            } else if (selected[i] && type == SINGLE_SELECT && !focused) {
                g.setColor(0xc0c0ff);
                g.fillRect(px + 1, ry, cw - 2, rh);
            }
            text(g, f, label(i), tx, ry + 1, fg);
        }
    }

    boolean key(int t, int key) {
        if (t != Display.KEY_PRESSED_EVENT || items.size() == 0) {
            return false;
        }
        if (type == CHOICE) {
            int sel = getSelectedIndex();
            if (key == Display.KEY_LEFT || key == Display.KEY_RIGHT) {
                int n = items.size();
                sel = (sel < 0 ? 0 : sel) + (key == Display.KEY_RIGHT ? 1 : n - 1);
                select(sel % n);
                if (panel != null) {
                    panel.fire(this, ComponentListener.SELECTION_CHANGED, sel % n);
                }
                return true;
            }
            return false;
        }
        if (key == Display.KEY_UP && cursor > 0) {
            cursor--;
            if (!multi()) {
                select(cursor);
            }
            changed();
            return true;
        }
        if (key == Display.KEY_DOWN && cursor < items.size() - 1) {
            cursor++;
            if (!multi()) {
                select(cursor);
            }
            changed();
            return true;
        }
        if (key == Display.KEY_SELECT) {
            if (multi()) {
                selected[cursor] = !selected[cursor];
                changed();
            } else {
                select(cursor);
            }
            if (panel != null) {
                panel.fire(this, ComponentListener.SELECTION_CHANGED, cursor);
            }
            return true;
        }
        return false;
    }
}
