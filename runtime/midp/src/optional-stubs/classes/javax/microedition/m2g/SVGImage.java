package javax.microedition.m2g;

import java.io.IOException;
import java.io.InputStream;

import org.w3c.dom.Document;
import org.w3c.dom.svg.SVGElement;

/** FunKey: no SVG engine (see ScalableImage). */
public abstract class SVGImage extends ScalableImage {
    protected SVGImage() {
    }

    public static ScalableImage createImage(InputStream stream, ExternalResourceHandler handler)
            throws IOException {
        return ScalableImage.createImage(stream, handler);
    }

    public static SVGImage createEmptyImage(ExternalResourceHandler handler) {
        throw new IllegalStateException("SVG (JSR 226) not available");
    }

    public abstract Document getDocument();

    public abstract void activate();

    public abstract void dispatchMouseEvent(String type, int x, int y);

    public abstract void focusOn(SVGElement element);

    public abstract void incrementTime(float seconds);
}
