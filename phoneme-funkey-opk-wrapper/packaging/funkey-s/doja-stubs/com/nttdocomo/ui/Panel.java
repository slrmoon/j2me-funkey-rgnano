package com.nttdocomo.ui;

public class Panel extends Frame {
    private ComponentListener componentListener;
    private KeyListener keyListener;

    public void setLayoutManager(LayoutManager manager) {
    }

    public void setComponentListener(ComponentListener listener) {
        componentListener = listener;
    }

    public void setKeyListener(KeyListener listener) {
        keyListener = listener;
    }

    public void setSoftKeyListener(SoftKeyListener listener) {
        super.setSoftKeyListener(listener);
    }

    protected void keyPressed(int keyCode) {
        super.keyPressed(keyCode);
        int dojaKey = Display.mapMIDPKeyCode(keyCode);
        if (keyListener != null) {
            keyListener.keyPressed(this, dojaKey);
        }
        if (componentListener != null && dojaKey == Display.KEY_SELECT) {
            componentListener.componentAction(null, ComponentListener.BUTTON_PRESSED, 0);
        }
    }

    protected void keyReleased(int keyCode) {
        super.keyReleased(keyCode);
        if (keyListener != null) {
            keyListener.keyReleased(this, Display.mapMIDPKeyCode(keyCode));
        }
    }
}
