package com.nttdocomo.opt.ui.j3d;

/**
 * Point sprites (center-anchored billboards: width, height, angle,
 * u0, v0, u1, v1, flags), points and lines, built as screen-space quads.
 */
final class FkSprites {
    private FkSprites() {
    }

    private static float[] xyz = new float[6 * 3 * 64];
    private static byte[] rgba = new byte[6 * 4 * 64];
    private static short[] uv = new short[6 * 2 * 64];
    private static final float[] a = new float[5];
    private static final float[] b = new float[5];
    private static final float[] cx = new float[4];
    private static final float[] cy = new float[4];

    private static void ensure(int quads) {
        if (quads * 6 * 3 > xyz.length) {
            int q = xyz.length / 18;
            while (q < quads) {
                q *= 2;
            }
            xyz = new float[q * 18];
            rgba = new byte[q * 24];
            uv = new short[q * 12];
        }
    }

    private static final int[] FAN = { 0, 1, 2, 0, 2, 3 };
    private static final int[] TUV = new int[8];

    private static int quad(int n, float z, int color, int alpha, int[] tuv, Texture t) {
        for (int k = 0; k < 6; k++) {
            int c = FAN[k];
            xyz[n * 3] = cx[c];
            xyz[n * 3 + 1] = cy[c];
            xyz[n * 3 + 2] = z;
            rgba[n * 4] = (byte) (color >> 16);
            rgba[n * 4 + 1] = (byte) (color >> 8);
            rgba[n * 4 + 2] = (byte) color;
            rgba[n * 4 + 3] = (byte) alpha;
            if (tuv != null && t != null) {
                uv[n * 2] = (short) ((tuv[c * 2] << 12) / t.width);
                uv[n * 2 + 1] = (short) ((tuv[c * 2 + 1] << 12) / t.height);
            }
            n++;
        }
        return n;
    }

    static void draw(FkRender r, PrimitiveArray p, int start, int count, Texture t, int fl) {
        int type = p.getType();
        int param = p.getParam();
        int[] v = p.getVertexArray();
        int[] cl = p.getColorArray();
        int[] sp = p.getPointSpriteArray();
        int alpha = (fl & 3) == 1 ? 128 : 255;
        int per = type == 2 ? 2 : 1;
        ensure(count);
        int n = 0;
        int[] tuv = TUV;
        for (int i = start; i < start + count; i++) {
            int color = 0xffffff;
            if (cl != null && cl.length > 0) {
                color = (param & 0x0C00) == 0x0400 ? cl[0] : cl[i < cl.length ? i : 0];
            }
            int vi = i * per;
            if (!r.project(v[vi * 3], v[vi * 3 + 1], v[vi * 3 + 2], a)) {
                continue;
            }
            if (type == FkRender.Graphics3D_POINT_SPRITES) {
                if (t == null || sp == null) {
                    continue;
                }
                int o = (param & 0x3000) == 0x1000 ? 0 : i * 8;
                if (o + 8 > sp.length) {
                    continue;
                }
                int u0 = sp[o + 3];
                int v0 = sp[o + 4];
                int u1 = sp[o + 5];
                int v1 = sp[o + 6];
                if (u1 <= u0 || v1 <= v0) {
                    continue;
                }
                int flags = sp[o + 7];
                boolean pixel = (flags & 1) != 0;
                boolean flat = (flags & 2) != 0;
                float w;
                float h;
                if (r.isPerspective()) {
                    float k;
                    if (pixel) {
                        k = flat ? 1f : r.nearPlane() * a[3] / r.projX();
                        w = sp[o] * k;
                        h = sp[o + 1] * k;
                    } else if (flat) {
                        w = sp[o] * r.projX() / r.nearPlane();
                        h = sp[o + 1] * r.projY() / r.nearPlane();
                    } else {
                        w = sp[o] * a[3];
                        h = sp[o + 1] * a[4];
                    }
                } else if (pixel) {
                    w = sp[o];
                    h = sp[o + 1];
                } else {
                    w = sp[o] * a[3];
                    h = sp[o + 1] * a[4];
                }
                if (w <= 0f || h <= 0f) {
                    continue;
                }
                double ang = sp[o + 2] * java.lang.Math.PI / 2048.0;
                float s = (float) java.lang.Math.sin(ang);
                float c = (float) java.lang.Math.cos(ang);
                float hw = w / 2f;
                float hh = h / 2f;
                corner(0, a[0], a[1], -hw, -hh, c, s);
                corner(1, a[0], a[1], hw, -hh, c, s);
                corner(2, a[0], a[1], hw, hh, c, s);
                corner(3, a[0], a[1], -hw, hh, c, s);
                int ur = u1 - 1 > u0 ? u1 - 1 : u0;
                int vb = v1 - 1 > v0 ? v1 - 1 : v0;
                tuv[0] = u0; tuv[1] = v0;
                tuv[2] = ur; tuv[3] = v0;
                tuv[4] = ur; tuv[5] = vb;
                tuv[6] = u0; tuv[7] = vb;
                n = quad(n, a[2], 0xffffff, alpha, tuv, t);
            } else if (type == 1) {
                cx[0] = a[0]; cy[0] = a[1];
                cx[1] = a[0] + 1; cy[1] = a[1];
                cx[2] = a[0] + 1; cy[2] = a[1] + 1;
                cx[3] = a[0]; cy[3] = a[1] + 1;
                n = quad(n, a[2], color, alpha, null, null);
            } else {
                if (!r.project(v[vi * 3 + 3], v[vi * 3 + 4], v[vi * 3 + 5], b)) {
                    continue;
                }
                float dx = b[0] - a[0];
                float dy = b[1] - a[1];
                float len = (float) java.lang.Math.sqrt(dx * dx + dy * dy);
                if (len < 0.01f) {
                    len = 1f;
                }
                float nx = -dy / len * 0.5f;
                float ny = dx / len * 0.5f;
                cx[0] = a[0] + nx; cy[0] = a[1] + ny;
                cx[1] = b[0] + nx; cy[1] = b[1] + ny;
                cx[2] = b[0] - nx; cy[2] = b[1] - ny;
                cx[3] = a[0] - nx; cy[3] = a[1] - ny;
                n = quad(n, (a[2] + b[2]) / 2f, color, alpha, null, null);
            }
        }
        if (n > 0) {
            boolean textured = type == FkRender.Graphics3D_POINT_SPRITES;
            r.drawScreen(xyz, n, rgba, textured ? uv : null, textured ? t : null,
                    textured ? fl : fl & ~4);
        }
    }

    private static void corner(int i, float x, float y, float lx, float ly, float c, float s) {
        cx[i] = x + lx * c - ly * s;
        cy[i] = y + lx * s + ly * c;
    }
}
