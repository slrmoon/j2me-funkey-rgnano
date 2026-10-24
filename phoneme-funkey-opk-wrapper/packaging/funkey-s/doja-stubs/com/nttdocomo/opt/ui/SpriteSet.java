package com.nttdocomo.opt.ui;

/** Up to 32 sprites with bounding-box collision flags. */
public class SpriteSet {
    private Sprite[] sprites;
    private int[] flags;

    protected SpriteSet() {
        sprites = new Sprite[0];
        flags = new int[0];
    }

    public SpriteSet(Sprite[] sprites) {
        if (sprites == null) {
            throw new NullPointerException("sprites");
        }
        if (sprites.length == 0 || sprites.length > 32) {
            throw new IllegalArgumentException("sprites");
        }
        this.sprites = sprites;
        flags = new int[sprites.length];
    }

    public int getCount() {
        return sprites.length;
    }

    public Sprite[] getSprites() {
        return sprites;
    }

    public Sprite getSprite(int index) {
        return sprites[index];
    }

    public void setCollisionAll() {
        for (int i = 0; i < sprites.length; i++) {
            flags[i] = flagOf(i);
        }
    }

    public void setCollisionOf(int index) {
        flags[index] = flagOf(index);
    }

    public boolean isCollision(int a, int b) {
        if (a == b) {
            return false;
        }
        return live(sprites[a]) && live(sprites[b]) && overlaps(sprites[a], sprites[b]);
    }

    public int getCollisionFlag(int index) {
        return flags[index];
    }

    private int flagOf(int index) {
        Sprite t = sprites[index];
        if (!live(t)) {
            return 0;
        }
        int f = 0;
        for (int i = 0; i < sprites.length; i++) {
            if (i != index && live(sprites[i]) && overlaps(t, sprites[i])) {
                f |= 1 << i;
            }
        }
        return f;
    }

    private static boolean live(Sprite s) {
        return s != null && s.visible && s.image != null;
    }

    private static boolean overlaps(Sprite a, Sprite b) {
        return a.x < b.x + b.width && a.x + a.width > b.x
                && a.y < b.y + b.height && a.y + a.height > b.y;
    }
}
