package com.nttdocomo.opt.ui.j3d;

import javax.microedition.m3g.Appearance;
import javax.microedition.m3g.Background;
import javax.microedition.m3g.Camera;
import javax.microedition.m3g.CompositingMode;
import javax.microedition.m3g.Graphics3D;
import javax.microedition.m3g.Image2D;
import javax.microedition.m3g.PolygonMode;
import javax.microedition.m3g.Texture2D;
import javax.microedition.m3g.Transform;
import javax.microedition.m3g.TriangleStripArray;
import javax.microedition.m3g.VertexArray;
import javax.microedition.m3g.VertexBuffer;

/**
 * MascotCapsule v3 renderer state of one DoJa Graphics, drawn with the
 * native M3G rasterizer in immediate mode straight into the target image.
 *
 * View space: x right, y down, z into the screen. Parallel projection:
 * screen = center + view * scale; perspective: screen = center +
 * view * f / z. Lighting, toon shading and the color key are applied
 * per vertex / per texture here; the rasterizer does depth test,
 * texturing and blending.
 */
public final class FkRender {
    private static final boolean TRACE = "1".equals(System.getProperty("doja.trace3d"));

    private final javax.microedition.lcdui.Image target;
    private final Runnable changed;
    private final int W;
    private final int H;

    /* transform and projection */
    private final int[] view = { 4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0 };
    private AffineTrans[] viewArray;
    private int centerX;
    private int centerY;
    private boolean perspective;
    private int scaleX = 4096;
    private int scaleY = 4096;
    private int viewW;
    private int viewH;
    private int pNear = 1;
    private int pFar = 32767;
    private int pAngle = 512;
    private int pW;
    private int pH;
    private boolean pExtent;
    private int clipX;
    private int clipY;
    private int clipW;
    private int clipH;

    /* effects */
    private boolean light;
    private int ambient = 1024;
    private int dirX;
    private int dirY;
    private int dirZ = 4096;
    private int dirI = 3072;
    private boolean semi;
    private boolean sphere;
    private Texture sphereTex;
    private boolean toon;
    private int toonH = 128;
    private int toonM = 255;
    private int toonS = 96;
    private Texture[] primTex;
    private int primIdx;
    private boolean frameDepth;

    public FkRender(javax.microedition.lcdui.Image t, Runnable onChange) {
        target = t;
        changed = onChange;
        W = t.getWidth();
        H = t.getHeight();
        clipW = W;
        clipH = H;
    }

    /* ------------------------------------------------------------------ */
    /* state                                                               */

    public void setViewTrans(AffineTrans t) {
        if (t == null) {
            throw new NullPointerException("transform");
        }
        t.get(view);
    }

    public void setViewTransArray(AffineTrans[] t) {
        viewArray = t;
    }

    public void setViewTrans(int index) {
        if (viewArray == null || index < 0 || index >= viewArray.length) {
            throw new IllegalArgumentException("index");
        }
        setViewTrans(viewArray[index]);
    }

    public void setScreenCenter(int x, int y) {
        centerX = x;
        centerY = y;
    }

    public void setScreenScale(int x, int y) {
        perspective = false;
        viewW = viewH = 0;
        scaleX = x;
        scaleY = y;
    }

    public void setScreenView(int w, int h) {
        perspective = false;
        viewW = w;
        viewH = h;
    }

    public void setPerspective(int near, int far, int angle) {
        checkNearFar(near, far);
        perspective = true;
        pExtent = false;
        pNear = near;
        pFar = far;
        pAngle = angle;
    }

    public void setPerspective(int near, int far, int w, int h) {
        checkNearFar(near, far);
        perspective = true;
        pExtent = true;
        pNear = near;
        pFar = far;
        pW = w;
        pH = h;
    }

    private static void checkNearFar(int near, int far) {
        if (near <= 0 || far <= near) {
            throw new IllegalArgumentException("near/far");
        }
    }

    public void setClipRect3D(int x, int y, int w, int h) {
        clipX = x;
        clipY = y;
        clipW = w;
        clipH = h;
    }

