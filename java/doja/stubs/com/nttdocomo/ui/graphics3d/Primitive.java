package com.nttdocomo.ui.graphics3d;

/**
 * Points, lines, triangles or quads given as arrays the game fills in:
 * integer coordinates, colours 0xRRGGBB, texture coordinates in texels.
 */
public class Primitive extends DrawableObject3D {
    public static final int PRIMITIVE_POINTS = 1;
    public static final int PRIMITIVE_LINES = 2;
    public static final int PRIMITIVE_TRIANGLES = 3;
    public static final int PRIMITIVE_QUADS = 4;
    public static final int PRIMITIVE_POINT_SPRITES = 5;
    public static final int NORMAL_NONE = 0;
    public static final int NORMAL_PER_FACE = 512;
    public static final int NORMAL_PER_VERTEX = 768;
    public static final int COLOR_NONE = 0;
    public static final int COLOR_PER_PRIMITIVE = 1024;
    public static final int COLOR_PER_FACE = 2048;
    public static final int COLOR_PER_VERTEX_INTERNAL = 3072;
    public static final int TEXTURE_COORD_NONE = 0;
    public static final int TEXTURE_COORD_PER_VERTEX = 12288;
    public static final int TEXTURE_COLORKEY = 16;
    public static final int POINT_SPRITE_PER_PRIMITIVE = 4096;
    public static final int POINT_SPRITE_PER_VERTEX = 12288;
    public static final int POINT_SPRITE_FLAG_LOCAL_SIZE = 0;
    public static final int POINT_SPRITE_FLAG_PIXEL_SIZE = 1;
    public static final int POINT_SPRITE_FLAG_PERSPECTIVE = 0;
    public static final int POINT_SPRITE_FLAG_NO_PERSPECTIVE = 2;

    int fkType;
    int fkParam;
    int fkCount;
    int fkPer;
    int[] fkVertex;
    int[] fkNormal;
    int[] fkColor;
    int[] fkTexCoord;
    int[] fkSprite;
    Texture fkTexture;
    int fkBlend = BLEND_NORMAL;
    float fkAlpha = 1f;

    public Primitive(int primitiveType, int primitiveParam, int primitiveCount) {
        super(TYPE_PRIMITIVE);
        if (primitiveType < PRIMITIVE_POINTS || primitiveType > PRIMITIVE_POINT_SPRITES
                || primitiveCount <= 0) {
            throw new IllegalArgumentException("primitive");
        }
        fkType = primitiveType;
        fkParam = primitiveParam;
        fkCount = primitiveCount;
        fkPer = primitiveType == PRIMITIVE_POINT_SPRITES ? 1 : primitiveType;
        int verts = primitiveCount * fkPer;
        fkVertex = new int[verts * 3];
        switch (primitiveParam & 0x0300) {
        case NORMAL_PER_FACE: fkNormal = new int[primitiveCount * 3]; break;
        case NORMAL_PER_VERTEX: fkNormal = new int[verts * 3]; break;
        default: break;
        }
        switch (primitiveParam & 0x0c00) {
        case COLOR_PER_PRIMITIVE: fkColor = new int[1]; break;
        case COLOR_PER_FACE: fkColor = new int[primitiveCount]; break;
        case COLOR_PER_VERTEX_INTERNAL: fkColor = new int[verts]; break;
        default: break;
        }
        if (primitiveType == PRIMITIVE_POINT_SPRITES) {
            fkSprite = new int[((primitiveParam & 0x3000) == POINT_SPRITE_PER_VERTEX
                    ? primitiveCount : 1) * 8];
        } else if ((primitiveParam & 0x3000) == TEXTURE_COORD_PER_VERTEX) {
            fkTexCoord = new int[verts * 2];
        }
    }

    protected Primitive() {
    }

    public int getPrimitiveType() {
        return fkType;
    }

    public int getPrimitiveParam() {
        return fkParam;
    }

    public int size() {
        return fkCount;
    }

    public int[] getVertexArray() {
        return fkVertex;
    }

    public int[] getNormalArray() {
        return fkNormal;
    }

    public int[] getColorArray() {
        return fkColor;
    }

    public int[] getTextureCoordArray() {
        return fkTexCoord;
    }

    public int[] getPointSpriteArray() {
        return fkSprite;
    }

    /** makes the vector at offset 4096 long */
    public static void normalize(int[] vector, int offset) {
        if (vector == null) {
            throw new NullPointerException("vector");
        }
        double x = vector[offset];
        double y = vector[offset + 1];
        double z = vector[offset + 2];
        double len = java.lang.Math.sqrt(x * x + y * y + z * z);
        if (len == 0) {
            return;
        }
        vector[offset] = (int) (x * 4096 / len);
        vector[offset + 1] = (int) (y * 4096 / len);
        vector[offset + 2] = (int) (z * 4096 / len);
    }

    /** degrees to the 4096-a-turn angles of the arrays */
    public static int convertAngle(float angle) {
        return (int) (angle * 4096f / 360f);
    }

    public static float convertAngle(int angle) {
        return angle * 360f / 4096f;
    }

    public void setTexture(Texture texture) {
        fkTexture = texture;
    }

    public void setPerspectiveCorrectionEnabled(boolean enabled) {
    }

    public void setBlendMode(int blendMode) {
        fkBlend = blendMode;
    }

    public void setTransparency(float transparency) {
        fkAlpha = transparency < 0 ? 0 : transparency > 1 ? 1 : transparency;
    }
}
