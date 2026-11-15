package com.nttdocomo.ui.graphics3d;

import com.funkey.doja.gl.FkGL;
import com.nttdocomo.opt.ui.j3d.FkMesh;
import com.nttdocomo.ui.util3d.Transform;
import java.util.Vector;

/**
 * The DoJa 5 3D of a Graphics (com.nttdocomo.ui.graphics3d.Graphics3D),
 * drawn with the software GL the OpenGL ES and C3D layers use.
 *
 * The camera looks along +z, x to the right, y down: a point lands at
 * centre + (x, y) * scale / z. Coordinates are plain numbers (the vertex
 * arrays are integers), transforms are float matrices, row by row.
 *
 * renderObject3D() only collects: the triangles of the object as it is at
 * that moment, with the view, fog and light of that moment. flushBuffer()
 * draws what was collected, over whatever 2D the game painted meanwhile
 * (games paint their background between the two). unlock() flushes too.
 */
public final class FkG3D {
    private static final boolean TRACE = System.getProperty("doja.g3dtrace") != null;
    /* model units of a figure are 1/64 of the units of the transforms */
    private static final float FIGURE_SCALE = 1f / 64f;

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
    private float ambient = 1f;
    private boolean warned;
    private final float[] v4 = new float[4];
    private final float[] cols = new float[16];
    private final Vector queue = new Vector();
    private final FkMesh mesh = new FkMesh();

    /** one draw call, kept until the flush */
    private static final class Cmd {
        float[] proj;
        float[] mv;
        int mode;
        int n;
        float[] pos;
        float[] col;
        float[] tex;
        Texture texture;
        boolean replace;
        boolean key;
        boolean blend;
        int src;
        int dst;
        boolean cull;
        boolean clip;
        int cx;
        int cy;
        int cw;
        int ch;
        boolean fog;
        int fogMode;
        float fogNear;
        float fogFar;
        float fogDensity;
        int fogColor;
    }

    /* the state a render call hands to the commands it makes */
    private float[] curProj;
    private int curOx;
    private int curOy;

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

    /* lights: the ambient light sets how bright figures are */
    public void addLight(Light l) {
        if (l == null) {
            throw new NullPointerException("light");
        }
        if (TRACE) {
            System.out.println("G3D light mode=" + l.fkMode + " i=" + l.fkIntensity);
        }
        if (l.fkMode == Light.AMBIENT) {
            ambient = l.fkIntensity < 0 ? 0 : l.fkIntensity > 1 ? 1 : l.fkIntensity;
        }
    }

    public void resetLights() {
        ambient = 1f;
    }

    public boolean pending() {
        return !queue.isEmpty();
    }

    /* ------------------------------------------------------------------ */
    /* collecting                                                          */

