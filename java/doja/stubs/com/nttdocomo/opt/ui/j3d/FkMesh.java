package com.nttdocomo.opt.ui.j3d;

/**
 * A posed figure (MBAC model, MTRA action) handed out as plain triangles,
 * for the DoJa 5 com.nttdocomo.ui.graphics3d layer: the same engine, drawn
 * by another renderer.
 */
public final class FkMesh {
    /** Gets the triangles, one batch for each texture and material. */
    public interface Sink {
        /**
         * @param texture  the polygons' texture number, -1: plain colours
         * @param material FkMesh.KEY, BLEND_*, DOUBLE bits
         * @param n        vertices (three for a triangle) in pos / uv / rgb
         */
        void batch(int texture, int material, int n);
    }

    public static final int KEY = 1;
    public static final int BLEND_MASK = 6;
    public static final int BLEND_HALF = 2;
    public static final int BLEND_ADD = 4;
    public static final int BLEND_SUB = 6;
    public static final int DOUBLE = 16;

    private static final int[] QUAD = { 0, 1, 2, 2, 1, 3 };
    private static final int[] TRI = { 0, 1, 2 };

    /** x, y, z of each vertex, in the model's units */
    public float[] pos = new float[0];
    /** texture coordinates in texels */
    public float[] uv = new float[0];
    /** colour 0..1 of each vertex (white for textured polygons) */
    public float[] rgb = new float[0];

    private int[] keys = new int[8];

    public static Figure load(byte[] mbac) {
        return new Figure(mbac);
    }

    public static ActionTable loadActions(byte[] mtra) {
        return new ActionTable(mtra);
    }

    public static int numPatterns(Figure f) {
        return f.getNumPattern();
    }

    public static void setPattern(Figure f, int pattern) {
        f.setPattern(pattern);
    }

    public static int numActions(ActionTable t) {
        return t.getNumAction();
    }

    /** frames as 16.16 */
    public static int maxFrame(ActionTable t, int action) {
        return t.getMaxFrame(action);
    }

    public static void pose(Figure f, ActionTable t, int action, int frame) {
        if (t == null || action < 0 || action >= t.data.numActions) {
            return;
        }
        f.setPosture(t, action, frame);
    }

    public void emit(Figure f, Sink sink) {
        FkModel m = f.model;
        int[] p = f.posed;
        int need = m.numPolys * 6;
        if (pos.length < need * 3) {
            pos = new float[need * 3];
            uv = new float[need * 2];
            rgb = new float[need * 3];
        }
        /* the distinct (texture, material) pairs */
        int nk = 0;
        for (int i = 0; i < m.numPolys; i++) {
            if (!visible(m.polyPattern[i], f.pattern)) {
                continue;
            }
            int key = key(m, i);
            int k = 0;
            while (k < nk && keys[k] != key) {
                k++;
            }
            if (k == nk) {
                if (nk == keys.length) {
                    int[] g = new int[nk * 2];
                    System.arraycopy(keys, 0, g, 0, nk);
                    keys = g;
                }
                keys[nk++] = key;
            }
        }
        for (int k = 0; k < nk; k++) {
            int key = keys[k];
            int n = 0;
            for (int i = 0; i < m.numPolys; i++) {
                if (!visible(m.polyPattern[i], f.pattern) || key(m, i) != key) {
                    continue;
                }
                int base = i * 4;
                int[] order = m.polyVerts[base + 3] >= 0 ? QUAD : TRI;
                int color = m.polyColor[i];
                for (int c = 0; c < order.length; c++) {
                    int corner = order[c];
                    int v = m.polyVerts[base + corner] * 3;
                    pos[n * 3] = p[v];
                    pos[n * 3 + 1] = p[v + 1];
                    pos[n * 3 + 2] = p[v + 2];
                    if (color < 0) {
                        uv[n * 2] = m.polyUV[i * 8 + corner * 2];
                        uv[n * 2 + 1] = m.polyUV[i * 8 + corner * 2 + 1];
                        rgb[n * 3] = 1f;
                        rgb[n * 3 + 1] = 1f;
                        rgb[n * 3 + 2] = 1f;
                    } else {
                        rgb[n * 3] = ((color >> 16) & 255) / 255f;
                        rgb[n * 3 + 1] = ((color >> 8) & 255) / 255f;
                        rgb[n * 3 + 2] = (color & 255) / 255f;
                    }
                    n++;
                }
            }
            int tex = (key >> 8) - 1;
            sink.batch(tex, key & 255, n);
        }
    }

    private static int key(FkModel m, int i) {
        int tex = m.polyColor[i] < 0 ? m.polyTexture[i] + 1 : 0;
        return (tex << 8) | (m.polyMaterial[i] & (KEY | BLEND_MASK | DOUBLE));
    }

    private static boolean visible(int polyPattern, int pattern) {
        return polyPattern == 0 || (polyPattern & pattern) != 0;
    }
}
