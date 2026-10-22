package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;

/**
 * Modal message box. show() blocks until a button is chosen.
 */
public final class Dialog extends Frame {
    public static final int DIALOG_INFO = 0;
    public static final int DIALOG_WARNING = 1;
    public static final int DIALOG_ERROR = 2;
    public static final int DIALOG_YESNO = 3;
    public static final int DIALOG_YESNOCANCEL = 4;
    public static final int BUTTON_OK = 1;
    public static final int BUTTON_CANCEL = 2;
    public static final int BUTTON_YES = 4;
    public static final int BUTTON_NO = 8;

    static volatile Dialog showing;

    private final int type;
    private final String title;
    private String text = "";
    private Font font;
    private int sel;
    private volatile int result;
    private final int[] buttons;
    private final String[] names;

    public Dialog(int t, String ttl) {
        if (t < DIALOG_INFO || t > DIALOG_YESNOCANCEL) {
            throw new IllegalArgumentException("type");
        }
        type = t;
        title = ttl;
        background = 0xf0f0f0;
        if (t == DIALOG_YESNO) {
            buttons = new int[] { BUTTON_YES, BUTTON_NO };
            names = new String[] { "Yes", "No" };
        } else if (t == DIALOG_YESNOCANCEL) {
            buttons = new int[] { BUTTON_YES, BUTTON_NO, BUTTON_CANCEL };
            names = new String[] { "Yes", "No", "Cancel" };
        } else {
            buttons = new int[] { BUTTON_OK };
            names = new String[] { "OK" };
        }
    }

    public void setBackground(int color) {
        super.setBackground(color);
    }

    public void setFont(Font f) {
        font = f;
    }

    public void setText(String t) {
        text = t == null ? "" : t;
    }

    public void setSoftLabel(int key, String caption) {
        super.setSoftLabel(key, caption);
    }

    public void setSoftLabelVisible(boolean visible) {
        super.setSoftLabelVisible(visible);
    }

    public int show() {
        result = 0;
        sel = 0;
        showing = this;
        DoJaRuntime.releaseKeysForModal();
        DoJaRuntime.frameChanged();
        if (DoJaRuntime.isEventThread()) {
            DoJaRuntime.pumpUntil(new DoJaRuntime.ModalCondition() {
                public boolean done() {
                    return result != 0;
                }
            });
        } else {
            synchronized (this) {
                while (result == 0 && !DoJaRuntime.isTerminated()) {
                    try {
                        wait(100);
                    } catch (InterruptedException ie) {
                        break;
                    }
                }
            }
        }
        if (showing == this) {
            showing = null;
        }
        DoJaRuntime.frameChanged();
        return result == 0 ? buttons[buttons.length - 1] : result;
    }

    void fkPaint(javax.microedition.lcdui.Graphics g) {
        paintOverlay(g);
    }

    void paintOverlay(javax.microedition.lcdui.Graphics g) {
        int width = Display.getWidth();
        int height = Display.getHeight();
        Font f = font != null ? font : Font.getDefaultFont();
        int lh = f.getHeight() + 2;
        int boxW = width - 16;
        int inner = boxW - 8;
        /* wrap text */
        java.util.Vector lines = new java.util.Vector();
        int pos = 0;
        String s = text;
        while (pos < s.length()) {
            int nl = s.indexOf('\n', pos);
            int lineEnd = nl < 0 ? s.length() : nl;
            int end = f.getLineBreak(s, pos, lineEnd - pos, inner);
            if (end <= pos) {
                end = lineEnd > pos ? pos + 1 : pos;
            }
            lines.addElement(s.substring(pos, end));
            pos = end;
            if (pos == nl) {
                pos++;
            }
            if (lines.size() > 30) {
                break;
            }
        }
        int titleH = title != null ? lh + 2 : 0;
        int boxH = titleH + lines.size() * lh + lh + 16;
        if (boxH > height - 8) {
            boxH = height - 8;
        }
        int bx = 8;
        int by = (height - boxH) / 2;
        g.setColor(background & 0xffffff);
        g.fillRect(bx, by, boxW, boxH);
        g.setColor(0x202020);
        g.drawRect(bx, by, boxW - 1, boxH - 1);
        int y = by + 3;
        if (title != null) {
            g.setColor(0x203060);
            g.fillRect(bx + 1, by + 1, boxW - 2, lh + 1);
            Component.text(g, f, title, bx + 4, y, 0xffffff);
            y += titleH;
        }
        for (int i = 0; i < lines.size(); i++) {
            if (y + lh > by + boxH - lh - 8) {
                break;
            }
            Component.text(g, f, (String) lines.elementAt(i), bx + 4, y, 0x000000);
            y += lh;
        }
        /* buttons */
        int n = buttons.length;
        int bw = (boxW - 8 - (n - 1) * 4) / n;
        int byb = by + boxH - lh - 6;
        for (int i = 0; i < n; i++) {
            int x = bx + 4 + i * (bw + 4);
            g.setColor(i == sel ? 0x3060c0 : 0xd0d0d0);
            g.fillRect(x, byb, bw, lh + 2);
            g.setColor(0x404040);
            g.drawRect(x, byb, bw - 1, lh + 1);
            int tw = f.stringWidth(names[i]);
            Component.text(g, f, names[i], x + (bw - tw) / 2, byb + 2, i == sel ? 0xffffff : 0);
        }
    }

    void fkKey(int t, int key) {
        if (t != Display.KEY_PRESSED_EVENT) {
            return;
        }
        int n = buttons.length;
        if (key == Display.KEY_LEFT || key == Display.KEY_UP) {
            sel = (sel + n - 1) % n;
            DoJaRuntime.frameChanged();
        } else if (key == Display.KEY_RIGHT || key == Display.KEY_DOWN) {
            sel = (sel + 1) % n;
            DoJaRuntime.frameChanged();
        } else if (key == Display.KEY_SELECT) {
            finish(buttons[sel]);
        } else if (key == Display.KEY_SOFT1) {
            finish(buttons[0]);
        } else if (key == Display.KEY_SOFT2 || key == Display.KEY_CLEAR) {
            finish(buttons[n - 1]);
        }
    }

    private void finish(int r) {
        synchronized (this) {
            result = r;
            notifyAll();
        }
        if (showing == this) {
            showing = null;
        }
        DoJaRuntime.frameChanged();
    }
}