    /** a * b, row by row matrices */
    private static float[] mul(float[] a, float[] b) {
        float[] r = new float[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j]
                        + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
            }
        }
        return r;
    }

    /** pixels a unit at distance 1 takes, across */
    private float scaleX() {
        if (extent) {
            return W * near / extW;
        }
        return (W / 2f) / (float) java.lang.Math.tan(java.lang.Math.toRadians(angle) / 2);
    }

    private float scaleY() {
        return extent ? H * near / extH : scaleX();
    }

    /** projection, column by column: +z into the screen, y down, centre moved by the origin */
    private float[] projection(int ox, int oy) {
        float tx = 2f * ox / W;
        float ty = 2f * oy / H;
        float[] pr = new float[16];
        if (persp) {
            pr[0] = scaleX() * 2f / W;
            pr[5] = -scaleY() * 2f / H;
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
        return pr;
    }

    public void render(DrawableObject3D o, Transform t, int originX, int originY) {
        if (o == null) {
            throw new NullPointerException("object");
        }
        curProj = projection(originX, originY);
        curOx = originX;
        curOy = originY;
        float[] mv = view;
        if (t != null) {
            float[] obj = new float[16];
            t.get(obj);
            mv = mul(view, obj);
        } else {
            mv = copy(view);
        }
        if (TRACE) {
            System.out.println("G3D render " + o + " persp=" + persp + " near=" + near + " far=" + far
                    + " angle=" + angle + " mv=" + mv[0] + "," + mv[5] + "," + mv[10]
                    + " t=" + mv[3] + "," + mv[7] + "," + mv[11]);
        }
        object(o, mv);
    }

    private static float[] copy(float[] a) {
        float[] r = new float[16];
        System.arraycopy(a, 0, r, 0, 16);
        return r;
    }

    private void object(DrawableObject3D o, float[] mv) {
        if (o instanceof Group) {
            Group gr = (Group) o;
            float[] gt = new float[16];
            gr.fkTransform.get(gt);
            float[] sub = mul(mv, gt);
            for (int i = 0; i < gr.fkElements.size(); i++) {
                Object e = gr.fkElements.elementAt(i);
                if (e instanceof DrawableObject3D) {
                    object((DrawableObject3D) e, sub);
                }
            }
        } else if (o instanceof Figure) {
            figure((Figure) o, mv);
        } else if (o instanceof Primitive) {
            Primitive p = (Primitive) o;
            if (p.fkType == Primitive.PRIMITIVE_POINT_SPRITES) {
                sprites(p, mv);
            } else {
                primitive(p, mv);
            }
        }
    }

    private Cmd command(float[] mvRows) {
        Cmd c = new Cmd();
        c.proj = curProj;
        float[] m = new float[16];
        for (int r = 0; r < 4; r++) {
            for (int k = 0; k < 4; k++) {
                m[k * 4 + r] = mvRows[r * 4 + k];
            }
        }
        c.mv = m;
        c.clip = clip;
        c.cx = curOx + clipX;
        c.cy = H - (curOy + clipY + clipH);
        c.cw = clipW;
        c.ch = clipH;
        Fog f = fog;
        if (f != null) {
            c.fog = true;
            c.fogMode = f.fkMode;
            c.fogNear = f.fkNear;
            c.fogFar = f.fkFar;
            c.fogDensity = f.fkDensity;
            c.fogColor = f.fkColor;
        }
        c.src = FkGL.SRC_ALPHA;
        c.dst = FkGL.ONE_MINUS_SRC_ALPHA;
        return c;
    }

    private static int colorOf(Primitive p, int vertex) {
        int[] c = p.fkColor;
        if (c == null) {
            return 0xffffff;
        }
        int i = (p.fkParam & 0x0c00) == Primitive.COLOR_PER_VERTEX_INTERNAL ? vertex : vertex / p.fkPer;
        return i < c.length ? c[i] : c[0];
    }

    private void primitive(Primitive p, float[] mv) {
        int per = p.fkPer;
        int n = p.fkCount * per;
        Texture t = per >= 3 && p.fkTexCoord != null ? p.fkTexture : null;
        if (t != null && t.fkRgba == null) {
            t = null;
        }
        Cmd c = command(mv);
        c.n = n;
        c.pos = new float[n * 3];
        c.col = new float[n * 4];
        int[] v = p.fkVertex;
        for (int i = 0; i < n * 3; i++) {
            c.pos[i] = v[i];
        }
        boolean blend = p.fkBlend == DrawableObject3D.BLEND_ALPHA
                || p.fkBlend == DrawableObject3D.BLEND_ADD;
        float alpha = blend ? p.fkAlpha : 1f;
        for (int i = 0; i < n; i++) {
            int rgb = colorOf(p, i);
            c.col[i * 4] = ((rgb >> 16) & 255) / 255f;
            c.col[i * 4 + 1] = ((rgb >> 8) & 255) / 255f;
            c.col[i * 4 + 2] = (rgb & 255) / 255f;
            c.col[i * 4 + 3] = alpha;
        }
        if (t != null) {
            c.texture = t;
            c.replace = t.fkBlend == Texture.REPLACE && !blend;
            c.tex = new float[n * 2];
            float iw = 1f / t.fkWidth;
            float ih = 1f / t.fkHeight;
            int[] uv = p.fkTexCoord;
            for (int i = 0; i < n; i++) {
                c.tex[i * 2] = uv[i * 2] * iw;
                c.tex[i * 2 + 1] = uv[i * 2 + 1] * ih;
            }
        }
        c.key = t != null && (p.fkParam & Primitive.TEXTURE_COLORKEY) != 0;
        c.blend = blend;
        if (p.fkBlend == DrawableObject3D.BLEND_ADD) {
            c.dst = FkGL.ONE;
        }
        switch (p.fkType) {
        case Primitive.PRIMITIVE_POINTS: c.mode = FkGL.POINTS; break;
        case Primitive.PRIMITIVE_LINES: c.mode = FkGL.LINES; break;
        case Primitive.PRIMITIVE_TRIANGLES: c.mode = FkGL.TRIANGLES; break;
        default: c.mode = FkGL.QUADS; break;
        }
        queue.addElement(c);
    }

    /**
     * Point sprites: a textured rectangle facing the camera at each point.
     * Eight numbers describe one: width, height, angle (4096 a turn),
     * u0, v0, u1, v1, flags; one set for all or one for each point.
     */
    private void sprites(Primitive p, float[] mv) {
        Texture t = p.fkTexture;
        int[] sp = p.fkSprite;
        if (t == null || t.fkRgba == null || sp == null) {
            return;
        }
        int count = p.fkCount;
        boolean each = sp.length >= count * 8 && (p.fkParam & 0x3000) == Primitive.POINT_SPRITE_PER_VERTEX;
        Cmd c = command(IDENTITY);
        c.pos = new float[count * 12];
        c.col = new float[count * 16];
        c.tex = new float[count * 8];
        boolean blend = p.fkBlend == DrawableObject3D.BLEND_ALPHA
                || p.fkBlend == DrawableObject3D.BLEND_ADD;
        float alpha = blend ? p.fkAlpha : 1f;
        float sx = persp ? scaleX() : (float) W / orthoW;
        float sy = persp ? scaleY() : (float) H / orthoH;
        float iw = 1f / t.fkWidth;
        float ih = 1f / t.fkHeight;
        int[] v = p.fkVertex;
        int n = 0;
        for (int i = 0; i < count; i++) {
            float x = v[i * 3];
            float y = v[i * 3 + 1];
            float z = v[i * 3 + 2];
            float ex = mv[0] * x + mv[1] * y + mv[2] * z + mv[3];
            float ey = mv[4] * x + mv[5] * y + mv[6] * z + mv[7];
            float ez = mv[8] * x + mv[9] * y + mv[10] * z + mv[11];
            if (persp && ez < near) {
                continue;
            }
            int o = each ? i * 8 : 0;
            int flags = sp[o + 7];
            boolean pixels = (flags & Primitive.POINT_SPRITE_FLAG_PIXEL_SIZE) != 0;
            boolean flat = (flags & Primitive.POINT_SPRITE_FLAG_NO_PERSPECTIVE) != 0;
            /* half sizes in view units at the point's distance */
            float hw;
            float hh;
            if (persp) {
                if (pixels) {
                    float px = flat ? 1f : near / ez;
                    hw = sp[o] * px * ez / sx / 2;
                    hh = sp[o + 1] * px * ez / sy / 2;
                } else {
                    float k = flat ? ez / near : 1f;
                    hw = sp[o] * k / 2;
                    hh = sp[o + 1] * k / 2;
                }
            } else if (pixels) {
                hw = sp[o] / sx / 2;
                hh = sp[o + 1] / sy / 2;
            } else {
                hw = sp[o] / 2f;
                hh = sp[o + 1] / 2f;
            }
            if (hw <= 0 || hh <= 0) {
                continue;
            }
            double rad = sp[o + 2] * java.lang.Math.PI / 2048.0;
            float sin = (float) java.lang.Math.sin(rad);
            float cos = (float) java.lang.Math.cos(rad);
            float u0 = sp[o + 3] * iw;
            float v0 = sp[o + 4] * ih;
            float u1 = sp[o + 5] * iw;
            float v1 = sp[o + 6] * ih;
            int rgb = colorOf(p, i);
            for (int k = 0; k < 4; k++) {
                float lx = (k == 0 || k == 3) ? -hw : hw;
                float ly = k < 2 ? -hh : hh;
                c.pos[n * 3] = ex + lx * cos - ly * sin;
                c.pos[n * 3 + 1] = ey + lx * sin + ly * cos;
                c.pos[n * 3 + 2] = ez;
                c.tex[n * 2] = (k == 0 || k == 3) ? u0 : u1;
                c.tex[n * 2 + 1] = k < 2 ? v0 : v1;
                c.col[n * 4] = ((rgb >> 16) & 255) / 255f;
                c.col[n * 4 + 1] = ((rgb >> 8) & 255) / 255f;
                c.col[n * 4 + 2] = (rgb & 255) / 255f;
                c.col[n * 4 + 3] = alpha;
                n++;
            }
        }
        if (n == 0) {
            return;
        }
        if (n < count * 4) {
            /* the arrays of a draw go together: cut all to the sprites kept */
            c.pos = cut(c.pos, n * 3);
            c.col = cut(c.col, n * 4);
            c.tex = cut(c.tex, n * 2);
        }
        c.n = n;
        c.mode = FkGL.QUADS;
        c.texture = t;
        c.key = (p.fkParam & Primitive.TEXTURE_COLORKEY) != 0;
        c.blend = blend;
        if (p.fkBlend == DrawableObject3D.BLEND_ADD) {
            c.dst = FkGL.ONE;
        }
        queue.addElement(c);
    }

    private static final float[] IDENTITY = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    private static float[] cut(float[] a, int n) {
        float[] r = new float[n];
        System.arraycopy(a, 0, r, 0, n);
        return r;
    }

    /* figures: the j3d engine poses them, FkMesh gives the triangles */
    private Figure curFigure;
    private float[] curMv;
    private final FkMesh.Sink sink = new FkMesh.Sink() {
        public void batch(int texture, int material, int n) {
            figureBatch(curFigure, curMv, texture, material, n);
        }
    };

    private void figure(Figure f, float[] mv) {
        if (f.fkFigure == null) {
            return;
        }
        float[] m = copy(mv);
        for (int r = 0; r < 3; r++) {
            m[r * 4] *= FIGURE_SCALE;
            m[r * 4 + 1] *= FIGURE_SCALE;
            m[r * 4 + 2] *= FIGURE_SCALE;
        }
        curFigure = f;
        curMv = m;
        try {
            mesh.emit(f.fkFigure, sink);
        } finally {
            curFigure = null;
            curMv = null;
        }
    }

    private void figureBatch(Figure f, float[] mv, int texture, int material, int n) {
        if (n == 0) {
            return;
        }
        Texture t = null;
        if (texture >= 0 && f.fkTextures != null && f.fkTextures.length > 0) {
            t = f.fkTextures[texture < f.fkTextures.length ? texture : 0];
            if (t != null && t.fkRgba == null) {
                t = null;
            }
        }
        if (TRACE) {
            System.out.println("G3D figure batch tex=" + texture + " mat=" + material + " n=" + n
                    + " textured=" + (t != null));
        }
        Cmd c = command(mv);
        c.n = n;
        c.mode = FkGL.TRIANGLES;
        c.pos = cut(mesh.pos, n * 3);
        c.col = new float[n * 4];
        int mb = material & FkMesh.BLEND_MASK;
        boolean objBlend = f.fkBlend == DrawableObject3D.BLEND_ALPHA
                || f.fkBlend == DrawableObject3D.BLEND_ADD;
        float alpha = objBlend ? f.fkAlpha : 1f;
        if (mb == FkMesh.BLEND_HALF) {
            alpha *= 0.5f;
        }
        float[] rgb = mesh.rgb;
        for (int i = 0; i < n; i++) {
            c.col[i * 4] = rgb[i * 3] * ambient;
            c.col[i * 4 + 1] = rgb[i * 3 + 1] * ambient;
            c.col[i * 4 + 2] = rgb[i * 3 + 2] * ambient;
            c.col[i * 4 + 3] = alpha;
        }
        if (t != null) {
            c.texture = t;
            c.tex = new float[n * 2];
            float iw = 1f / t.fkWidth;
            float ih = 1f / t.fkHeight;
            float[] uv = mesh.uv;
            for (int i = 0; i < n; i++) {
                c.tex[i * 2] = uv[i * 2] * iw;
                c.tex[i * 2 + 1] = uv[i * 2 + 1] * ih;
            }
        }
        c.key = t != null && (material & FkMesh.KEY) != 0;
        c.blend = objBlend || mb != 0;
        if (mb == FkMesh.BLEND_SUB) {
            c.src = FkGL.ZERO;
            c.dst = FkGL.ONE_MINUS_SRC_COLOR;
        } else if (mb == FkMesh.BLEND_ADD || (mb == 0 && f.fkBlend == DrawableObject3D.BLEND_ADD)) {
            c.dst = FkGL.ONE;
        }
        c.cull = CULL && (material & FkMesh.DOUBLE) == 0;
        queue.addElement(c);
    }

    private static final boolean CULL = !"0".equals(System.getProperty("doja.g3d.cull"));
    private static final int FRONT = "ccw".equals(System.getProperty("doja.g3d.front"))
            ? FkGL.CCW : FkGL.CW;

    /* ------------------------------------------------------------------ */
    /* drawing                                                             */

    /** draws what was collected; a new depth buffer for the next lot */
    public void flush() {
        if (queue.isEmpty()) {
            return;
        }
        if (TRACE) {
            System.out.println("G3D flush " + queue.size() + " on " + W + "x" + H);
        }
        FkGL g = gl;
        g.viewport(0, 0, W, H);
        g.enable(FkGL.SCISSOR_TEST, false);
        g.clearDepth(1f);
        g.depthMask(true);
        g.clear(FkGL.DEPTH_BUFFER_BIT);
        g.enable(FkGL.LIGHTING, false);
        g.enable(FkGL.DEPTH_TEST, true);
        g.depthFunc(FkGL.LEQUAL);
        int count = queue.size();
        for (int i = 0; i < count; i++) {
            Cmd c = (Cmd) queue.elementAt(i);
            if (TRACE) {
                System.out.println("G3D   draw mode=" + c.mode + " n=" + c.n + " tex=" + (c.texture != null)
                        + " key=" + c.key + " blend=" + c.blend + " cull=" + c.cull + " fog=" + c.fog
                        + (c.fog ? " " + c.fogNear + ".." + c.fogFar + " #" + Integer.toHexString(c.fogColor) : "")
                        + " clip=" + (c.clip ? c.cx + "," + c.cy + "," + c.cw + "," + c.ch : "-")
                        + " a=" + c.col[3] + " rgb=" + c.col[0] + "," + c.col[1] + "," + c.col[2]
                        + (c.texture != null ? " tex " + c.texture.fkWidth + "x" + c.texture.fkHeight
                                + " a0=" + c.texture.fkRgba[3] + " uv=" + c.tex[0] + "," + c.tex[1] + ".." + c.tex[c.n * 2 - 4] + "," + c.tex[c.n * 2 - 3] : "")
                        + " p0=" + c.pos[0] + "," + c.pos[1] + "," + c.pos[2]);
            }
            g.enable(FkGL.SCISSOR_TEST, c.clip);
            if (c.clip) {
                g.scissor(c.cx, c.cy, c.cw, c.ch);
            }
            g.matrixMode(FkGL.PROJECTION);
            g.loadMatrix(c.proj);
            g.matrixMode(FkGL.MODELVIEW);
            g.loadMatrix(c.mv);
            g.enable(FkGL.FOG, c.fog);
            if (c.fog) {
                v4[0] = c.fogMode == Fog.LINEAR ? FkGL.LINEAR : FkGL.EXP;
                g.fog(FkGL.FOG_MODE, v4);
                v4[0] = c.fogNear;
                g.fog(FkGL.FOG_START, v4);
                v4[0] = c.fogFar;
                g.fog(FkGL.FOG_END, v4);
                v4[0] = c.fogDensity;
                g.fog(FkGL.FOG_DENSITY, v4);
                v4[0] = ((c.fogColor >> 16) & 255) / 255f;
                v4[1] = ((c.fogColor >> 8) & 255) / 255f;
                v4[2] = (c.fogColor & 255) / 255f;
                v4[3] = 1f;
                g.fog(FkGL.FOG_COLOR, v4);
            }
            if (c.texture != null) {
                bind(c.texture);
                g.enable(FkGL.TEXTURE_2D, true);
                v4[0] = c.replace ? FkGL.REPLACE : FkGL.MODULATE;
                g.texEnv(FkGL.TEXTURE_ENV_MODE, v4);
            } else {
                g.enable(FkGL.TEXTURE_2D, false);
            }
            g.enable(FkGL.ALPHA_TEST, c.key);
            if (c.key) {
                g.alphaFunc(FkGL.GREATER, 0.5f);
            }
            g.enable(FkGL.BLEND, c.blend);
            if (c.blend) {
                g.blendFunc(c.src, c.dst);
            }
            g.depthMask(!c.blend);
            g.enable(FkGL.CULL_FACE, c.cull);
            if (c.cull) {
                g.frontFace(FRONT);
                g.cullFace(FkGL.BACK);
            }
            g.drawImmediate(c.mode, c.n, c.pos, c.col, null, c.tex);
        }
        queue.removeAllElements();
        g.depthMask(true);
        g.enable(FkGL.CULL_FACE, false);
        g.enable(FkGL.SCISSOR_TEST, false);
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
}