    public void enableLight(boolean e) {
        light = e;
    }

    public void setAmbientLight(int a) {
        ambient = a;
    }

    public void setDirectionLight(Vector3D d, int intensity) {
        if (d == null) {
            throw new NullPointerException("direction");
        }
        Vector3D n = new Vector3D(d.x, d.y, d.z);
        if (n.x != 0 || n.y != 0 || n.z != 0) {
            n.normalize();
        }
        dirX = n.x;
        dirY = n.y;
        dirZ = n.z;
        dirI = intensity;
    }

    public void enableSemiTransparent(boolean e) {
        semi = e;
    }

    public void enableSphereMap(boolean e) {
        sphere = e;
    }

    public void setSphereTexture(Texture t) {
        if (t == null) {
            throw new NullPointerException("texture");
        }
        sphereTex = t;
    }

    public void enableToonShader(boolean e) {
        toon = e;
    }

    public void setToonParam(int h, int m, int s) {
        if (h < 0 || h > 255 || m < 0 || m > 255 || s < 0 || s > 255) {
            throw new IllegalArgumentException("toon");
        }
        toonH = h;
        toonM = m;
        toonS = s;
    }

    public void setPrimitiveTextureArray(Texture t) {
        primTex = t == null ? null : new Texture[] { t };
        primIdx = 0;
    }

    public void setPrimitiveTextureArray(Texture[] t) {
        primTex = t;
        primIdx = 0;
    }

    public void setPrimitiveTexture(int index) {
        if (primTex == null || index < 0 || index >= primTex.length) {
            throw new IllegalArgumentException("index");
        }
        primIdx = index;
    }

    /* ------------------------------------------------------------------ */
    /* drawing                                                             */

    /** drawn now; occlusion is shared with the other figures until flush */
    public void drawFigure(Figure f) {
        if (f == null) {
            throw new NullPointerException("figure");
        }
        if (begin(!frameDepth)) {
            frameDepth = true;
            try {
                figure(f);
            } finally {
                end();
            }
        }
    }

    public void renderFigure(Figure f) {
        if (f == null) {
            throw new NullPointerException("figure");
        }
        if (begin(true)) {
            try {
                figure(f);
            } finally {
                end();
            }
        }
    }

    public void flush() {
        frameDepth = false;
    }

    public void renderPrimitives(PrimitiveArray p, int start, int count, int attr) {
        if (p == null) {
            throw new NullPointerException("primitives");
        }
        if (start < 0 || count < 0 || start + count > p.size()) {
            throw new ArrayIndexOutOfBoundsException();
        }
        if (count == 0) {
            return;
        }
        if (begin(true)) {
            try {
                primitives(p, start, count, attr);
            } finally {
                end();
            }
        }
    }

    public void executeCommandList(int[] cmds) {
        FkCommands.run(this, cmds);
    }

    /* ------------------------------------------------------------------ */
    /* M3G plumbing (one rasterizer for every Graphics)                    */

    private static Graphics3D g3d;
    private static Camera camera;
    private static Camera pixelCamera;
    private static Background depthOnly;
    private static Transform model;
    private static Transform proj;
    private static final float[] mtx = new float[16];
    private static VertexBuffer vb;
    private static VertexArray vaPos;
    private static VertexArray vaCol;
    private static VertexArray vaTex;
    private static int cap;
    private static short[] pos;
    private static byte[] col;
    private static short[] tex;
    private static final Appearance[] plainApp = new Appearance[16];
    private static boolean failed;

    private static void setup() {
        g3d = Graphics3D.getInstance();
        camera = new Camera();
        pixelCamera = new Camera();
        depthOnly = new Background();
        depthOnly.setColorClearEnable(false);
        depthOnly.setDepthClearEnable(true);
        model = new Transform();
        proj = new Transform();
        vb = new VertexBuffer();
        grow(768);
    }

