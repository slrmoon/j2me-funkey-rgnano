package com.nttdocomo.ui;

public class ImageMap {
    private int windowX;
    private int windowY;
    private int windowWidth;
    private int windowHeight;
    private int cellWidth = 16;
    private int cellHeight = 16;
    private int mapWidth;
    private int mapHeight;
    private int[] mapData = new int[0];
    private Image[] images = new Image[0];
    private boolean concat;

    protected ImageMap() {
    }

    public ImageMap(int cw, int ch) {
        if (cw <= 0 || ch <= 0) {
            throw new IllegalArgumentException("cell size");
        }
        cellWidth = cw;
        cellHeight = ch;
    }

    public ImageMap(int cw, int ch, int mw, int mh, byte[] data, Image[] imgs) {
        this(cw, ch);
        setImageMap(mw, mh, data, imgs);
    }

    public ImageMap(int cw, int ch, int mw, int mh, int[] data, Image[] imgs) {
        this(cw, ch);
        setImageMap(mw, mh, data, imgs, false);
    }

    public ImageMap(int cw, int ch, int mw, int mh, int[] data, Image[] imgs, boolean c) {
        this(cw, ch);
        setImageMap(mw, mh, data, imgs, c);
    }

    public void setImageMap(int mw, int mh, byte[] data, Image[] imgs) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        int[] d = new int[data.length];
        for (int i = 0; i < d.length; i++) {
            d[i] = data[i] & 255;
        }
        setImageMap(mw, mh, d, imgs, false);
    }

    public void setImageMap(int mw, int mh, int[] data, Image[] imgs) {
        setImageMap(mw, mh, data, imgs, false);
    }

    public void setImageMap(int mw, int mh, int[] data, Image[] imgs, boolean c) {
        if (data == null || imgs == null) {
            throw new NullPointerException();
        }
        if (mw < 0 || mh < 0 || data.length < mw * mh) {
            throw new IllegalArgumentException("map size");
        }
        mapWidth = mw;
        mapHeight = mh;
        mapData = data;
        images = imgs;
        concat = c;
        windowX = 0;
        windowY = 0;
        windowWidth = mw;
        windowHeight = mh;
    }

    public void setWindow(int x, int y, int w, int h) {
        if (w < 0 || h < 0) {
            throw new IllegalArgumentException("window");
        }
        windowX = x;
        windowY = y;
        windowWidth = w;
        windowHeight = h;
    }

    public void setWindowLocation(int x, int y) {
        windowX = x;
        windowY = y;
    }

    public void moveWindowLocation(int dx, int dy) {
        windowX += dx;
        windowY += dy;
    }

    void draw(Graphics g, int dx, int dy) {
        for (int row = 0; row < windowHeight; row++) {
            int my = windowY + row;
            if (my < 0 || my >= mapHeight) {
                continue;
            }
            for (int col = 0; col < windowWidth; col++) {
                int mx = windowX + col;
                if (mx < 0 || mx >= mapWidth) {
                    continue;
                }
                int idx = mapData[my * mapWidth + mx];
                if (idx < 0) {
                    continue;
                }
                int px = dx + col * cellWidth;
                int py = dy + row * cellHeight;
                if (concat) {
                    int rem = idx;
                    for (int i = 0; i < images.length; i++) {
                        Image im = images[i];
                        if (im == null) {
                            continue;
                        }
                        int across = im.getWidth() / cellWidth;
                        int down = im.getHeight() / cellHeight;
                        int n = across * down;
                        if (n <= 0) {
                            continue;
                        }
                        if (rem < n) {
                            g.drawImage(im, px, py, (rem % across) * cellWidth,
                                    (rem / across) * cellHeight, cellWidth, cellHeight);
                            break;
                        }
                        rem -= n;
                    }
                } else if (idx < images.length && images[idx] != null) {
                    Image im = images[idx];
                    int w = cellWidth < im.getWidth() ? cellWidth : im.getWidth();
                    int h = cellHeight < im.getHeight() ? cellHeight : im.getHeight();
                    g.drawImage(im, px, py, 0, 0, w, h);
                }
            }
        }
    }
}
