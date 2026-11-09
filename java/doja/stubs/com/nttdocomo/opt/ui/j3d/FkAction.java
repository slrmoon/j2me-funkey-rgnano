package com.nttdocomo.opt.ui.j3d;

import java.io.IOException;

/**
 * MascotCapsule v3 action table (MTRA, versions 2..5): per action and
 * bone either a fixed matrix or keyframed translation / scale / rotation
 * (a direction vector) / roll. Frames are 16.16 fixed point.
 */
final class FkAction {
    int numActions;
    int numBones;
    int[] maxFrame;
    /** [action][bone] */
    Track[][] tracks;
    /** dynamic patterns: [action] frames and masks (v5) */
    int[][] dynFrames;
    int[][] dynMasks;

    static final class Track {
        int type;
        int[] fixed;                 /* type 0 */
        int[] tKey;                  /* translation keys: frame, x, y, z */
        int[] sKey;                  /* scale */
        int[] rKey;                  /* rotation direction */
        int[] rollKey;               /* frame, angle */
    }

    static FkAction load(byte[] data) throws IOException {
        FkBits r = new FkBits(data);
        if (r.u8() != 'M' || r.u8() != 'T') {
            throw new IOException("not an MTRA action table");
        }
        int version = r.u8();
        if (r.u8() != 0 || version < 2 || version > 5) {
            throw new IOException("MTRA version " + version);
        }
        FkAction a = new FkAction();
        a.numActions = r.u16();
        a.numBones = r.u16();
        for (int i = 0; i < 8; i++) {
            r.u16();
        }
        r.s32();
        a.maxFrame = new int[a.numActions];
        a.tracks = new Track[a.numActions][a.numBones];
        a.dynFrames = new int[a.numActions][];
        a.dynMasks = new int[a.numActions][];
        for (int act = 0; act < a.numActions; act++) {
            a.maxFrame[act] = r.u16();
            for (int b = 0; b < a.numBones; b++) {
                a.tracks[act][b] = readTrack(r);
            }
            if (version >= 5) {
                int n = r.u16();
                a.dynFrames[act] = new int[n];
                a.dynMasks[act] = new int[n];
                for (int i = 0; i < n; i++) {
                    a.dynFrames[act][i] = r.u16();
                    a.dynMasks[act][i] = r.s32();
                }
            }
        }
        return a;
    }

    private static int[] keys3(FkBits r) throws IOException {
        int n = r.u16();
        int[] k = new int[n * 4];
        for (int i = 0; i < n; i++) {
            k[i * 4] = r.u16();
            k[i * 4 + 1] = r.s16();
            k[i * 4 + 2] = r.s16();
            k[i * 4 + 3] = r.s16();
        }
        return k;
    }

    private static int[] keys1(FkBits r) throws IOException {
        int n = r.u16();
        int[] k = new int[n * 2];
        for (int i = 0; i < n; i++) {
            k[i * 2] = r.u16();
            k[i * 2 + 1] = r.s16();
        }
        return k;
    }

    private static Track readTrack(FkBits r) throws IOException {
        Track t = new Track();
        t.type = r.u8();
        switch (t.type) {
        case 0:
            t.fixed = new int[12];
            for (int i = 0; i < 12; i++) {
                t.fixed[i] = r.s16();
            }
            break;
        case 1:
            break;
        case 2:
            t.tKey = keys3(r);
            t.sKey = keys3(r);
            t.rKey = keys3(r);
            t.rollKey = keys1(r);
            break;
        case 3:
            t.tKey = new int[] { 0, r.s16(), r.s16(), r.s16() };
            t.rKey = keys3(r);
            t.rollKey = new int[] { 0, r.s16() };
            break;
        case 4:
            t.rKey = keys3(r);
            t.rollKey = keys1(r);
            break;
        case 5:
            t.rKey = keys3(r);
            break;
        case 6:
            t.tKey = keys3(r);
            t.rKey = keys3(r);
            t.rollKey = keys1(r);
            break;
        default:
            throw new IOException("MTRA bone type " + t.type);
        }
        return t;
    }

    /** linear interpolation of keyed values (stride 4 or 2) at a 16.16 frame */
    private static void sample(int[] k, int stride, int frame, int[] out) {
        int n = k.length / stride;
        int comps = stride - 1;
        if (n == 0) {
            for (int c = 0; c < comps; c++) {
                out[c] = 0;
            }
            return;
        }
        if (frame < 0) {
            frame = 0;
        }
        int f = frame >> 16;
        int last = (n - 1) * stride;
        if (f >= k[last] || n == 1) {
            for (int c = 0; c < comps; c++) {
                out[c] = k[last + 1 + c];
            }
            return;
        }
        int i = 0;
        while (i < n - 1 && k[(i + 1) * stride] <= f) {
            i++;
        }
        int a = i * stride;
        int b = a + stride;
        int k0 = k[a];
        int k1 = k[b];
        if (k1 <= k0 || frame <= (k0 << 16)) {
            for (int c = 0; c < comps; c++) {
                out[c] = k[a + 1 + c];
            }
            return;
        }
        int frac = (int) (((long) frame - ((long) k0 << 16)) / (k1 - k0));   /* 0..65535 */
        for (int c = 0; c < comps; c++) {
            int v0 = k[a + 1 + c];
            int v1 = k[b + 1 + c];
            out[c] = v0 + (int) (((long) (v1 - v0) * frac) >> 16);
        }
    }