    private static void grow(int n) {
        int c = cap == 0 ? 768 : cap;
        while (c < n) {
            c *= 2;
        }
        cap = c;
        pos = new short[c * 3];
        col = new byte[c * 4];
        tex = new short[c * 2];
        vaPos = new VertexArray(c, 3, 2);
        vaCol = new VertexArray(c, 4, 1);
        vaTex = new VertexArray(c, 2, 2);
    }

    private javax.microedition.lcdui.Graphics bound;

    private boolean begin(boolean clearDepth) {
        if (failed) {
            return false;
        }
        try {
            if (g3d == null) {
                setup();
            }
            javax.microedition.lcdui.Graphics lg = target.getGraphics();
            int x0 = clipX < 0 ? 0 : clipX;
            int y0 = clipY < 0 ? 0 : clipY;
            int x1 = clipX + clipW > W ? W : clipX + clipW;
            int y1 = clipY + clipH > H ? H : clipY + clipH;
            if (x1 <= x0 || y1 <= y0) {
                return false;
            }
            lg.setClip(x0, y0, x1 - x0, y1 - y0);
            g3d.bindTarget(lg, true, 0);
            bound = lg;
            g3d.setViewport(0, 0, W, H);
            if (clearDepth) {
                g3d.clear(depthOnly);
            }
            loadProjection();
            g3d.setCamera(camera, null);
            return true;
        } catch (RuntimeException e) {
            failed = true;
            System.out.println("DoJa 3D: M3G unavailable: " + e);
            return false;
        }
    }

    private void end() {
        if (bound != null) {
            g3d.releaseTarget();
            bound = null;
        }
        if (changed != null) {
            changed.run();
        }
    }

    /* parallel views keep this depth range */
    private static final float ZRANGE = 65536f;

    /** pixels per view unit (parallel) or focal length (perspective), x */
    float projX() {
        if (perspective) {
            /* the extent is a 4.12 fixed-point width at the near plane */
            return pExtent ? (pW <= 0 ? 1f : (float) W * pNear * 4096f / pW) : focal();
        }
        return viewW > 0 ? (float) W / viewW : scaleX / 4096f;
    }

    float projY() {
        if (perspective) {
            return pExtent ? (pH <= 0 ? 1f : (float) H * pNear * 4096f / pH) : focal();
        }
        return viewH > 0 ? (float) H / viewH : scaleY / 4096f;
    }

    /** focal length in pixels for an angle (4096 = 360 degrees) */
    private float focal() {
        int a = pAngle;
        if (a <= 0) {
            a = 512;
        }
        if (a > 2000) {
            a = 2000;
        }
        double t = java.lang.Math.tan(a * java.lang.Math.PI / 4096.0);
        return (float) (W * 0.5 / t);
    }

    private void loadProjection() {
        float fx = projX();
        float fy = projY();
        for (int i = 0; i < 16; i++) {
            mtx[i] = 0f;
        }
        float ox = 2f * centerX / W - 1f;
        float oy = 1f - 2f * centerY / H;
        if (perspective) {
            float n = pNear;
            float f = pFar;
            mtx[0] = 2f * fx / W;
            mtx[2] = ox;
            mtx[5] = -2f * fy / H;
            mtx[6] = oy;
            mtx[10] = (f + n) / (f - n);
            mtx[11] = -2f * f * n / (f - n);
            mtx[14] = 1f;
        } else {
            mtx[0] = 2f * fx / W;
            mtx[3] = ox;
            mtx[5] = -2f * fy / H;
            mtx[7] = oy;
            mtx[10] = 1f / ZRANGE;
            mtx[15] = 1f;
        }
        proj.set(mtx);
        camera.setGeneric(proj);
    }

