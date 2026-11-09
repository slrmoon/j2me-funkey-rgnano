package com.mascotcapsule.micro3d.v3;

/**
 * Orientation, position and projection of a figure on the screen. The
 * last of setScale, setParallelSize and setPerspective takes effect.
 */
public class FigureLayout {
    static final int SCALE = 0;
    static final int PARALLEL_SIZE = 1;
    static final int PERSPECTIVE_FOV = 2;
    static final int PERSPECTIVE_WH = 3;

    AffineTrans trans;
    AffineTrans[] transArray;
    int transIndex;
    int mode = SCALE;
    int scaleX;
    int scaleY;
    int parallelW;
    int parallelH;
    int centerX;
    int centerY;
    int near;
    int far;
    int angle;
    int perspW;
    int perspH;

    /** no transformation, parallel projection, scale 512 */
    public FigureLayout() {
        this(null, 512, 512, 0, 0);
    }

    public FigureLayout(AffineTrans trans, int sx, int sy, int cx, int cy) {
        setAffineTrans(trans);
        setScale(sx, sy);
        setCenter(cx, cy);
    }

    /** the active transformation; null for none */
    public final AffineTrans getAffineTrans() {
        if (transArray != null) {
            return transArray[transIndex];
        }
        return trans;
    }

    public final void setAffineTrans(AffineTrans trans) {
        this.trans = trans;
        transArray = null;
        transIndex = 0;
    }

    public final void setAffineTrans(AffineTrans[] trans) {
        if (trans == null) {
            throw new NullPointerException();
        }
        for (int i = 0; i < trans.length; i++) {
            if (trans[i] == null) {
                throw new NullPointerException();
            }
        }
        transArray = trans;
        transIndex = 0;
    }

    /** @deprecated */
    public final void setAffineTransArray(AffineTrans[] trans) {
        setAffineTrans(trans);
    }

    public final void selectAffineTrans(int idx) {
        if (transArray == null || idx < 0 || idx >= transArray.length) {
            throw new IllegalArgumentException("idx");
        }
        transIndex = idx;
    }

    public final int getScaleX() {
        return scaleX;
    }

    public final int getScaleY() {
        return scaleY;
    }

    /** parallel projection, 4096 = one pixel per model unit */
    public final void setScale(int sx, int sy) {
        scaleX = sx;
        scaleY = sy;
        mode = SCALE;
    }

    public final int getParallelWidth() {
        return parallelW;
    }

    public final int getParallelHeight() {
        return parallelH;
    }

    /** parallel projection of w x h model units onto the screen */
    public final void setParallelSize(int w, int h) {
        if (w < 0 || h < 0) {
            throw new IllegalArgumentException();
        }
        parallelW = w;
        parallelH = h;
        mode = PARALLEL_SIZE;
    }

    public final int getCenterX() {
        return centerX;
    }

    public final int getCenterY() {
        return centerY;
    }

    public final void setCenter(int cx, int cy) {
        centerX = cx;
        centerY = cy;
    }

    private static void checkNearFar(int zNear, int zFar) {
        if (zNear >= zFar || zNear < 1 || zNear >= 32767 || zFar < 2 || zFar >= 32768) {
            throw new IllegalArgumentException("near/far");
        }
    }

    /** angle of view: 4096 = 360 degrees, 1..2047 */
    public final void setPerspective(int zNear, int zFar, int angle) {
        checkNearFar(zNear, zFar);
        if (angle <= 0 || angle >= 2048) {
            throw new IllegalArgumentException("angle");
        }
        near = zNear;
        far = zFar;
        this.angle = angle;
        mode = PERSPECTIVE_FOV;
    }

    /** size of the near clipping plane, view units x 4096 */
    public final void setPerspective(int zNear, int zFar, int width, int height) {
        checkNearFar(zNear, zFar);
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("size");
        }
        near = zNear;
        far = zFar;
        perspW = width;
        perspH = height;
        mode = PERSPECTIVE_WH;
    }
}
