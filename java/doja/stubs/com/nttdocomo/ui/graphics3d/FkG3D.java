package com.nttdocomo.ui.graphics3d;

import com.funkey.doja.gl.FkGL;
import com.nttdocomo.ui.util3d.Transform;

/**
 * The DoJa 5 3D of a Graphics (com.nttdocomo.ui.graphics3d.Graphics3D),
 * drawn with the software GL the OpenGL ES and C3D layers use.
 *
 * The camera looks along +z, x to the right, y down: a point lands at
 * centre + (x, y) * scale / z. Coordinates are plain numbers (the vertex
 * arrays are integers), transforms are float matrices, row by row.
 */
public final class FkG3D {
    private final FkGL gl;
    private final int W;
    private final int H;
    private boolean persp;
    private boolean extent;
    private float near = 1f;
    private float far = 32767f;
    private float angle = 60f;
    private float extW;
    private float extH;
    private int orthoW;
    private int orthoH;
    private boolean clip;
    private int clipX;
    private int clipY;
    private int clipW;
    private int clipH;
    private final float[] view = new float[16];
    private Fog fog;
    private boolean depthFresh;
    private boolean warned;
    private float[] pos = new float[0];
    private float[] col = new float[0];
    private float[] tex = new float[0];
    private final float[] m = new float[16];
    private final float[] obj = new float[16];
    private final float[] v4 = new float[4];

    public FkG3D(FkGL gl) {
        this.gl = gl;
        W = gl.width();
        H = gl.height();
        orthoW = W;
        orthoH = H;
        for (int i = 0; i < 16; i += 5) {
            view[i] = 1f;
        }
    }

    public void setClipRect(int x, int y, int w, int h) {
        clip = true;
        clipX = x;
        clipY = y;
        clipW = w;
        clipH = h;
    }

    public void setParallelView(int w, int h) {
        if (w <= 0 || h <= 0) {
            throw new IllegalArgumentException("view");
        }
        persp = false;
        orthoW = w;
        orthoH = h;
    }

    public void setPerspectiveView(float n, float f, int w, int h) {
        check(n, f);
        if (w <= 0 || h <= 0) {
            throw new IllegalArgumentException("view");
        }
        persp = true;
        extent = true;
        near = n;
        far = f;
        extW = w;
        extH = h;
    }

    public void setPerspectiveView(float n, float f, float a) {
        check(n, f);
        if (a <= 0 || a >= 180) {
            throw new IllegalArgumentException("angle");
        }
        persp = true;
        extent = false;
        near = n;
        far = f;
        angle = a;
    }

    private static void check(float n, float f) {
        if (n <= 0 || f <= n) {
            throw new IllegalArgumentException("near/far");
        }
    }

    public void setTransform(Transform t) {
        if (t == null) {
            throw new NullPointerException("transform");
        }
        t.get(view);
    }

    public void setFog(Fog f) {
        fog = f;
    }

    /** what was drawn is on the screen; the next drawing starts a new depth */
    public void flush() {
        depthFresh = false;
    }

    public void render(DrawableObject3D o, Transform t, int originX, int originY) {
        if (o == null) {
            throw new NullPointerException("object");
        }
        if (!(o instanceof Primitive)) {
            if (!warned) {
                warned = true;
                com.funkey.doja.Stub.log("graphics3d: only Primitive is drawn yet");
            }
            return;
        }
        Primitive p = (Primitive) o;
        if (p.fkType == Primitive.PRIMITIVE_POINT_SPRITES) {
            if (!warned) {
                warned = true;
                com.funkey.doja.Stub.log("graphics3d: point sprites are not drawn yet");
            }
            return;
        }
        setup(t, originX, originY);
        draw(p);
    }

