package com.funkey.doja;

import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;

import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.Frame;

/**
 * The only MIDP Displayable of a DoJa application. It shows the current
 * com.nttdocomo.ui.Frame and turns MIDP key callbacks into DoJa key events.
 */
public final class DoJaHost extends Canvas {
    private boolean shownOnce;

    DoJaHost() {
        setFullScreenMode(true);
    }

    protected void paint(Graphics g) {
        if (DoJaRuntime.TRACE) {
            System.out.println("DoJa trace: host paint " + getWidth() + "x" + getHeight());
        }
        String[] err = DoJaRuntime.fatalText();
        if (err != null) {
            paintError(g, err);
            return;
        }
        try {
            paintFrame(g);
        } catch (Throwable t) {
            System.out.println("DoJa: host paint failed: " + t);
            t.printStackTrace();
        }
        if (DoJaRuntime.TRACE) {
            System.out.println("DoJa trace: host paint done");
        }
    }

    private void paintFrame(Graphics g) {
        int w = getWidth();
        int h = getHeight();
        int dw = DoJaRuntime.drawWidth();
        int dh = DoJaRuntime.drawHeight();
        int ox = dw > 0 && dw < w ? (w - dw) / 2 : 0;
        int oy = dh > 0 && dh < h ? (h - dh) / 2 : 0;
        if (ox > 0 || oy > 0) {
            g.setColor(0);
            g.fillRect(0, 0, w, h);
        }
        Frame f = com.nttdocomo.ui.Display.getCurrent();
        if (f == null) {
            if (ox == 0 && oy == 0) {
                g.setColor(0);
                g.fillRect(0, 0, w, h);
            }
            return;
        }
        g.translate(ox, oy);
        g.setClip(0, 0, dw > 0 ? dw : w, dh > 0 ? dh : h);
        FkBridge.hostPaint(f, g);
    }

    private void paintError(Graphics g, String[] lines) {
        int w = getWidth();
        int h = getHeight();
        g.setClip(0, 0, w, h);
        g.setColor(0x400000);
        g.fillRect(0, 0, w, h);
        Font f = Font.getFont(Font.FACE_SYSTEM, Font.STYLE_PLAIN, Font.SIZE_SMALL);
        g.setFont(f);
        int y = 8;
        int lh = f.getHeight() + 1;
        for (int i = 0; i < lines.length && y < h; i++) {
            g.setColor(i == 0 ? 0xff6060 : 0xffffff);
            String s = lines[i];
            /* wrap long lines (class names, messages) */
            while (s.length() > 0 && y < h) {
                int n = s.length();
                while (n > 1 && f.substringWidth(s, 0, n) > w - 8) {
                    n--;
                }
                g.drawString(s.substring(0, n), 4, y, Graphics.TOP | Graphics.LEFT);
                s = s.substring(n);
                y += lh;
            }
            if (lines[i].length() == 0) {
                y += lh / 2;
            }
        }
    }

    protected void keyPressed(int keyCode) {
        if (DoJaRuntime.fatalText() != null) {
            DoJaRuntime.terminate();
            return;
        }
        DoJaRuntime.keyEvent(mapKey(keyCode), true);
    }

    protected void keyReleased(int keyCode) {
        DoJaRuntime.keyEvent(mapKey(keyCode), false);
    }

    protected void keyRepeated(int keyCode) {
        /* DoJa has no auto-repeat events: games poll getKeypadState() */
    }

    protected void showNotify() {
        if (shownOnce) {
            DoJaRuntime.postResume();
        }
        shownOnce = true;
    }

    protected void hideNotify() {
        DoJaRuntime.releaseAllKeys();
    }

    /** MIDP key code (FunKey key profile "doja") to DoJa key code. */
    static int mapKey(int keyCode) {
        switch (keyCode) {
        case -1:
            return 0x11; /* KEY_UP */
        case -2:
            return 0x13; /* KEY_DOWN */
        case -3:
            return 0x10; /* KEY_LEFT */
        case -4:
            return 0x12; /* KEY_RIGHT */
        case -5:
            return 0x14; /* KEY_SELECT */
        case -6:
            return 0x15; /* KEY_SOFT1 */
        case -7:
            return 0x16; /* KEY_SOFT2 */
        case -8:
            return 0x20; /* KEY_CLEAR */
        case 42:
            return 0x0a; /* KEY_ASTERISK */
        case 35:
            return 0x0b; /* KEY_POUND */
        default:
            if (keyCode >= '0' && keyCode <= '9') {
                return keyCode - '0';
            }
            return -1;
        }
    }
}
