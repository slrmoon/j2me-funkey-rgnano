import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Graphics;
import javax.microedition.midlet.MIDlet;

public final class HelloMidlet extends MIDlet {
    private Display display;
    private PlaceholderCanvas canvas;

    protected void startApp() {
        if (display == null) {
            display = Display.getDisplay(this);
            canvas = new PlaceholderCanvas();
        }
        display.setCurrent(canvas);
    }

    protected void pauseApp() {
    }

    protected void destroyApp(boolean unconditional) {
    }

    private final class PlaceholderCanvas extends Canvas {
        protected void paint(Graphics g) {
            int w = getWidth();
            int h = getHeight();

            g.setColor(0x101820);
            g.fillRect(0, 0, w, h);

            g.setColor(0x00FF88);
            g.drawString("phoneME FunKey", w / 2, h / 2 - 10,
                    Graphics.TOP | Graphics.HCENTER);

            g.setColor(0xFFFFFF);
            g.drawString("Use native launcher", w / 2, h / 2 + 8,
                    Graphics.TOP | Graphics.HCENTER);
        }
    }
}
