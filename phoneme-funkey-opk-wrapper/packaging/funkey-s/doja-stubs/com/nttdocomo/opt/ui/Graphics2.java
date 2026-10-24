package com.nttdocomo.opt.ui;

import com.nttdocomo.opt.ui.j3d.AffineTrans;
import com.nttdocomo.ui.FkBridge2;
import com.nttdocomo.ui.Graphics;
import com.nttdocomo.ui.Image;
import com.nttdocomo.ui.MediaImage;

/**
 * Optional graphics extensions; every Graphics handed out by the runtime
 * is a Graphics2.
 */
public class Graphics2 extends Graphics {
    public static final int CM_NORMAL = 0;
    public static final int CM_ZOOM = 256;
    public static final int OP_REPL = 0;
    public static final int OP_ADD = 1;
    public static final int OP_SUB = 2;

    private long lastSync;

    public Graphics2() {
    }

    public void setOrigin(int x, int y) {
        super.setOrigin(x, y);
    }

    public void setCoordinateMode(int mode) {
    }

    public void setRenderMode(int operator, int srcRatio, int dstRatio) {
        if (operator < OP_REPL || operator > OP_SUB) {
            throw new IllegalArgumentException("operator");
        }
        FkBridge2.setRenderMode(this, operator, srcRatio, dstRatio);
    }

    public void drawSpriteSet(SpriteSet sprites) {
        if (sprites == null) {
            throw new NullPointerException("sprites");
        }
        drawSpriteSet(sprites, 0, sprites.getCount());
    }

    /** each visible sprite with its own flip and render mode */
    public void drawSpriteSet(SpriteSet sprites, int offset, int count) {
        if (sprites == null) {
            throw new NullPointerException("sprites");
        }
        if (offset < 0 || count < 0 || offset + count > sprites.getCount()) {
            throw new ArrayIndexOutOfBoundsException();
        }
        int op = FkBridge2.renderOp(this);
        int src = FkBridge2.renderSrc(this);
        int dst = FkBridge2.renderDst(this);
        int flip = FkBridge2.flipMode(this);
        try {
            for (int i = offset; i < offset + count; i++) {
                Sprite s = sprites.getSprite(i);
                if (s == null || s.image == null) {
                    throw new NullPointerException("sprite");
                }
                if (!s.visible) {
                    continue;
                }
                super.setFlipMode(s.flipMode);
                FkBridge2.setRenderMode(this, s.op, s.srcRatio, s.dstRatio);
                drawImage(s.image, s.x, s.y, s.sx, s.sy, s.width, s.height);
            }
        } finally {
            super.setFlipMode(flip);
            FkBridge2.setRenderMode(this, op, src, dst);
        }
    }

    public Image getImage(int x, int y, int width, int height) {
        return getImageRegion(x, y, width, height);
    }

    public void drawNthImage(MediaImage image, int k, int x, int y) {
        drawImage(image.getImage(), x, y);
    }

    public static int getIntermediateColor(int c1, int c2, int ratio) {
        if (ratio < 0) {
            ratio = 0;
        } else if (ratio > 255) {
            ratio = 255;
        }
        int r = (((c1 >> 16) & 255) * (255 - ratio) + ((c2 >> 16) & 255) * ratio) / 255;
        int g = (((c1 >> 8) & 255) * (255 - ratio) + ((c2 >> 8) & 255) * ratio) / 255;
        int b = ((c1 & 255) * (255 - ratio) + (c2 & 255) * ratio) / 255;
        return (r << 16) | (g << 8) | b;
    }

    public void drawImage(Image image, AffineTrans at, int x, int y, int width, int height) {
        drawImage(image, toMatrix(at), x, y, width, height);
    }

    public void drawImage(Image image, AffineTrans at) {
        drawImage(image, toMatrix(at));
    }

    private static int[] toMatrix(AffineTrans at) {
        /* AffineTrans uses 4096 = 1.0, the same as the int[] matrix */
        return new int[] { at.m00, at.m01, at.m03, at.m10, at.m11, at.m13 };
    }

    public int getSyncUnlockInterval() {
        return 0;
    }

    public int syncUnlock(int interval) {
        long now = System.currentTimeMillis();
        long wait = lastSync + interval - now;
        if (interval > 0 && wait > 0 && wait <= interval) {
            try {
                Thread.sleep(wait);
            } catch (InterruptedException ie) {
            }
            now = System.currentTimeMillis();
        }
        int elapsed = lastSync == 0 ? interval : (int) (now - lastSync);
        lastSync = now;
        unlock(true);
        return elapsed;
    }
}
