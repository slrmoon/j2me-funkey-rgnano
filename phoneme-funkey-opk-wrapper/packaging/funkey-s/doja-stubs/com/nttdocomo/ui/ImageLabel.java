package com.nttdocomo.ui;

public final class ImageLabel extends Component {
    private Image image;

    public ImageLabel() {
    }

    public ImageLabel(Image img) {
        image = img;
    }

    public void setImage(Image img) {
        image = img;
        changed();
    }

    int prefWidth() {
        return image == null ? 0 : image.getWidth();
    }

    int prefHeight() {
        return image == null ? 0 : image.getHeight();
    }

    void paint(javax.microedition.lcdui.Graphics g, int px, int py, boolean focused) {
        if (image != null && !image.disposed) {
            g.drawImage(image.drawable(), px, py, Graphics.TL);
        }
    }
}