    private static void loadModel(int[] m, int shift) {
        float s = (float) (1 << shift);
        mtx[0] = m[0] / 4096f * s;
        mtx[1] = m[1] / 4096f * s;
        mtx[2] = m[2] / 4096f * s;
        mtx[3] = m[3];
        mtx[4] = m[4] / 4096f * s;
        mtx[5] = m[5] / 4096f * s;
        mtx[6] = m[6] / 4096f * s;
        mtx[7] = m[7];
        mtx[8] = m[8] / 4096f * s;
        mtx[9] = m[9] / 4096f * s;
        mtx[10] = m[10] / 4096f * s;
        mtx[11] = m[11];
        mtx[12] = 0f;
        mtx[13] = 0f;
        mtx[14] = 0f;
        mtx[15] = 1f;
        model.set(mtx);
    }

    private static TriangleStripArray[] stripCache = new TriangleStripArray[256];

    private static TriangleStripArray strip(int tris) {
        if (tris >= stripCache.length) {
            TriangleStripArray[] n = new TriangleStripArray[tris * 2];
            System.arraycopy(stripCache, 0, n, 0, stripCache.length);
            stripCache = n;
        }
        TriangleStripArray s = stripCache[tris];
        if (s == null) {
            int[] len = new int[tris];
            for (int i = 0; i < tris; i++) {
                len[i] = 3;
            }
            s = new TriangleStripArray(0, len);
            stripCache[tris] = s;
        }
        return s;
    }

    /* appearance flags */
    private static final int F_BLEND = 3;      /* 0 opaque, 1 half, 2 add, 3 sub */
    private static final int F_KEY = 4;
    private static final int F_DOUBLE = 8;

    private static Appearance makeApp(Texture2D t, int flags) {
        Appearance a = new Appearance();
        CompositingMode cm = new CompositingMode();
        int blend = flags & F_BLEND;
        boolean key = (flags & F_KEY) != 0;
        switch (blend) {
        case 1:
            cm.setBlending(CompositingMode.ALPHA);
            break;
        case 2:
            cm.setBlending(CompositingMode.ALPHA_ADD);
            break;
        case 3:
            cm.setBlending(CompositingMode.MODULATE);
            break;
        default:
            cm.setBlending(CompositingMode.REPLACE);
            break;
        }
        cm.setAlphaThreshold(key ? (blend == 1 ? 0.25f : 0.5f) : 0f);
        cm.setDepthWriteEnable(blend == 0);
        cm.setDepthTestEnable(true);
        a.setCompositingMode(cm);
        PolygonMode pm = new PolygonMode();
        pm.setCulling((flags & F_DOUBLE) != 0 ? PolygonMode.CULL_NONE : PolygonMode.CULL_BACK);
        pm.setWinding(PolygonMode.WINDING_CW);
        pm.setShading(PolygonMode.SHADE_SMOOTH);
        pm.setPerspectiveCorrectionEnable(true);
        a.setPolygonMode(pm);
        if (t != null) {
            a.setTexture(0, t);
        }
        return a;
    }

    private static Appearance appearance(Texture t, int flags) {
        if (t == null) {
            Appearance a = plainApp[flags];
            if (a == null) {
                a = makeApp(null, flags);
                plainApp[flags] = a;
            }
            return a;
        }
        Object[] cache = (Object[]) t.m3g;
        if (cache == null) {
            cache = new Object[18];
            t.m3g = cache;
        }
        Appearance a = (Appearance) cache[flags];
        if (a == null) {
            /* keyed and opaque variants of the texture: index 0 transparent or not */
            boolean key = (flags & F_KEY) != 0;
            int slot = key ? 16 : 17;
            Texture2D t2 = (Texture2D) cache[slot];
            if (t2 == null) {
                t2 = makeTexture(t, key);
                cache[slot] = t2;
            }
            a = makeApp(t2, flags);
            cache[flags] = a;
        }
        return a;
    }

    private static int pow2(int n) {
        int p = 1;
        while (p < n && p < 256) {
            p <<= 1;
        }
        return p;
    }

