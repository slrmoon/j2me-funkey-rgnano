package com.nttdocomo.opt.ui;

/**
 * Color table of a PalettedImage. Entries are RGB colors as
 * Graphics.getColorOfRGB makes them; changes show on the next draw.
 */
public class Palette {
    int[] entries;
    /** bumped on every change, images re-render when it moves */
    int version;

    public Palette(int count) {
        if (count <= 0) {
            throw new IllegalArgumentException("count");
        }
        if (count > 256) {
            count = 256;
        }
        entries = new int[count];
    }

    public Palette(int[] colors) {
        if (colors == null) {
            throw new NullPointerException("colors");
        }
        if (colors.length == 0) {
            throw new IllegalArgumentException("colors");
        }
        int n = colors.length > 256 ? 256 : colors.length;
        entries = new int[n];
        for (int i = 0; i < n; i++) {
            entries[i] = colors[i] & 0xffffff;
        }
    }

    protected Palette() {
        entries = new int[256];
    }

    public int getEntry(int index) {
        return entries[index];
    }

    public int getEntryCount() {
        return entries.length;
    }

    public void setEntry(int index, int color) {
        entries[index] = color & 0xffffff;
        version++;
    }

    int[] argb() {
        int[] a = new int[entries.length];
        for (int i = 0; i < a.length; i++) {
            a[i] = 0xff000000 | entries[i];
        }
        return a;
    }
}