    /** column by column for GL, from our rows */
    private static void columns(float[] rows, float[] out) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                out[c * 4 + r] = rows[r * 4 + c];
            }
        }
    }

    private void setup(Transform t, int ox, int oy) {
        FkGL g = gl;
        g.viewport(0, 0, W, H);
        if (clip) {
            g.enable(FkGL.SCISSOR_TEST, true);
            g.scissor(ox + clipX, H - (oy + clipY + clipH), clipW, clipH);
        } else {
            g.enable(FkGL.SCISSOR_TEST, false);
        }
        if (!depthFresh) {
            depthFresh = true;
            g.clearDepth(1f);
            g.depthMask(true);
            g.clear(FkGL.DEPTH_BUFFER_BIT);
        }
        /* projection: +z into the screen, y down, centre moved by the origin */
        float tx = 2f * ox / W;
        float ty = 2f * oy / H;
        float[] pr = m;
        for (int i = 0; i < 16; i++) {
            pr[i] = 0f;
        }
        if (persp) {
            float a;
            float b;
            if (extent) {
                a = 2f * near / extW;
                b = 2f * near / extH;
            } else {
                float tan = (float) java.lang.Math.tan(java.lang.Math.toRadians(angle) / 2);
                a = 1f / tan;
                b = a * W / H;
            }
            pr[0] = a;
            pr[5] = -b;
            pr[8] = tx;
            pr[9] = -ty;
            pr[10] = (far + near) / (far - near);
            pr[11] = 1f;
            pr[14] = -2f * far * near / (far - near);
        } else {
            pr[0] = 2f / orthoW;
            pr[5] = -2f / orthoH;
            pr[10] = 1f / 32768f;
            pr[12] = tx;
            pr[13] = -ty;
            pr[15] = 1f;
        }
        g.matrixMode(FkGL.PROJECTION);
        g.loadMatrix(pr);
        g.matrixMode(FkGL.MODELVIEW);
        columns(view, m);
        g.loadMatrix(m);
        if (t != null) {
            t.get(obj);
            columns(obj, m);
            g.multMatrix(m);
        }
        g.enable(FkGL.LIGHTING, false);
        g.enable(FkGL.CULL_FACE, false);
        g.enable(FkGL.DEPTH_TEST, true);
        g.depthFunc(FkGL.LEQUAL);
        if (fog != null) {
            g.enable(FkGL.FOG, true);
            v4[0] = fog.fkMode == Fog.LINEAR ? FkGL.LINEAR : FkGL.EXP;
            g.fog(FkGL.FOG_MODE, v4);
            v4[0] = fog.fkNear;
            g.fog(FkGL.FOG_START, v4);
            v4[0] = fog.fkFar;
            g.fog(FkGL.FOG_END, v4);
            v4[0] = fog.fkDensity;
            g.fog(FkGL.FOG_DENSITY, v4);
            v4[0] = ((fog.fkColor >> 16) & 255) / 255f;
            v4[1] = ((fog.fkColor >> 8) & 255) / 255f;
            v4[2] = (fog.fkColor & 255) / 255f;
            v4[3] = 1f;
            g.fog(FkGL.FOG_COLOR, v4);
        } else {
            g.enable(FkGL.FOG, false);
        }
    }

    private void bind(Texture t) {
        FkGL g = gl;
        if (t.fkGl != g || t.fkId == 0) {
            int[] id = new int[1];
            g.genTextures(id, 0, 1);
            t.fkGl = g;
            t.fkId = id[0];
            g.bindTexture(t.fkId);
            g.texImage(0, t.fkWidth, t.fkHeight, t.fkRgba, true);
            g.texParameter(FkGL.TEXTURE_MIN_FILTER, FkGL.NEAREST);
            g.texParameter(FkGL.TEXTURE_MAG_FILTER, FkGL.NEAREST);
        } else {
            g.bindTexture(t.fkId);
        }
    }

    private void draw(Primitive p) {
        FkGL g = gl;
        int per = p.fkPer;
        int n = p.fkCount * per;
        Texture t = per >= 3 && p.fkTexCoord != null ? p.fkTexture : null;
        if (t != null && t.fkRgba == null) {
            t = null;
        }
        if (pos.length < n * 3) {
            pos = new float[n * 3];
            col = new float[n * 4];
            tex = new float[n * 2];
        }
        int[] v = p.fkVertex;
        for (int i = 0; i < n * 3; i++) {
            pos[i] = v[i];
        }
        boolean blend = p.fkBlend == DrawableObject3D.BLEND_ALPHA
                || p.fkBlend == DrawableObject3D.BLEND_ADD;
        float alpha = blend ? p.fkAlpha : 1f;
        int cmode = p.fkParam & 0x0c00;
        int[] c = p.fkColor;
        for (int i = 0; i < n; i++) {
            int rgb = 0xffffff;
            if (c != null) {
                switch (cmode) {
                case Primitive.COLOR_PER_PRIMITIVE: rgb = c[0]; break;
                case Primitive.COLOR_PER_FACE: rgb = c[i / per]; break;
                case Primitive.COLOR_PER_VERTEX_INTERNAL: rgb = c[i]; break;
                default: break;
                }
            }
            col[i * 4] = ((rgb >> 16) & 255) / 255f;
            col[i * 4 + 1] = ((rgb >> 8) & 255) / 255f;
            col[i * 4 + 2] = (rgb & 255) / 255f;
            col[i * 4 + 3] = alpha;
        }
        if (t != null) {
            bind(t);
            g.enable(FkGL.TEXTURE_2D, true);
            v4[0] = t.fkBlend == Texture.REPLACE && !blend ? FkGL.REPLACE : FkGL.MODULATE;
            g.texEnv(FkGL.TEXTURE_ENV_MODE, v4);
            float iw = 1f / t.fkWidth;
            float ih = 1f / t.fkHeight;
            int[] uv = p.fkTexCoord;
            for (int i = 0; i < n; i++) {
                tex[i * 2] = uv[i * 2] * iw;
                tex[i * 2 + 1] = uv[i * 2 + 1] * ih;
            }
        } else {
            g.enable(FkGL.TEXTURE_2D, false);
        }
        boolean key = t != null && (p.fkParam & Primitive.TEXTURE_COLORKEY) != 0;
        g.enable(FkGL.ALPHA_TEST, key);
        if (key) {
            g.alphaFunc(FkGL.GREATER, 0.5f);
        }
        g.enable(FkGL.BLEND, blend);
        if (blend) {
            g.blendFunc(FkGL.SRC_ALPHA,
                    p.fkBlend == DrawableObject3D.BLEND_ADD ? FkGL.ONE : FkGL.ONE_MINUS_SRC_ALPHA);
        }
        g.depthMask(!blend);
        int mode;
        switch (p.fkType) {
        case Primitive.PRIMITIVE_POINTS: mode = FkGL.POINTS; break;
        case Primitive.PRIMITIVE_LINES: mode = FkGL.LINES; break;
        case Primitive.PRIMITIVE_TRIANGLES: mode = FkGL.TRIANGLES; break;
        default: mode = FkGL.QUADS; break;
        }
        g.drawImmediate(mode, n, pos, col, null, t != null ? tex : null);
        g.depthMask(true);
    }
}