    /** RGBA image resampled to power-of-two sides (texcoords are normalized) */
    private static Texture2D makeTexture(Texture t, boolean key) {
        int tw = pow2(t.width);
        int th = pow2(t.height);
        byte[] rgba = new byte[tw * th * 4];
        int[] pal = t.palette;
        int o = 0;
        for (int y = 0; y < th; y++) {
            int sy = y * t.height / th;
            for (int x = 0; x < tw; x++) {
                int sx = x * t.width / tw;
                int idx = t.pixels[sy * t.width + sx] & 255;
                int c = idx < pal.length ? pal[idx] : 0;
                rgba[o++] = (byte) (c >> 16);
                rgba[o++] = (byte) (c >> 8);
                rgba[o++] = (byte) c;
                rgba[o++] = (byte) (key && idx == 0 ? 0 : 255);
            }
        }
        Image2D img = new Image2D(Image2D.RGBA, tw, th, rgba);
        Texture2D t2 = new Texture2D(img);
        t2.setFiltering(Texture2D.FILTER_BASE_LEVEL, Texture2D.FILTER_NEAREST);
        t2.setWrapping(Texture2D.WRAP_CLAMP, Texture2D.WRAP_CLAMP);
        t2.setBlending(Texture2D.FUNC_MODULATE);
        return t2;
    }

    /** sends n vertices from the arrays and draws them as triangles */
    private void draw(int n, int shift, boolean textured, Appearance app) {
        if (n == 0) {
            return;
        }
        vaPos.set(0, n, pos);
        vaCol.set(0, n, col);
        vb.setPositions(vaPos, 1f, null);
        vb.setColors(vaCol);
        if (textured) {
            vaTex.set(0, n, tex);
            vb.setTexCoords(0, vaTex, 1f / 4096f, null);
        } else {
            vb.setTexCoords(0, null, 1f, null);
        }
        g3d.render(vb, strip(n / 3), app, model);
    }

    /* ------------------------------------------------------------------ */
    /* lighting                                                            */

    /** brightness 0..255 for a model-space normal under the current view */
    private int shade(int nx, int ny, int nz, Texture t) {
        if (!light) {
            return 255;
        }
        int vx = (view[0] * nx + view[1] * ny + view[2] * nz) >> 12;
        int vy = (view[4] * nx + view[5] * ny + view[6] * nz) >> 12;
        int vz = (view[8] * nx + view[9] * ny + view[10] * nz) >> 12;
        /* normals stay unit length (4096) under rotations; the light
         * shines along dir: faces turned against it are lit */
        int d = -((vx * dirX + vy * dirY + vz * dirZ) >> 12);
        if (d < 0) {
            d = 0;
        }
        int i = ambient + (int) (((long) d * dirI) >> 12);
        int b = i * 255 / 4096;
        if (b > 255) {
            b = 255;
        } else if (b < 0) {
            b = 0;
        }
        if (toon || (t != null && t.toon)) {
            int th = t != null && t.toon ? t.toonHighlight : toonH;
            int mid = t != null && t.toon ? t.toonMid : toonM;
            int sh = t != null && t.toon ? t.toonShadow : toonS;
            b = b >= th ? mid : sh;
        }
        return b;
    }

    /* ------------------------------------------------------------------ */
    /* figures                                                             */

    private int[] faceNormal = new int[3];

    private void normalOf(int[] p, int a, int b, int c) {
        int ax = p[b * 3] - p[a * 3];
        int ay = p[b * 3 + 1] - p[a * 3 + 1];
        int az = p[b * 3 + 2] - p[a * 3 + 2];
        int bx = p[c * 3] - p[a * 3];
        int by = p[c * 3 + 1] - p[a * 3 + 1];
        int bz = p[c * 3 + 2] - p[a * 3 + 2];
        long nx = (long) ay * bz - (long) az * by;
        long ny = (long) az * bx - (long) ax * bz;
        long nz = (long) ax * by - (long) ay * bx;
        double len = java.lang.Math.sqrt((double) (nx * nx + ny * ny + nz * nz));
        if (len < 1) {
            faceNormal[0] = 0;
            faceNormal[1] = 0;
            faceNormal[2] = 4096;
            return;
        }
        faceNormal[0] = (int) (nx * 4096 / len);
        faceNormal[1] = (int) (ny * 4096 / len);
        faceNormal[2] = (int) (nz * 4096 / len);
    }

