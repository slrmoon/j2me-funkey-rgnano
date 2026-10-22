package com.funkey.doja;

import javax.microedition.lcdui.Canvas;
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

    protected void keyPressed(int keyCode) {
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
