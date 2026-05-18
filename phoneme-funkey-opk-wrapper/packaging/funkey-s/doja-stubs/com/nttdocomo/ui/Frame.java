package com.nttdocomo.ui;

public class Frame extends javax.microedition.lcdui.Canvas {
    private SoftKeyListener softKeyListener;

    public int getWidth() {
        return super.getWidth();
    }

    public int getHeight() {
        return super.getHeight();
    }

    public void setSoftLabel(int key, String label) {
    }

    public void setSoftKeyListener(SoftKeyListener listener) {
        softKeyListener = listener;
    }

    protected void paint(javax.microedition.lcdui.Graphics g) {
    }

    protected void keyPressed(int keyCode) {
        if (softKeyListener != null) {
            int dojaKey = Display.mapMIDPKeyCode(keyCode);
            if (dojaKey == Display.KEY_SOFT1) {
                softKeyListener.softKeyPressed(0);
            } else if (dojaKey == Display.KEY_SOFT2) {
                softKeyListener.softKeyPressed(1);
            }
        }
    }

    protected void keyReleased(int keyCode) {
        if (softKeyListener != null) {
            int dojaKey = Display.mapMIDPKeyCode(keyCode);
            if (dojaKey == Display.KEY_SOFT1) {
                softKeyListener.softKeyReleased(0);
            } else if (dojaKey == Display.KEY_SOFT2) {
                softKeyListener.softKeyReleased(1);
            }
        }
    }
}
