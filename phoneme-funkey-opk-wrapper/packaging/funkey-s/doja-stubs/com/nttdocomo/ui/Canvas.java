package com.nttdocomo.ui;

public class Canvas extends Frame {
    public static final int KEY_PRESSED_EVENT = Display.KEY_PRESSED_EVENT;
    public static final int KEY_RELEASED_EVENT = Display.KEY_RELEASED_EVENT;
    public static final int KEY_UP = Display.KEY_UP;
    public static final int KEY_DOWN = Display.KEY_DOWN;
    public static final int KEY_LEFT = Display.KEY_LEFT;
    public static final int KEY_RIGHT = Display.KEY_RIGHT;
    public static final int KEY_SELECT = Display.KEY_SELECT;
    public static final int KEY_SOFT1 = Display.KEY_SOFT1;
    public static final int KEY_SOFT2 = Display.KEY_SOFT2;

    private javax.microedition.lcdui.Image backBuffer;
    private Graphics backGraphics;
    private int backgroundColor = 0xffffff;

    protected void paint(javax.microedition.lcdui.Graphics g) {
        if (backBuffer != null) {
            g.drawImage(backBuffer, 0, 0,
                    javax.microedition.lcdui.Graphics.LEFT | javax.microedition.lcdui.Graphics.TOP);
            return;
        }
        paint(new Graphics(g));
    }

    public void paint(Graphics g) {
    }

    public Graphics getGraphics() {
        ensureBackBuffer();
        return backGraphics;
    }

    public void setBackground(int rgb) {
        backgroundColor = rgb;
        ensureBackBuffer();
        int old = backGraphics.getColor();
        backGraphics.setColor(rgb);
        backGraphics.fillRect(0, 0, getWidth(), getHeight());
        backGraphics.setColor(old);
        repaint();
    }

    protected void keyPressed(int keyCode) {
        super.keyPressed(keyCode);
        int dojaKey = Display.mapMIDPKeyCode(keyCode);
        System.out.println("DoJa key pressed raw=" + keyCode + " mapped=" + dojaKey);
        processEvent(Display.KEY_PRESSED_EVENT, dojaKey);
    }

    protected void keyReleased(int keyCode) {
        super.keyReleased(keyCode);
        int dojaKey = Display.mapMIDPKeyCode(keyCode);
        System.out.println("DoJa key released raw=" + keyCode + " mapped=" + dojaKey);
        processEvent(Display.KEY_RELEASED_EVENT, dojaKey);
    }

    protected void keyRepeated(int keyCode) {
        int dojaKey = Display.mapMIDPKeyCode(keyCode);
        System.out.println("DoJa key repeated raw=" + keyCode + " mapped=" + dojaKey);
        processEvent(Display.KEY_PRESSED_EVENT, dojaKey);
    }

    public void processEvent(int type, int param) {
    }

    public void imeOn(String text, int mode, int max) {
    }

    private void ensureBackBuffer() {
        if (backBuffer == null) {
            backBuffer = javax.microedition.lcdui.Image.createImage(getWidth(), getHeight());
            backGraphics = new Graphics(backBuffer.getGraphics(), this);
            int old = backGraphics.getColor();
            backGraphics.setColor(backgroundColor);
            backGraphics.fillRect(0, 0, getWidth(), getHeight());
            backGraphics.setColor(old);
        }
    }

    javax.microedition.lcdui.Graphics backBufferGraphics() {
        ensureBackBuffer();
        return backBuffer.getGraphics();
    }
}
