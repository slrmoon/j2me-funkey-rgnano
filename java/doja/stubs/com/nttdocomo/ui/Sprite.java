package com.nttdocomo.ui;

public class Sprite {
    Image image;
    int x;
    int y;
    int sx;
    int sy;
    int width;
    int height;
    boolean visible = true;
    int flipMode;

    protected Sprite() {
    }

    public Sprite(Image img) {
        setImage(img);
    }

    public Sprite(Image img, int px, int py, int w, int h) {
        setImage(img, px, py, w, h);
    }

    public void setLocation(int px, int py) {
        x = px;
        y = py;
    }

    public void setImage(Image img) {
        if (img == null) {
            throw new NullPointerException("image");
        }
        image = img;
        sx = 0;
        sy = 0;
        width = img.getWidth();
        height = img.getHeight();
    }

    public void setImage(Image img, int px, int py, int w, int h) {
        if (img == null) {
            throw new NullPointerException("image");
        }
        image = img;
        sx = px;
        sy = py;
        width = w;
        height = h;
    }

    public void setVisible(boolean v) {
        visible = v;
    }

    public boolean isVisible() {
        return visible;
    }

    public void setFlipMode(int mode) {
        if (mode < 0 || mode > 7) {
            throw new IllegalArgumentException("flipMode");
        }
        flipMode = mode;
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public void setRotation(int[] rotation) {
    }
}
