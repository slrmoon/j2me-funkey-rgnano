package com.nttdocomo.ui;

public class SpriteSet {
    private final Sprite[] sprites;
    private final int[] flags;

    public SpriteSet(Sprite[] s) {
        if (s == null) {
            throw new NullPointerException("sprites");
        }
        if (s.length > 32) {
            throw new IllegalArgumentException("too many sprites");
        }
        sprites = s;
        flags = new int[s.length];
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

    private boolean hit(int a, int b) {
        Sprite p = sprites[a];
        Sprite q = sprites[b];
        if (p == null || q == null || !p.visible || !q.visible) {
            return false;
        }
        return p.x < q.x + q.width && q.x < p.x + p.width
                && p.y < q.y + q.height && q.y < p.y + p.height;
    }

    public void setCollisionAll() {
        for (int i = 0; i < sprites.length; i++) {
            setCollisionOf(i);
        }
    }

    public void setCollisionOf(int index) {
        if (index < 0 || index >= sprites.length) {
            throw new ArrayIndexOutOfBoundsException(index);
        }
        int f = 0;
        for (int j = 0; j < sprites.length; j++) {
            if (j != index && hit(index, j)) {
                f |= 1 << j;
            }
        }
        flags[index] = f;
    }

    public boolean isCollision(int a, int b) {
        if (a < 0 || a >= sprites.length || b < 0 || b >= sprites.length) {
            throw new ArrayIndexOutOfBoundsException();
        }
        return (flags[a] & (1 << b)) != 0;
    }

    public int getCollisionFlag(int index) {
        return flags[index];
    }
}
