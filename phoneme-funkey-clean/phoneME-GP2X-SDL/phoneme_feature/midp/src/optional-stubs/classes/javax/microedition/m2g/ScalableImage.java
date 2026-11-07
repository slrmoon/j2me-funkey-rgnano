package javax.microedition.m2g;

import java.io.IOException;
import java.io.InputStream;

/**
 * JSR 226. FunKey: there is no SVG engine: creating an image throws
 * IOException as for a document that cannot be loaded.
 */
public abstract class ScalableImage {
    protected ScalableImage() {
    }

    public static ScalableImage createImage(InputStream stream, ExternalResourceHandler handler)
            throws IOException {
        if (stream == null) {
            throw new NullPointerException();
        }
        throw new IOException("SVG (JSR 226) not available");
    }

    public static ScalableImage createImage(String url, ExternalResourceHandler handler)
            throws IOException {
        if (url == null) {
            throw new NullPointerException();
        }
        throw new IOException("SVG (JSR 226) not available");
    }

    public abstract int getViewportWidth();

    public abstract int getViewportHeight();

    public abstract void setViewportWidth(int width);

    public abstract void setViewportHeight(int height);

    public abstract void requestCompleted(String URI, InputStream resourceData) throws IOException;
}
