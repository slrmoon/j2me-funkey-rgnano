package javax.microedition.m2g;

/** FunKey: binds and releases like the handset; there is nothing to render. */
public class ScalableGraphics {
    public static final int RENDERING_QUALITY_LOW = 1;
    public static final int RENDERING_QUALITY_HIGH = 2;

    private Object target;

    private ScalableGraphics() {
    }

    public static ScalableGraphics createInstance() {
        return new ScalableGraphics();
    }

    public void bindTarget(Object target) {
        if (target == null) {
            throw new NullPointerException();
        }
        if (!(target instanceof javax.microedition.lcdui.Graphics)) {
            throw new IllegalArgumentException();
        }
        if (this.target != null) {
            throw new IllegalStateException();
        }
        this.target = target;
    }

    public void releaseTarget() {
        if (target == null) {
            throw new IllegalStateException();
        }
        target = null;
    }

    public void render(int x, int y, ScalableImage image) {
        if (image == null) {
            throw new NullPointerException();
        }
        if (target == null) {
            throw new IllegalStateException();
        }
    }

    public void setRenderingQuality(int mode) {
        if (mode != RENDERING_QUALITY_LOW && mode != RENDERING_QUALITY_HIGH) {
            throw new IllegalArgumentException();
        }
    }

    public void setTransparency(float alpha) {
        if (alpha < 0f || alpha > 1f) {
            throw new IllegalArgumentException();
        }
    }
}