    private static int shiftFor(int[] v, int n) {
        int max = 0;
        for (int i = 0; i < n; i++) {
            int a = v[i] < 0 ? -v[i] : v[i];
            if (a > max) {
                max = a;
            }
        }
        int s = 0;
        while ((max >> s) > 32767) {
            s++;
        }
        return s;
    }

    private Texture[] groupTex = new Texture[16];
    private int[] groupFlags = new int[16];

    private void figure(Figure f) {
        FkModel m = f.model;
        int[] p = f.posed;
        int[] nrm = f.posedNormals;
        int shift = shiftFor(p, p.length);
        loadModel(view, shift);
        int need = m.numPolys * 6;
        if (need > cap) {
            grow(need);
        }
        /* collect the distinct (texture, flags) groups, opaque ones first */
        int groups = 0;
        for (int i = 0; i < m.numPolys; i++) {
            if (!visible(m.polyPattern[i], f.pattern)) {
                continue;
            }
            Texture t = m.polyColor[i] < 0 ? f.textureFor(m.polyTexture[i]) : null;
            int fl = flags(m.polyMaterial[i], t != null);
            int g = 0;
            while (g < groups && (groupTex[g] != t || groupFlags[g] != fl)) {
                g++;
            }
            if (g == groups) {
                if (groups == groupTex.length) {
                    Texture[] nt = new Texture[groups * 2];
                    int[] nf = new int[groups * 2];
                    System.arraycopy(groupTex, 0, nt, 0, groups);
                    System.arraycopy(groupFlags, 0, nf, 0, groups);
                    groupTex = nt;
                    groupFlags = nf;
                }
                groupTex[groups] = t;
                groupFlags[groups] = fl;
                groups++;
            }
        }
        for (int pass = 0; pass < 2; pass++) {
            for (int g = 0; g < groups; g++) {
                boolean blended = (groupFlags[g] & F_BLEND) != 0;
                if (blended != (pass == 1)) {
                    continue;
                }
                figureGroup(f, groupTex[g], groupFlags[g], shift, nrm);
            }
        }
        groupTex[0] = null;
        for (int g = 0; g < groups; g++) {
            groupTex[g] = null;
        }
    }

    private static boolean visible(int polyPattern, int pattern) {
        return polyPattern == 0 || (polyPattern & pattern) != 0;
    }

    private int flags(int material, boolean textured) {
        int fl = 0;
        if (semi) {
            fl |= (material & FkModel.MAT_BLEND) >> 1;
        }
        if (textured && (material & FkModel.MAT_COLOR_KEY) != 0) {
            fl |= F_KEY;
        }
        if ((material & FkModel.MAT_DOUBLE) != 0) {
            fl |= F_DOUBLE;
        }
        return fl;
    }

    /* figure quads are strips: (0 1 2) and (2 1 3) */
    private static final int[] QUAD_STRIP = { 0, 1, 2, 2, 1, 3 };
    private static final int[] TRI = { 0, 1, 2 };

