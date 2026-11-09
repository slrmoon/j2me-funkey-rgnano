package com.funkey.mc3;

/**
 * A batch of primitives: type (1 points, 2 lines, 3 triangles, 4 quads,
 * 5 point sprites), data layout bits (normals 0x0300, colors 0x0C00,
 * texture coordinates / point sprite parameters 0x3000) and the arrays.
 * Arrays the layout does not use are null.
 */
public final class Prim {
    public static final int POINTS = 1;
    public static final int LINES = 2;
    public static final int TRIANGLES = 3;
    public static final int QUADS = 4;
    public static final int POINT_SPRITES = 5;

    public final int type;
    public final int param;
    public final int size;
    public final int[] vertices;
    public final int[] normals;
    public final int[] colors;
    public final int[] texCoords;
    public final int[] spriteParams;

    /** vertices per primitive */
    public static int perPrimitive(int type) {
        switch (type) {
        case LINES: return 2;
        case TRIANGLES: return 3;
        case QUADS: return 4;
        case POINTS: case POINT_SPRITES: return 1;
        default: return 0;
        }
    }

    public static int vertexLength(int type, int size) {
        return size * perPrimitive(type) * 3;
    }

    public static int normalLength(int type, int param, int size) {
        if (type != TRIANGLES && type != QUADS) {
            return 0;
        }
        int n = param & 0x0300;
        return n == 0x0300 ? size * perPrimitive(type) * 3 : n == 0x0200 ? size * 3 : 0;
    }

    public static int colorLength(int param, int size) {
        int c = param & 0x0C00;
        return c == 0x0400 ? 1 : c == 0x0800 ? size : 0;
    }

    public static int texCoordLength(int type, int param, int size) {
        int t = param & 0x3000;
        if (type == POINT_SPRITES) {
            return t == 0x1000 ? 8 : t != 0 ? size * 8 : 0;
        }
        if (type != TRIANGLES && type != QUADS) {
            return 0;
        }
        return t == 0x3000 ? size * perPrimitive(type) * 2 : 0;
    }

    /** arrays allocated for the layout (command lists fill them) */
    public Prim(int type, int param, int size) {
        this(type, param, size,
                new int[vertexLength(type, size)],
                alloc(normalLength(type, param, size)),
                alloc(texCoordLength(type, param, size)),
                alloc(colorLength(param, size)));
    }

    private static int[] alloc(int n) {
        return n == 0 ? null : new int[n];
    }

    /**
     * Wraps the caller's arrays (not copied). texCoords holds the point
     * sprite parameters for point sprites. Throws IllegalArgumentException
     * when an array the layout uses is missing or too short.
     */
    public Prim(int type, int param, int size, int[] vertices, int[] normals,
            int[] texCoords, int[] colors) {
        if (perPrimitive(type) == 0) {
            throw new IllegalArgumentException("primitive type");
        }
        if (size <= 0 || size >= 256) {
            throw new IllegalArgumentException("primitive count");
        }
        this.type = type;
        this.param = param;
        this.size = size;
        this.vertices = need(vertices, vertexLength(type, size));
        this.normals = need(normals, normalLength(type, param, size));
        this.colors = need(colors, colorLength(param, size));
        int t = need(texCoords, texCoordLength(type, param, size)) == null ? 0 : 1;
        this.texCoords = t != 0 && type != POINT_SPRITES ? texCoords : null;
        this.spriteParams = t != 0 && type == POINT_SPRITES ? texCoords : null;
    }

    private static int[] need(int[] a, int n) {
        if (n == 0) {
            return null;
        }
        if (a == null) {
            throw new NullPointerException();
        }
        if (a.length < n) {
            throw new IllegalArgumentException("array too short");
        }
        return a;
    }
}
