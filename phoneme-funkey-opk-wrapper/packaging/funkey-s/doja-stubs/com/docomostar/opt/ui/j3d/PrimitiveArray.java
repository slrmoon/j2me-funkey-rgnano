package com.docomostar.opt.ui.j3d;

/**
 * Vertex/normal/color/texture arrays sized like the handset (layout from
 * openDoJa). Lenient: an odd type/param combination gets arrays instead of
 * an exception. Games fill these before Graphics3D.renderPrimitives, which
 * does not draw yet.
 */
public class PrimitiveArray {
    private final int type;
    private final int param;
    private final int size;
    private final int[] vertexArray;
    private final int[] normalArray;
    private final int[] colorArray;
    private final int[] textureCoordArray;
    private final int[] pointSpriteArray;

    public PrimitiveArray(int type, int param, int size) {
        if (size <= 0 || size >= 256) {
            throw new IllegalArgumentException("size");
        }
        int per;
        switch (type) {
        case 2: per = 2; break;     /* LINES */
        case 3: per = 3; break;     /* TRIANGLES */
        case 4: per = 4; break;     /* QUADS */
        case 1: case 5: per = 1; break; /* POINTS, POINT_SPRITES */
        default: throw new IllegalArgumentException("type");
        }
        this.type = type;
        this.param = param;
        this.size = size;
        int verts = size * per;
        vertexArray = new int[verts * 3];
        int normal = param & 0x0300;
        int color = param & 0x0C00;
        int tex = param & 0x3000;
        normalArray = normal == 0x0300 ? new int[verts * 3]
                : normal == 0x0200 ? new int[size * 3] : null;
        colorArray = color == 0x0400 ? new int[1]
                : color == 0x0800 ? new int[size] : null;
        if (type == 5) {
            textureCoordArray = null;
            pointSpriteArray = tex == 0x1000 ? new int[8] : new int[size * 8];
        } else {
            textureCoordArray = tex == 0x3000 ? new int[verts * 2] : null;
            pointSpriteArray = null;
        }
    }

    public int getType() { return type; }
    public int getParam() { return param; }
    public int size() { return size; }
    public int[] getVertexArray() { return vertexArray; }
    public int[] getNormalArray() { return normalArray; }
    public int[] getColorArray() { return colorArray; }
    public int[] getTextureCoordArray() { return textureCoordArray; }
    public int[] getPointSpriteArray() { return pointSpriteArray; }

    protected PrimitiveArray() {
        type = 3; param = 0; size = 0;
        vertexArray = new int[0];
        normalArray = null; colorArray = null;
        textureCoordArray = null; pointSpriteArray = null;
    }
}
