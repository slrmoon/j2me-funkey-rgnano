package com.nttdocomo.opt.ui;

import com.nttdocomo.ui.Graphics;
import com.nttdocomo.ui.Image;

/** A part of an image placed on screen, drawn by Graphics2.drawSpriteSet. */
public class Sprite {
    Image image;
    int sx;
    int sy;
    int width;
    int height;
    int x;
    int y;
    boolean visible = true;
    int flipMode = Graphics.FLIP_NONE;
    int op = Graphics2.OP_REPL;
    int srcRatio = 255;
    int dstRatio = 255;

    protected Sprite() {
    }

    public Sprite(Image image) {
        setImage(image);
    }

    public Sprite(Image image, int x, int y, int width, int height) {
        setImage(image, x, y, width, height);
    }

    public void setLocation(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public void setImage(Image image) {
        if (image == null) {
            throw new NullPointerException("image");
        }
        setImage(image, 0, 0, image.getWidth(), image.getHeight());
    }

    public void setImage(Image image, int x, int y, int width, int height) {
        if (image == null) {
            throw new NullPointerException("image");
        }
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("size");
        }
        this.image = image;
        sx = x;
        sy = y;
        this.width = width;
        this.height = height;
    }

    public void setVisible(boolean b) {
        visible = b;
    }

    public boolean isVisible() {
        return visible;
    }

    public void setFlipMode(int mode) {
        flipMode = mode;
    }

    public void setRenderMode(int operator, int src, int dst) {
        if (operator < Graphics2.OP_REPL || operator > Graphics2.OP_SUB) {
            throw new IllegalArgumentException("operator");
        }
        if (src < 0 || src > 255 || dst < 0 || dst > 255) {
            throw new IllegalArgumentException("ratio");
        }
        op = operator;
        srcRatio = src;
        dstRatio = dst;
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
}