    private void figureGroup(Figure f, Texture t, int fl, int shift, int[] nrm) {
        FkModel m = f.model;
        int[] p = f.posed;
        int n = 0;
        int alpha = (fl & F_BLEND) == 1 ? 128 : 255;
        for (int i = 0; i < m.numPolys; i++) {
            if (!visible(m.polyPattern[i], f.pattern)) {
                continue;
            }
            Texture pt = m.polyColor[i] < 0 ? f.textureFor(m.polyTexture[i]) : null;
            if (pt != t || flags(m.polyMaterial[i], pt != null) != fl) {
                continue;
            }
            int base = i * 4;
            boolean quad = m.polyVerts[base + 3] >= 0;
            int[] order = quad ? QUAD_STRIP : TRI;
            int color = m.polyColor[i];
            if (nrm == null) {
                normalOf(p, m.polyVerts[base], m.polyVerts[base + 1], m.polyVerts[base + 2]);
            }
            int flat = nrm == null ? shade(faceNormal[0], faceNormal[1], faceNormal[2], t) : 0;
            for (int k = 0; k < order.length; k++) {
                int corner = order[k];
                int v = m.polyVerts[base + corner];
                pos[n * 3] = (short) (p[v * 3] >> shift);
                pos[n * 3 + 1] = (short) (p[v * 3 + 1] >> shift);
                pos[n * 3 + 2] = (short) (p[v * 3 + 2] >> shift);
                int b = nrm == null ? flat : shade(nrm[v * 3], nrm[v * 3 + 1], nrm[v * 3 + 2], t);
                if (t != null) {
                    col[n * 4] = (byte) b;
                    col[n * 4 + 1] = (byte) b;
                    col[n * 4 + 2] = (byte) b;
                    tex[n * 2] = (short) ((m.polyUV[i * 8 + corner * 2] << 12) / t.width);
                    tex[n * 2 + 1] = (short) ((m.polyUV[i * 8 + corner * 2 + 1] << 12) / t.height);
                } else {
                    col[n * 4] = (byte) (((color >> 16) & 255) * b / 255);
                    col[n * 4 + 1] = (byte) (((color >> 8) & 255) * b / 255);
                    col[n * 4 + 2] = (byte) ((color & 255) * b / 255);
                }
                col[n * 4 + 3] = (byte) alpha;
                n++;
            }
        }
        draw(n, 0, t != null, appearance(t, fl));
    }

    /* ------------------------------------------------------------------ */
    /* primitives                                                          */

    /* primitive quads are fans: (0 1 2) and (0 2 3) */
    private static final int[] QUAD_FAN = { 0, 1, 2, 0, 2, 3 };

    private void primitives(PrimitiveArray p, int start, int count, int attr) {
        int type = p.getType();
        int param = p.getParam();
        int[] v = p.getVertexArray();
        int[] tc = p.getTextureCoordArray();
        int[] cl = p.getColorArray();
        int[] nm = p.getNormalArray();
        Texture t = null;
        if (tc != null || type == Graphics3D_POINT_SPRITES) {
            if ((attr & 2) != 0 && sphereTex != null) {
                t = sphereTex;
            } else if (primTex != null && primIdx < primTex.length) {
                t = primTex[primIdx];
            }
        }
        int fl = F_DOUBLE;
        if (semi) {
            fl |= (attr & 0x60) >> 5;
        }
        if ((attr & 16) != 0 && t != null) {
            fl |= F_KEY;
        }
        boolean lit = light && (attr & 1) != 0 && nm != null;
        boolean saved = light;
        light = lit;
        try {
            if (type == Graphics3D_POINT_SPRITES || type == 1 || type == 2) {
                FkSprites.draw(this, p, start, count, t, fl);
                return;
            }
            int per = type == 4 ? 4 : 3;
            int[] order = type == 4 ? QUAD_FAN : TRI;
            int need = count * order.length;
            if (need > cap) {
                grow(need);
            }
            int shift = shiftFor(v, (start + count) * per * 3);
            loadModel(view, shift);
            int alpha = (fl & F_BLEND) == 1 ? 128 : 255;
            int normalMode = param & 0x0300;
            int colorMode = param & 0x0C00;
            int n = 0;
            for (int i = start; i < start + count; i++) {
                int color = 0xffffff;
                if (cl != null) {
                    color = colorMode == 0x0400 ? cl[0] : cl[i];
                }
                int faceB = 255;
                if (lit && normalMode == 0x0200) {
                    faceB = shade(nm[i * 3], nm[i * 3 + 1], nm[i * 3 + 2], t);
                }
                for (int k = 0; k < order.length; k++) {
                    int vi = i * per + order[k];
                    pos[n * 3] = (short) (v[vi * 3] >> shift);
                    pos[n * 3 + 1] = (short) (v[vi * 3 + 1] >> shift);
                    pos[n * 3 + 2] = (short) (v[vi * 3 + 2] >> shift);
                    int b = faceB;
                    if (lit && normalMode == 0x0300) {
                        b = shade(nm[vi * 3], nm[vi * 3 + 1], nm[vi * 3 + 2], t);
                    }
                    if (t != null) {
                        col[n * 4] = (byte) b;
                        col[n * 4 + 1] = (byte) b;
                        col[n * 4 + 2] = (byte) b;
                        if (tc != null) {
                            tex[n * 2] = (short) ((tc[vi * 2] << 12) / t.width);
                            tex[n * 2 + 1] = (short) ((tc[vi * 2 + 1] << 12) / t.height);
                        } else {
                            tex[n * 2] = 0;
                            tex[n * 2 + 1] = 0;
                        }
                    } else {
                        col[n * 4] = (byte) (((color >> 16) & 255) * b / 255);
                        col[n * 4 + 1] = (byte) (((color >> 8) & 255) * b / 255);
                        col[n * 4 + 2] = (byte) ((color & 255) * b / 255);
                    }
                    col[n * 4 + 3] = (byte) alpha;
                    n++;
                }
            }
            draw(n, shift, t != null, appearance(t, fl));
        } finally {
            light = saved;
        }
    }

