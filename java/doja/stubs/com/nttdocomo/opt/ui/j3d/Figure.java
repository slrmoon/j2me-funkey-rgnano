package com.nttdocomo.opt.ui.j3d;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * A MascotCapsule model with its current posture: setPosture computes
 * the vertices and normals of every bone for an action frame.
 */
public class Figure {
    FkModel model;
    /** posed vertices and normals (model space) */
    int[] posed;
    int[] posedNormals;
    Texture[] textures;
    int selected = -1;
    int pattern;

    private int[] local;
    private int[] global;
    private int[] act;
    private boolean[] done;

    protected Figure() {
    }

    public Figure(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        try {
            init(FkModel.load(data));
        } catch (IOException e) {
            throw new IllegalArgumentException(e.getMessage());
        }
    }

    public Figure(InputStream in) throws IOException {
        this(readAll(in));
    }

    static byte[] readAll(InputStream in) throws IOException {
        if (in == null) {
            throw new NullPointerException("inputStream");
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        int n;
        while ((n = in.read(buf)) > 0) {
            out.write(buf, 0, n);
        }
        return out.toByteArray();
    }

    private void init(FkModel m) {
        model = m;
        posed = new int[m.vertices.length];
        posedNormals = m.normals == null ? null : new int[m.normals.length];
        local = new int[m.numBones * 12];
        global = new int[m.numBones * 12];
        act = new int[12];
        pose(null, 0, 0);
    }

    public void setTexture(Texture texture) {
        if (texture == null) {
            throw new NullPointerException("texture");
        }
        textures = new Texture[] { texture };
        selected = 0;
    }

    public void setTexture(Texture[] list) {
        if (list == null) {
            throw new NullPointerException("textures");
        }
        for (int i = 0; i < list.length; i++) {
            if (list[i] == null) {
                throw new NullPointerException("textures");
            }
        }
        Texture[] copy = new Texture[list.length];
        System.arraycopy(list, 0, copy, 0, list.length);
        textures = copy;
        selected = copy.length == 1 ? 0 : -1;
    }

    public int getNumTextures() {
        return textures == null ? 0 : textures.length;
    }

    public void changeTexture(int index) {
        if (textures == null || index < 0 || index >= textures.length) {
            throw new IllegalArgumentException("index");
        }
        selected = index;
    }

    public void setPosture(ActionTable table, int action, int frame) {
        if (table == null) {
            throw new NullPointerException("actionTable");
        }
        if (action < 0 || action >= table.data.numActions) {
            throw new IllegalArgumentException("action");
        }
        pose(table.data, action, frame);
        pattern = table.data.patternAt(action, frame, pattern);
    }

    public int getNumPattern() {
        return model.numPatterns;
    }

    public void setPattern(int p) {
        pattern = p;
    }

    /** texture for a polygon's texture number */
    Texture textureFor(int polyTexture) {
        if (textures == null || textures.length == 0) {
            return null;
        }
        int i = selected >= 0 ? selected : polyTexture;
        if (i < 0 || i >= textures.length) {
            i = 0;
        }
        return textures[i];
    }

    private static void mul34(int[] d, int doff, int[] a, int aoff, int[] b, int boff) {
        int a00 = a[aoff], a01 = a[aoff + 1], a02 = a[aoff + 2], a03 = a[aoff + 3];
        int a10 = a[aoff + 4], a11 = a[aoff + 5], a12 = a[aoff + 6], a13 = a[aoff + 7];
        int a20 = a[aoff + 8], a21 = a[aoff + 9], a22 = a[aoff + 10], a23 = a[aoff + 11];
        for (int c = 0; c < 4; c++) {
            int b0 = b[boff + c];
            int b1 = b[boff + 4 + c];
            int b2 = b[boff + 8 + c];
            int t = c == 3 ? 1 : 0;
            d[doff + c] = AffineTrans.mul(a00, b0) + AffineTrans.mul(a01, b1) + AffineTrans.mul(a02, b2) + t * a03;
            d[doff + 4 + c] = AffineTrans.mul(a10, b0) + AffineTrans.mul(a11, b1) + AffineTrans.mul(a12, b2) + t * a13;
            d[doff + 8 + c] = AffineTrans.mul(a20, b0) + AffineTrans.mul(a21, b1) + AffineTrans.mul(a22, b2) + t * a23;
        }
    }

    /** bone matrices (bind * action, parent first), then every vertex */
    private void pose(FkAction a, int action, int frame) {
        FkModel m = model;
        int nb = m.numBones;
        if (nb == 0) {
            System.arraycopy(m.vertices, 0, posed, 0, posed.length);
            if (posedNormals != null) {
                System.arraycopy(m.normals, 0, posedNormals, 0, posedNormals.length);
            }
            return;
        }
        for (int b = 0; b < nb; b++) {
            int o = b * 12;
            if (a != null && b < a.numBones) {
                a.boneMatrix(action, b, frame, act, 0);
                mul34(local, o, m.boneMatrix, o, act, 0);
            } else {
                System.arraycopy(m.boneMatrix, o, local, o, 12);
            }
        }
        /* parents come before children in MBAC files; fall back to a loop */
        if (done == null || done.length < nb) {
            done = new boolean[nb];
        }
        for (int b = 0; b < nb; b++) {
            done[b] = false;
        }
        int left = nb;
        while (left > 0) {
            int before = left;
            for (int b = 0; b < nb; b++) {
                if (done[b]) {
                    continue;
                }
                int p = m.boneParent[b];
                if (p < 0 || p >= nb) {
                    System.arraycopy(local, b * 12, global, b * 12, 12);
                } else if (done[p]) {
                    mul34(global, b * 12, global, p * 12, local, b * 12);
                } else {
                    continue;
                }
                done[b] = true;
                left--;
            }
            if (left == before) {
                break;   /* cycle: leave the rest unposed */
            }
        }
        int[] v = m.vertices;
        int[] n = m.normals;
        int vi = 0;
        for (int b = 0; b < nb; b++) {
            int o = b * 12;
            int g00 = global[o], g01 = global[o + 1], g02 = global[o + 2], g03 = global[o + 3];
            int g10 = global[o + 4], g11 = global[o + 5], g12 = global[o + 6], g13 = global[o + 7];
            int g20 = global[o + 8], g21 = global[o + 9], g22 = global[o + 10], g23 = global[o + 11];
            int end = vi + m.boneLength[b] * 3;
            for (; vi < end && vi < v.length; vi += 3) {
                int x = v[vi], y = v[vi + 1], z = v[vi + 2];
                posed[vi] = ((x * g00 + y * g01 + z * g02 + 2048) >> 12) + g03;
                posed[vi + 1] = ((x * g10 + y * g11 + z * g12 + 2048) >> 12) + g13;
                posed[vi + 2] = ((x * g20 + y * g21 + z * g22 + 2048) >> 12) + g23;
                if (n != null) {
                    int nx = n[vi], ny = n[vi + 1], nz = n[vi + 2];
                    posedNormals[vi] = (nx * g00 + ny * g01 + nz * g02) >> 12;
                    posedNormals[vi + 1] = (nx * g10 + ny * g11 + nz * g12) >> 12;
                    posedNormals[vi + 2] = (nx * g20 + ny * g21 + nz * g22) >> 12;
                }
            }
        }
    }
}
