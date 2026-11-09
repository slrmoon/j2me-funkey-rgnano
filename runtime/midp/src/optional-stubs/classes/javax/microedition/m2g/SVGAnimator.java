package javax.microedition.m2g;

/** FunKey: no SVG engine, so there are no images to animate. */
public abstract class SVGAnimator {
    protected SVGAnimator() {
    }

    public static SVGAnimator createAnimator(SVGImage svgImage) {
        if (svgImage == null) {
            throw new NullPointerException();
        }
        throw new IllegalArgumentException("SVG (JSR 226) not available");
    }

    public static SVGAnimator createAnimator(SVGImage svgImage, String componentBaseClass) {
        return createAnimator(svgImage);
    }

    public abstract void setSVGEventListener(SVGEventListener svgEventListener);

    public abstract void setTimeIncrement(float timeIncrement);

    public abstract float getTimeIncrement();

    public abstract void play();

    public abstract void pause();

    public abstract void stop();

    public abstract Object getTargetComponent();

    public abstract void invokeAndWait(Runnable runnable) throws InterruptedException;

    public abstract void invokeLater(Runnable runnable);
}