    static final int Graphics3D_POINT_SPRITES = 5;

    /* ------------------------------------------------------------------ */
    /* screen-space quads (point sprites, points, lines)                   */

    /** projects a model point; returns false when it is behind the camera */
    boolean project(int x, int y, int z, float[] out) {
        float vx = (view[0] * (float) x + view[1] * (float) y + view[2] * (float) z) / 4096f + view[3];
        float vy = (view[4] * (float) x + view[5] * (float) y + view[6] * (float) z) / 4096f + view[7];
        float vz = (view[8] * (float) x + view[9] * (float) y + view[10] * (float) z) / 4096f + view[11];
        if (perspective) {
            if (vz < pNear || vz > pFar) {
                return false;
            }
            float fx = projX();
            float fy = projY();
            out[0] = centerX + vx * fx / vz;
            out[1] = centerY + vy * fy / vz;
            float f = pFar;
            float n = pNear;
            out[2] = ((f + n) / (f - n) * vz - 2f * f * n / (f - n)) / vz;
            out[3] = fx / vz;
            out[4] = fy / vz;
        } else {
            out[0] = centerX + vx * projX();
            out[1] = centerY + vy * projY();
            out[2] = vz / ZRANGE;
            out[3] = projX();
            out[4] = projY();
        }
        return out[2] >= -1f && out[2] <= 1f;
    }

    boolean isPerspective() {
        return perspective;
    }

    int nearPlane() {
        return pNear;
    }

    /** n screen-space vertices: x, y in pixels, z in NDC */
    void drawScreen(float[] xyz, int n, byte[] rgba, short[] uv, Texture t, int fl) {
        if (n > cap) {
            grow(n);
        }
        for (int i = 0; i < n; i++) {
            pos[i * 3] = (short) xyz[i * 3];
            pos[i * 3 + 1] = (short) xyz[i * 3 + 1];
            pos[i * 3 + 2] = (short) (xyz[i * 3 + 2] * 16384f);
        }
        System.arraycopy(rgba, 0, col, 0, n * 4);
        if (uv != null) {
            System.arraycopy(uv, 0, tex, 0, n * 2);
        }
        for (int i = 0; i < 16; i++) {
            mtx[i] = 0f;
        }
        mtx[0] = 2f / W;
        mtx[3] = -1f;
        mtx[5] = -2f / H;
        mtx[7] = 1f;
        mtx[10] = 1f / 16384f;
        mtx[15] = 1f;
        proj.set(mtx);
        pixelCamera.setGeneric(proj);
        g3d.setCamera(pixelCamera, null);
        model.setIdentity();
        draw(n, 0, uv != null && t != null, appearance(t, fl | F_DOUBLE));
        g3d.setCamera(camera, null);
    }

    static void trace(String s) {
        if (TRACE) {
            System.out.println("DoJa 3D: " + s);
        }
    }
}