    private final int[] tv = new int[3];
    private final int[] sv = new int[3];
    private final int[] rv = new int[3];
    private final int[] lv = new int[1];

    /** writes the 3x4 matrix of a bone for an action at a frame */
    void boneMatrix(int action, int bone, int frame, int[] m, int off) {
        Track t = tracks[action][bone];
        switch (t.type) {
        case 0:
            System.arraycopy(t.fixed, 0, m, off, 12);
            return;
        case 1:
            identity(m, off);
            return;
        default:
            break;
        }
        if (t.tKey != null) {
            sample(t.tKey, 4, t.type == 3 ? 0 : frame, tv);
        } else {
            tv[0] = tv[1] = tv[2] = 0;
        }
        if (t.sKey != null) {
            sample(t.sKey, 4, frame, sv);
        } else {
            sv[0] = sv[1] = sv[2] = 4096;
        }
        sample(t.rKey, 4, frame, rv);
        if (t.rollKey != null) {
            sample(t.rollKey, 2, t.type == 3 ? 0 : frame, lv);
        } else {
            lv[0] = 0;
        }
        rotation(m, off, rv[0], rv[1], rv[2], lv[0]);
        /* scale the columns */
        for (int row = 0; row < 3; row++) {
            int o = off + row * 4;
            m[o] = AffineTrans.mul(m[o], sv[0]);
            m[o + 1] = AffineTrans.mul(m[o + 1], sv[1]);
            m[o + 2] = AffineTrans.mul(m[o + 2], sv[2]);
        }
        m[off + 3] = tv[0];
        m[off + 7] = tv[1];
        m[off + 11] = tv[2];
    }

    static void identity(int[] m, int off) {
        for (int i = 0; i < 12; i++) {
            m[off + i] = 0;
        }
        m[off] = m[off + 5] = m[off + 10] = 4096;
    }

    private static int mulT(int a, int b) {
        return (int) (((long) a * b) >> 12);
    }

    /**
     * Rotation taking the z axis onto the direction (x, y, z), then a roll
     * about that axis.
     */
    private static void rotation(int[] m, int off, int x, int y, int z, int roll) {
        identity(m, off);
        long sq = (long) x * x + (long) y * y + (long) z * z;
        if (sq == 0) {
            z = 4096;
            x = y = 0;
        } else {
            int len = (int) (java.lang.Math.sqrt((double) sq) + 0.5);
            x = (int) (((long) x << 12) / len);
            y = (int) (((long) y << 12) / len);
            z = (int) (((long) z << 12) / len);
        }
        if (x == 0 && y == 0) {
            if (z < 0) {
                m[off + 5] = -4096;
                m[off + 10] = -4096;
            }
        } else {
            int xx = mulT(x, x);
            int yy = mulT(y, y);
            int xy = mulT(x, y);
            int den = xx + yy;
            int a = den == 0 ? 0 : (int) (((long) (4096 - z) << 12) / den);
            m[off] = mulT(a, yy) + z;
            m[off + 1] = -mulT(xy, a);
            m[off + 2] = x;
            m[off + 4] = m[off + 1];
            m[off + 5] = mulT(a, xx) + z;
            m[off + 6] = y;
            m[off + 8] = -x;
            m[off + 9] = -y;
            m[off + 10] = z;
        }
        if (roll != 0) {
            int s = Math.sin(roll);
            int c = Math.cos(roll);
            for (int row = 0; row < 3; row++) {
                int o = off + row * 4;
                int c0 = m[o];
                int c1 = m[o + 1];
                m[o] = AffineTrans.mul(c0, c) + AffineTrans.mul(c1, s);
                m[o + 1] = AffineTrans.mul(c1, c) - AffineTrans.mul(c0, s);
            }
        }
    }

    /** pattern mask selected by the dynamic pattern list at a frame */
    int patternAt(int action, int frame, int fallback) {
        int[] fr = dynFrames[action];
        if (fr == null) {
            return fallback;
        }
        int f = (frame < 0 ? 0 : frame) >> 16;
        int sel = fallback;
        for (int i = 0; i < fr.length; i++) {
            if (fr[i] <= f) {
                sel = dynMasks[action][i];
            } else {
                break;
            }
        }
        return sel;
    }
}
