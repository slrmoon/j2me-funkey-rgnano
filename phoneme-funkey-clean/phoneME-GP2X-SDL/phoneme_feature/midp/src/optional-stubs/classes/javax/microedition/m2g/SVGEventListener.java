package javax.microedition.m2g;

public interface SVGEventListener {
    void keyPressed(int keyCode);

    void keyReleased(int keyCode);

    void pointerPressed(int x, int y);

    void pointerReleased(int x, int y);

    void hideNotify();

    void showNotify();

    void sizeChanged(int width, int height);
}
