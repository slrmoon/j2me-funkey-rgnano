package com.funkey.doja.gl;

import java.util.Hashtable;

import javax.microedition.m3g.Appearance;
import javax.microedition.m3g.Background;
import javax.microedition.m3g.Camera;
import javax.microedition.m3g.CompositingMode;
import javax.microedition.m3g.Fog;
import javax.microedition.m3g.Graphics3D;
import javax.microedition.m3g.Image2D;
import javax.microedition.m3g.Light;
import javax.microedition.m3g.Material;
import javax.microedition.m3g.PolygonMode;
import javax.microedition.m3g.Texture2D;
import javax.microedition.m3g.Transform;
import javax.microedition.m3g.TriangleStripArray;
import javax.microedition.m3g.VertexArray;
import javax.microedition.m3g.VertexBuffer;

/**
 * A small OpenGL ES 1.x fixed-function state machine drawn by the native
 * M3G rasterizer: used by com.nttdocomo.ui.ogl.GraphicsOGL and by the
 * C3D API (com.nttdocomo.opt.ui.j3d2). One instance per drawing surface,
 * so textures and matrices survive getGraphics().
 *
 * Triangles are expanded to one vertex per corner (no index buffers are
 * created per draw); points and lines are drawn as small screen-space
 * quads.
 */
public final class FkGL {
    /* primitives */
    public static final int POINTS = 0, LINES = 1, LINE_LOOP = 2, LINE_STRIP = 3,
            TRIANGLES = 4, TRIANGLE_STRIP = 5, TRIANGLE_FAN = 6, QUADS = 7;
    /* capabilities */
    public static final int FOG = 0x0B60, LIGHTING = 0x0B50, TEXTURE_2D = 0x0DE1,
            CULL_FACE = 0x0B44, ALPHA_TEST = 0x0BC0, BLEND = 0x0BE2, COLOR_LOGIC_OP = 0x0BF2,
            DITHER = 0x0BD0, STENCIL_TEST = 0x0B90, DEPTH_TEST = 0x0B71, LIGHT0 = 0x4000,
            POINT_SMOOTH = 0x0B10, LINE_SMOOTH = 0x0B20, SCISSOR_TEST = 0x0C11,
            COLOR_MATERIAL = 0x0B57, NORMALIZE = 0x0BA1, RESCALE_NORMAL = 0x803A,
            POLYGON_OFFSET_FILL = 0x8037, VERTEX_ARRAY = 0x8074, NORMAL_ARRAY = 0x8075,
            COLOR_ARRAY = 0x8076, TEXTURE_COORD_ARRAY = 0x8078;
    /* matrices */
    public static final int MODELVIEW = 0x1700, PROJECTION = 0x1701, TEXTURE = 0x1702;
    /* blending */
    public static final int ZERO = 0, ONE = 1, SRC_COLOR = 0x0300, ONE_MINUS_SRC_COLOR = 0x0301,
            SRC_ALPHA = 0x0302, ONE_MINUS_SRC_ALPHA = 0x0303, DST_ALPHA = 0x0304,
            ONE_MINUS_DST_ALPHA = 0x0305, DST_COLOR = 0x0306, ONE_MINUS_DST_COLOR = 0x0307;
    /* compare */
    public static final int NEVER = 0x0200, LESS = 0x0201, EQUAL = 0x0202, LEQUAL = 0x0203,
            GREATER = 0x0204, NOTEQUAL = 0x0205, GEQUAL = 0x0206, ALWAYS = 0x0207;
    /* faces */
    public static final int FRONT = 0x0404, BACK = 0x0405, FRONT_AND_BACK = 0x0408,
            CW = 0x0900, CCW = 0x0901, FLAT = 0x1D00, SMOOTH = 0x1D01;
    /* clear */
    public static final int DEPTH_BUFFER_BIT = 0x0100, STENCIL_BUFFER_BIT = 0x0400,
            COLOR_BUFFER_BIT = 0x4000;
    /* lights and materials */
    public static final int AMBIENT = 0x1200, DIFFUSE = 0x1201, SPECULAR = 0x1202,
            POSITION = 0x1203, SPOT_DIRECTION = 0x1204, SPOT_EXPONENT = 0x1205,
            SPOT_CUTOFF = 0x1206, CONSTANT_ATTENUATION = 0x1207, LINEAR_ATTENUATION = 0x1208,
            QUADRATIC_ATTENUATION = 0x1209, EMISSION = 0x1600, SHININESS = 0x1601,
            AMBIENT_AND_DIFFUSE = 0x1602, LIGHT_MODEL_TWO_SIDE = 0x0B52,
            LIGHT_MODEL_AMBIENT = 0x0B53;
    /* fog */
    public static final int FOG_DENSITY = 0x0B62, FOG_START = 0x0B63, FOG_END = 0x0B64,
            FOG_MODE = 0x0B65, FOG_COLOR = 0x0B66, EXP = 0x0800, EXP2 = 0x0801, LINEAR = 0x2601;
    /* textures */
    public static final int TEXTURE_ENV = 0x2300, TEXTURE_ENV_MODE = 0x2200,
            TEXTURE_ENV_COLOR = 0x2201, MODULATE = 0x2100, DECAL = 0x2101, ADD = 0x0104,
            REPLACE = 0x1E01, BLEND_ENV = 0x0BE2, TEXTURE_MAG_FILTER = 0x2800,
            TEXTURE_MIN_FILTER = 0x2801, TEXTURE_WRAP_S = 0x2802, TEXTURE_WRAP_T = 0x2803,
            NEAREST = 0x2600, LINEAR_FILTER = 0x2601, REPEAT = 0x2901, CLAMP_TO_EDGE = 0x812F,
            ALPHA = 0x1906, RGB = 0x1907, RGBA = 0x1908, LUMINANCE = 0x1909,
            LUMINANCE_ALPHA = 0x190A, UNSIGNED_SHORT_4_4_4_4 = 0x8033,
            UNSIGNED_SHORT_5_5_5_1 = 0x8034, UNSIGNED_SHORT_5_6_5 = 0x8363;

    private final javax.microedition.lcdui.Image target;
    private final int W;
    private final int H;
    private Runnable changed;

    /* matrices */
    private final float[][] mvStack = new float[32][16];
    private final float[][] prStack = new float[8][16];
    private final float[][] txStack = new float[8][16];
    private int mvTop;
    private int prTop;
    private int txTop;
    private int matrixMode = MODELVIEW;

    /* state */
    private final Hashtable caps = new Hashtable();
    private boolean depthTest;
    private boolean cull;
    private boolean blend;
    private boolean alphaTest;
    private boolean texture2d;
    private boolean fogOn;
    private boolean lighting;
    private boolean colorMaterial;
    private boolean scissor;
    private boolean polyOffset;
    private final boolean[] lightOn = new boolean[8];
    private int blendSrc = ONE;
    private int blendDst = ZERO;
    private int depthFunc = LESS;
    private boolean depthMask = true;
    private int alphaFunc = ALWAYS;
    private float alphaRef;
    private int cullFace = BACK;
    private int frontFace = CCW;
    private int shade = SMOOTH;
    private boolean twoSide;
    private float offsetFactor;
    private float offsetUnits;
    private int vpX;
    private int vpY;
    private int vpW;
    private int vpH;
    private int scX;
    private int scY;
    private int scW;
    private int scH;
    private float depthNear = 0f;
    private float depthFar = 1f;
    private final float[] clearColor = new float[4];
    private float clearDepth = 1f;
    private final float[] color = { 1f, 1f, 1f, 1f };
    private final float[] normal = { 0f, 0f, 1f };
    private final float[] texCoord = { 0f, 0f };
    private int error;

    /* arrays */
    public final Attrib vertexArray = new Attrib();
    public final Attrib colorArray = new Attrib();
    public final Attrib normalArray = new Attrib();
    public final Attrib texArray = new Attrib();
    private boolean vaOn;
    private boolean caOn;
    private boolean naOn;
    private boolean taOn;
    /* arrays a draw reads: the client arrays, or the immediate-mode ones */
    private Attrib curV = vertexArray;
    private Attrib curC = colorArray;
    private Attrib curN = normalArray;
    private Attrib curT = texArray;
    private boolean useC;
    private boolean useN;
    private boolean useT;
    private final Attrib immV = new Attrib();
    private final Attrib immC = new Attrib();
    private final Attrib immN = new Attrib();
    private final Attrib immT = new Attrib();

    /**
     * Immediate-mode vertices (glBegin / glEnd style): positions x,y,z,
     * colors r,g,b,a, normals, texture s,t (arrays may be null).
     */
    public void drawImmediate(int mode, int n, float[] p, float[] c, float[] nn, float[] t) {
        immV.set(3, Attrib.FLOAT, 0, p, 0);
        immC.set(4, Attrib.FLOAT, 0, c, 0);
        immN.set(3, Attrib.FLOAT, 0, nn, 0);
        immT.set(2, Attrib.FLOAT, 0, t, 0);
        curV = immV;
        curC = immC;
        curN = immN;
        curT = immT;
        useC = c != null;
        useN = nn != null;
        useT = t != null;
        try {
            drawWith(mode, null, 0, 0, n);
        } finally {
            curV = vertexArray;
            curC = colorArray;
            curN = normalArray;
            curT = texArray;
        }
    }

    /* lights / material / fog */
    private final float[][] lAmb = new float[8][];
    private final float[][] lDif = new float[8][];
    private final float[][] lSpec = new float[8][];
    private final float[][] lPos = new float[8][];
    private final float[][] lDir = new float[8][];
    private final float[] lExp = new float[8];
    private final float[] lCut = new float[8];
    private final float[][] lAtt = new float[8][];
    private final float[] modelAmbient = { 0.2f, 0.2f, 0.2f, 1f };
    private final float[] mAmb = { 0.2f, 0.2f, 0.2f, 1f };
    private final float[] mDif = { 0.8f, 0.8f, 0.8f, 1f };
    private final float[] mSpec = { 0f, 0f, 0f, 1f };
    private final float[] mEmi = { 0f, 0f, 0f, 1f };
    private float mShin;
    private int fogMode = EXP;
    private float fogDensity = 1f;
    private float fogStart = 0f;
    private float fogEnd = 1f;
    private final float[] fogColor = new float[4];

    /* buffer objects (DirectBuffer copies) */
    public final Hashtable buffers = new Hashtable();
    public int arrayBuffer;
    public int elementBuffer;
    private int nextBufferName = 1;

    public int newBufferName() {
        while (buffers.containsKey(new Integer(nextBufferName))) {
            nextBufferName++;
        }
        return nextBufferName++;
    }

    public com.nttdocomo.ui.ogl.DirectBuffer buffer(int id) {
        return id == 0 ? null : (com.nttdocomo.ui.ogl.DirectBuffer) buffers.get(new Integer(id));
    }

    /* textures */
    private final Hashtable textures = new Hashtable();
    private int boundTex;
    private int nextTexName = 1;
    private int envMode = MODULATE;
    private final float[] envColor = new float[4];

    public FkGL(javax.microedition.lcdui.Image t) {
        target = t;
        W = t.getWidth();
        H = t.getHeight();
        vpW = W;
        vpH = H;
        scW = W;
        scH = H;
        Mat.identity(mvStack[0]);
        Mat.identity(prStack[0]);
        Mat.identity(txStack[0]);
        for (int i = 0; i < 8; i++) {
            lAmb[i] = new float[] { 0f, 0f, 0f, 1f };
            lDif[i] = i == 0 ? new float[] { 1f, 1f, 1f, 1f } : new float[] { 0f, 0f, 0f, 1f };
            lSpec[i] = i == 0 ? new float[] { 1f, 1f, 1f, 1f } : new float[] { 0f, 0f, 0f, 1f };
            lPos[i] = new float[] { 0f, 0f, 1f, 0f };
            lDir[i] = new float[] { 0f, 0f, -1f };
            lCut[i] = 180f;
            lAtt[i] = new float[] { 1f, 0f, 0f };
        }
    }

    public javax.microedition.lcdui.Image target() {
        return target;
    }

    public void setChangeListener(Runnable r) {
        changed = r;
    }

    public int width() {
        return W;
    }

    public int height() {
        return H;
    }

    /* ------------------------------------------------------------------ */
    /* matrices                                                            */

    public float[] current() {
        switch (matrixMode) {
        case PROJECTION:
            return prStack[prTop];
        case TEXTURE:
            return txStack[txTop];
        default:
            return mvStack[mvTop];
        }
    }

    public float[] modelview() {
        return mvStack[mvTop];
    }

    public float[] projection() {
        return prStack[prTop];
    }

    public void matrixMode(int m) {
        if (m != MODELVIEW && m != PROJECTION && m != TEXTURE) {
            error = 0x0500;
            return;
        }
        matrixMode = m;
    }

    public void loadIdentity() {
        Mat.identity(current());
    }

    public void loadMatrix(float[] m) {
        System.arraycopy(m, 0, current(), 0, 16);
    }

    public void multMatrix(float[] m) {
        Mat.post(current(), m);
    }

    public void pushMatrix() {
        switch (matrixMode) {
        case PROJECTION:
            if (prTop + 1 < prStack.length) {
                System.arraycopy(prStack[prTop], 0, prStack[prTop + 1], 0, 16);
                prTop++;
            } else {
                error = 0x0503;
            }
            break;
        case TEXTURE:
            if (txTop + 1 < txStack.length) {
                System.arraycopy(txStack[txTop], 0, txStack[txTop + 1], 0, 16);
                txTop++;
            } else {
                error = 0x0503;
            }
            break;
        default:
            if (mvTop + 1 < mvStack.length) {
                System.arraycopy(mvStack[mvTop], 0, mvStack[mvTop + 1], 0, 16);
                mvTop++;
            } else {
                error = 0x0503;
            }
            break;
        }
    }

    public void popMatrix() {
        switch (matrixMode) {
        case PROJECTION:
            if (prTop > 0) {
                prTop--;
            } else {
                error = 0x0504;
            }
            break;
        case TEXTURE:
            if (txTop > 0) {
                txTop--;
            } else {
                error = 0x0504;
            }
            break;
        default:
            if (mvTop > 0) {
                mvTop--;
            } else {
                error = 0x0504;
            }
            break;
        }
    }

    public void translate(float x, float y, float z) {
        Mat.translate(current(), x, y, z);
    }

    public void rotate(float a, float x, float y, float z) {
        Mat.rotate(current(), a, x, y, z);
    }

    public void scale(float x, float y, float z) {
        Mat.scale(current(), x, y, z);
    }

    public void frustum(float l, float r, float b, float t, float n, float f) {
        Mat.frustum(current(), l, r, b, t, n, f);
    }

    public void ortho(float l, float r, float b, float t, float n, float f) {
        Mat.ortho(current(), l, r, b, t, n, f);
    }

    public void lookAt(float ex, float ey, float ez, float cx, float cy, float cz,
            float ux, float uy, float uz) {
        Mat.lookAt(current(), ex, ey, ez, cx, cy, cz, ux, uy, uz);
    }

    /* ------------------------------------------------------------------ */
    /* state                                                               */

    public void enable(int cap, boolean on) {
        switch (cap) {
        case DEPTH_TEST: depthTest = on; break;
        case CULL_FACE: cull = on; break;
        case BLEND: blend = on; break;
        case ALPHA_TEST: alphaTest = on; break;
        case TEXTURE_2D: texture2d = on; break;
        case FOG: fogOn = on; break;
        case LIGHTING: lighting = on; break;
        case COLOR_MATERIAL: colorMaterial = on; break;
        case SCISSOR_TEST: scissor = on; break;
        case POLYGON_OFFSET_FILL: polyOffset = on; break;
        default:
            if (cap >= LIGHT0 && cap < LIGHT0 + 8) {
                lightOn[cap - LIGHT0] = on;
            }
            break;
        }
        caps.put(new Integer(cap), on ? Boolean.TRUE : Boolean.FALSE);
    }

    public boolean isEnabled(int cap) {
        Object o = caps.get(new Integer(cap));
        return o == Boolean.TRUE || (o == null && cap == DITHER);
    }

    public void clientState(int array, boolean on) {
        switch (array) {
        case VERTEX_ARRAY: vaOn = on; break;
        case COLOR_ARRAY: caOn = on; break;
        case NORMAL_ARRAY: naOn = on; break;
        case TEXTURE_COORD_ARRAY: taOn = on; break;
        default: break;
        }
    }

    public void blendFunc(int s, int d) {
        blendSrc = s;
        blendDst = d;
    }

    public void depthFunc(int f) {
        depthFunc = f;
    }

    public void depthMask(boolean m) {
        depthMask = m;
    }

    public void depthRange(float n, float f) {
        depthNear = clamp01(n);
        depthFar = clamp01(f);
    }

    public void alphaFunc(int f, float ref) {
        alphaFunc = f;
        alphaRef = clamp01(ref);
    }

    public void cullFace(int f) {
        cullFace = f;
    }

    public void frontFace(int f) {
        frontFace = f;
    }

    public void shadeModel(int m) {
        shade = m;
    }

    public void polygonOffset(float factor, float units) {
        offsetFactor = factor;
        offsetUnits = units;
    }

    public void viewport(int x, int y, int w, int h) {
        if (w < 0 || h < 0) {
            error = 0x0501;
            return;
        }
        vpX = x;
        vpY = y;
        vpW = w;
        vpH = h;
    }

    public int[] viewportArray() {
        return new int[] { vpX, vpY, vpW, vpH };
    }

    public void scissor(int x, int y, int w, int h) {
        scX = x;
        scY = y;
        scW = w;
        scH = h;
    }

    public void clearColor(float r, float g, float b, float a) {
        clearColor[0] = clamp01(r);
        clearColor[1] = clamp01(g);
        clearColor[2] = clamp01(b);
        clearColor[3] = clamp01(a);
    }

    public void clearDepth(float d) {
        clearDepth = clamp01(d);
    }

    public void color(float r, float g, float b, float a) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
    }

    public float[] currentColor() {
        return color;
    }

    public void normal(float x, float y, float z) {
        normal[0] = x;
        normal[1] = y;
        normal[2] = z;
    }

    public float[] currentNormal() {
        return normal;
    }

    public void texCoord(float s, float t) {
        texCoord[0] = s;
        texCoord[1] = t;
    }

    public float[] currentTexCoord() {
        return texCoord;
    }

    public int getError() {
        int e = error;
        error = 0;
        return e;
    }

    public void setError(int e) {
        if (error == 0) {
            error = e;
        }
    }

    private static float clamp01(float v) {
        return v < 0f ? 0f : v > 1f ? 1f : v;
    }

    /* lights, material, fog */

    public void light(int light, int pname, float[] v) {
        int i = light - LIGHT0;
        if (i < 0 || i >= 8 || v == null) {
            error = 0x0500;
            return;
        }
        float[] mv = mvStack[mvTop];
        switch (pname) {
        case AMBIENT: copy4(v, lAmb[i]); break;
        case DIFFUSE: copy4(v, lDif[i]); break;
        case SPECULAR: copy4(v, lSpec[i]); break;
        case POSITION: {
            /* stored in eye space, as OpenGL does */
            float[] o = new float[4];
            Mat.transform(mv, v[0], v[1], v[2], v.length > 3 ? v[3] : 1f, o);
            lPos[i] = o;
            break;
        }
        case SPOT_DIRECTION: {
            float[] o = new float[4];
            Mat.transform(mv, v[0], v[1], v[2], 0f, o);
            lDir[i] = new float[] { o[0], o[1], o[2] };
            break;
        }
        case SPOT_EXPONENT: lExp[i] = v[0]; break;
        case SPOT_CUTOFF: lCut[i] = v[0]; break;
        case CONSTANT_ATTENUATION: lAtt[i][0] = v[0]; break;
        case LINEAR_ATTENUATION: lAtt[i][1] = v[0]; break;
        case QUADRATIC_ATTENUATION: lAtt[i][2] = v[0]; break;
        default: error = 0x0500; break;
        }
    }

    public void lightModel(int pname, float[] v) {
        if (pname == LIGHT_MODEL_AMBIENT) {
            copy4(v, modelAmbient);
        } else if (pname == LIGHT_MODEL_TWO_SIDE) {
            twoSide = v[0] != 0f;
        }
    }

    public void material(int face, int pname, float[] v) {
        switch (pname) {
        case AMBIENT: copy4(v, mAmb); break;
        case DIFFUSE: copy4(v, mDif); break;
        case AMBIENT_AND_DIFFUSE: copy4(v, mAmb); copy4(v, mDif); break;
        case SPECULAR: copy4(v, mSpec); break;
        case EMISSION: copy4(v, mEmi); break;
        case SHININESS: mShin = v[0]; break;
        default: error = 0x0500; break;
        }
    }

    public void fog(int pname, float[] v) {
        switch (pname) {
        case FOG_MODE: fogMode = (int) v[0]; break;
        case FOG_DENSITY: fogDensity = v[0]; break;
        case FOG_START: fogStart = v[0]; break;
        case FOG_END: fogEnd = v[0]; break;
        case FOG_COLOR: copy4(v, fogColor); break;
        default: error = 0x0500; break;
        }
    }

    private static void copy4(float[] src, float[] dst) {
        for (int k = 0; k < 4 && k < src.length; k++) {
            dst[k] = src[k];
        }
    }

    /* ------------------------------------------------------------------ */
    /* textures                                                            */

    static final class Tex {
        int w;
        int h;
        Image2D img;
        Texture2D t2d;
        int minFilter = 0x2702;   /* NEAREST_MIPMAP_LINEAR */
        int magFilter = LINEAR_FILTER;
        int wrapS = REPEAT;
        int wrapT = REPEAT;
        int format;
        boolean dirty = true;
    }

    public void genTextures(int[] out, int off, int n) {
        for (int k = 0; k < n; k++) {
            while (textures.containsKey(new Integer(nextTexName))) {
                nextTexName++;
            }
            out[off + k] = nextTexName;
            textures.put(new Integer(nextTexName), new Tex());
            nextTexName++;
        }
    }

    public void deleteTextures(int[] ids, int off, int n) {
        for (int k = 0; k < n; k++) {
            if (ids[off + k] != 0) {
                textures.remove(new Integer(ids[off + k]));
                if (boundTex == ids[off + k]) {
                    boundTex = 0;
                }
            }
        }
    }

    public boolean isTexture(int id) {
        return id != 0 && textures.containsKey(new Integer(id));
    }

    public void bindTexture(int id) {
        boundTex = id;
        if (id != 0 && !textures.containsKey(new Integer(id))) {
            textures.put(new Integer(id), new Tex());
        }
    }

    private Tex bound() {
        return boundTex == 0 ? null : (Tex) textures.get(new Integer(boundTex));
    }

    public void texParameter(int pname, int value) {
        Tex t = bound();
        if (t == null) {
            return;
        }
        switch (pname) {
        case TEXTURE_MIN_FILTER: t.minFilter = value; break;
        case TEXTURE_MAG_FILTER: t.magFilter = value; break;
        case TEXTURE_WRAP_S: t.wrapS = value; break;
        case TEXTURE_WRAP_T: t.wrapT = value; break;
        default: return;
        }
        t.dirty = true;
    }

    public void texEnv(int pname, float[] v) {
        if (pname == TEXTURE_ENV_MODE) {
            envMode = (int) v[0];
        } else if (pname == TEXTURE_ENV_COLOR) {
            copy4(v, envColor);
        }
    }

    private static int pow2(int n) {
        int p = 1;
        while (p < n && p < 1024) {
            p <<= 1;
        }
        return p;
    }

    /**
     * Level 0 of the bound texture from RGBA bytes (other levels are
     * ignored: the rasterizer filters level 0).
     */
    public void texImage(int level, int w, int h, byte[] rgba, boolean hasAlpha) {
        Tex t = bound();
        if (t == null || level != 0 || w <= 0 || h <= 0) {
            return;
        }
        int tw = pow2(w);
        int th = pow2(h);
        int fmt = hasAlpha ? Image2D.RGBA : Image2D.RGB;
        int bpp = hasAlpha ? 4 : 3;
        byte[] px = new byte[tw * th * bpp];
        int o = 0;
        for (int y = 0; y < th; y++) {
            int sy = y * h / th;
            for (int x = 0; x < tw; x++) {
                int sx = x * w / tw;
                int p = (sy * w + sx) * 4;
                px[o++] = rgba[p];
                px[o++] = rgba[p + 1];
                px[o++] = rgba[p + 2];
                if (hasAlpha) {
                    px[o++] = rgba[p + 3];
                }
            }
        }
        try {
            /* mutable, so glTexSubImage2D can patch it */
            Image2D img = new Image2D(fmt, tw, th);
            img.set(0, 0, tw, th, px);
            t.img = img;
        } catch (RuntimeException e) {
            error = 0x0501;
            return;
        }
        t.w = w;
        t.h = h;
        t.format = fmt;
        t.t2d = null;
        t.dirty = true;
        pending = null;
    }

    /** sub-image update: rebuilds from a kept RGBA copy */
    private byte[] pending;

    public void texSubImage(int level, int x, int y, int w, int h, byte[] rgba) {
        Tex t = bound();
        if (t == null || t.img == null || level != 0) {
            return;
        }
        int tw = t.img.getWidth();
        int th = t.img.getHeight();
        int bpp = t.format == Image2D.RGBA ? 4 : 3;
        if (tw != t.w || th != t.h) {
            return;   /* resampled textures cannot be patched */
        }
        byte[] row = new byte[w * bpp];
        Image2D mutable = t.img;
        if (!mutable.isMutable()) {
            return;
        }
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                int p = (j * w + i) * 4;
                row[i * bpp] = rgba[p];
                row[i * bpp + 1] = rgba[p + 1];
                row[i * bpp + 2] = rgba[p + 2];
                if (bpp == 4) {
                    row[i * bpp + 3] = rgba[p + 3];
                }
            }
            mutable.set(x, y + j, w, 1, row);
        }
    }

    private Texture2D texture2D(Tex t) {
        if (t == null || t.img == null) {
            return null;
        }
        if (t.t2d == null) {
            try {
                t.t2d = new Texture2D(t.img);
            } catch (RuntimeException e) {
                t.img = null;
                return null;
            }
            t.dirty = true;
        }
        if (t.dirty) {
            int mag = t.magFilter == NEAREST ? Texture2D.FILTER_NEAREST : Texture2D.FILTER_LINEAR;
            t.t2d.setFiltering(Texture2D.FILTER_BASE_LEVEL, mag);
            t.t2d.setWrapping(t.wrapS == REPEAT ? Texture2D.WRAP_REPEAT : Texture2D.WRAP_CLAMP,
                    t.wrapT == REPEAT ? Texture2D.WRAP_REPEAT : Texture2D.WRAP_CLAMP);
            t.dirty = false;
        }
        return t.t2d;
    }

    /* ------------------------------------------------------------------ */
    /* M3G                                                                 */

    private static Graphics3D g3d;
    private static Camera camera;
    private static Background bg;
    private static Transform xf;
    private static Transform lightXf;
    private static final float[] rowMajor = new float[16];
    private static VertexBuffer vb;
    private static VertexArray vaPos;
    private static VertexArray vaCol;
    private static VertexArray vaNrm;
    private static VertexArray vaTex;
    private static int cap;
    private static short[] pos;
    private static byte[] col;
    private static short[] nrm;
    private static short[] tex;
    private static TriangleStripArray[] strips = new TriangleStripArray[256];
    private static boolean failed;

    private Appearance app;
    private CompositingMode cm;
    private PolygonMode pm;
    private Material mat;
    private Fog fogObj;
    private Light[] m3gLights;
    private Light ambientLight;

    private static void setup() {
        g3d = Graphics3D.getInstance();
        camera = new Camera();
        bg = new Background();
        xf = new Transform();
        lightXf = new Transform();
        vb = new VertexBuffer();
        grow(1024);
    }

    private static void grow(int n) {
        int c = cap == 0 ? 1024 : cap;
        while (c < n) {
            c *= 2;
        }
        cap = c;
        pos = new short[c * 3];
        col = new byte[c * 4];
        nrm = new short[c * 3];
        tex = new short[c * 2];
        vaPos = new VertexArray(c, 3, 2);
        vaCol = new VertexArray(c, 4, 1);
        vaNrm = new VertexArray(c, 3, 2);
        vaTex = new VertexArray(c, 2, 2);
    }

    private static TriangleStripArray strip(int tris) {
        if (tris >= strips.length) {
            TriangleStripArray[] n = new TriangleStripArray[tris * 2];
            System.arraycopy(strips, 0, n, 0, strips.length);
            strips = n;
        }
        TriangleStripArray s = strips[tris];
        if (s == null) {
            int[] len = new int[tris];
            for (int i = 0; i < tris; i++) {
                len[i] = 3;
            }
            s = new TriangleStripArray(0, len);
            strips[tris] = s;
        }
        return s;
    }

    private javax.microedition.lcdui.Graphics bound;

    private boolean bind() {
        if (failed) {
            return false;
        }
        try {
            if (g3d == null) {
                setup();
            }
            javax.microedition.lcdui.Graphics lg = target.getGraphics();
            if (scissor) {
                int y0 = H - scY - scH;
                lg.setClip(scX, y0, scW, scH);
                if (lg.getClipWidth() <= 0 || lg.getClipHeight() <= 0) {
                    return false;
                }
            }
            g3d.bindTarget(lg, true, 0);
            bound = lg;
            return true;
        } catch (RuntimeException e) {
            failed = true;
            System.out.println("DoJa GL: M3G unavailable: " + e);
            return false;
        }
    }

    private void unbind() {
        if (bound != null) {
            g3d.releaseTarget();
            bound = null;
        }
        if (changed != null) {
            changed.run();
        }
    }

    private static int argb(float[] c) {
        int a = (int) (clamp01(c[3]) * 255f + 0.5f);
        int r = (int) (clamp01(c[0]) * 255f + 0.5f);
        int g = (int) (clamp01(c[1]) * 255f + 0.5f);
        int b = (int) (clamp01(c[2]) * 255f + 0.5f);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    public void clear(int mask) {
        if ((mask & (COLOR_BUFFER_BIT | DEPTH_BUFFER_BIT)) == 0 || !bind()) {
            return;
        }
        try {
            g3d.setViewport(0, 0, W, H);
            bg.setColor(argb(clearColor));
            bg.setColorClearEnable((mask & COLOR_BUFFER_BIT) != 0);
            bg.setDepthClearEnable((mask & DEPTH_BUFFER_BIT) != 0);
            g3d.setDepthRange(0f, clearDepth);
            g3d.clear(bg);
        } finally {
            unbind();
        }
    }

    private void prepareAppearance(Tex t) {
        if (app == null) {
            app = new Appearance();
            cm = new CompositingMode();
            pm = new PolygonMode();
            mat = new Material();
            fogObj = new Fog();
            app.setCompositingMode(cm);
            app.setPolygonMode(pm);
        }
        /* blending */
        int mode = CompositingMode.REPLACE;
        if (blend) {
            if (blendSrc == SRC_ALPHA && blendDst == ONE_MINUS_SRC_ALPHA) {
                mode = CompositingMode.ALPHA;
            } else if (blendSrc == ONE && blendDst == ONE_MINUS_SRC_ALPHA) {
                mode = CompositingMode.ALPHA;
            } else if (blendDst == ONE && (blendSrc == SRC_ALPHA || blendSrc == ONE)) {
                mode = CompositingMode.ALPHA_ADD;
            } else if ((blendSrc == DST_COLOR && blendDst == ZERO) || (blendSrc == ZERO && blendDst == SRC_COLOR)) {
                mode = CompositingMode.MODULATE;
            } else if (blendSrc == DST_COLOR && blendDst == SRC_COLOR) {
                mode = CompositingMode.MODULATE_X2;
            } else if (blendSrc == ZERO && blendDst == ONE) {
                mode = CompositingMode.ALPHA;   /* nothing drawn: alpha 0 below */
            } else if (blendSrc != ONE || blendDst != ZERO) {
                mode = CompositingMode.ALPHA;
            }
        }
        cm.setBlending(mode);
        float thr = 0f;
        if (alphaTest) {
            switch (alphaFunc) {
            case GREATER: thr = alphaRef + 1f / 255f; break;
            case GEQUAL: thr = alphaRef; break;
            case EQUAL: thr = alphaRef; break;
            case NEVER: thr = 1.01f; break;
            default: thr = 0f; break;
            }
        }
        cm.setAlphaThreshold(thr > 1f ? 1f : thr);
        boolean test = depthTest && depthFunc != ALWAYS;
        cm.setDepthTestEnable(test);
        cm.setDepthWriteEnable(depthTest && depthMask);
        cm.setDepthOffset(polyOffset ? offsetFactor : 0f, polyOffset ? offsetUnits : 0f);
        /* faces */
        if (cull) {
            pm.setCulling(cullFace == FRONT ? PolygonMode.CULL_FRONT : PolygonMode.CULL_BACK);
        } else {
            pm.setCulling(PolygonMode.CULL_NONE);
        }
        pm.setWinding(frontFace == CW ? PolygonMode.WINDING_CW : PolygonMode.WINDING_CCW);
        pm.setShading(shade == FLAT ? PolygonMode.SHADE_FLAT : PolygonMode.SHADE_SMOOTH);
        pm.setTwoSidedLightingEnable(twoSide);
        pm.setPerspectiveCorrectionEnable(true);
        /* texture */
        Texture2D t2 = texture2d ? texture2D(t) : null;
        if (t2 != null) {
            int f;
            switch (envMode) {
            case REPLACE: f = Texture2D.FUNC_REPLACE; break;
            case DECAL: f = Texture2D.FUNC_DECAL; break;
            case ADD: f = Texture2D.FUNC_ADD; break;
            case BLEND_ENV: f = Texture2D.FUNC_BLEND; break;
            default: f = Texture2D.FUNC_MODULATE; break;
            }
            t2.setBlending(f);
            t2.setBlendColor(argb(envColor) & 0xffffff);
            float[] tm = txStack[txTop];
            if (tm[0] != 1f || tm[5] != 1f || tm[12] != 0f || tm[13] != 0f || tm[1] != 0f || tm[4] != 0f) {
                Mat.toRowMajor(tm, rowMajor);
                xf.set(rowMajor);
                t2.setTransform(xf);
            } else {
                t2.setTransform(null);
            }
        }
        app.setTexture(0, t2);
        /* lighting */
        if (lighting) {
            mat.setColor(Material.AMBIENT, argb(mAmb));
            mat.setColor(Material.DIFFUSE, argb(mDif));
            mat.setColor(Material.SPECULAR, argb(mSpec));
            mat.setColor(Material.EMISSIVE, argb(mEmi));
            mat.setShininess(mShin < 0f ? 0f : mShin > 128f ? 128f : mShin);
            mat.setVertexColorTrackingEnable(colorMaterial);
            app.setMaterial(mat);
        } else {
            app.setMaterial(null);
        }
        /* fog */
        if (fogOn) {
            fogObj.setColor(argb(fogColor) & 0xffffff);
            if (fogMode == LINEAR) {
                fogObj.setMode(Fog.LINEAR);
                fogObj.setLinear(fogStart, fogEnd);
            } else {
                fogObj.setMode(Fog.EXPONENTIAL);
                fogObj.setDensity(fogDensity < 0f ? 0f : fogDensity);
            }
            app.setFog(fogObj);
        } else {
            app.setFog(null);
        }
    }

    private void prepareLights() {
        g3d.resetLights();
        if (!lighting) {
            return;
        }
        if (m3gLights == null) {
            m3gLights = new Light[8];
            ambientLight = new Light();
            ambientLight.setMode(Light.AMBIENT);
        }
        float ar = modelAmbient[0];
        float ag = modelAmbient[1];
        float ab = modelAmbient[2];
        for (int i = 0; i < 8; i++) {
            if (!lightOn[i]) {
                continue;
            }
            ar += lAmb[i][0];
            ag += lAmb[i][1];
            ab += lAmb[i][2];
            Light l = m3gLights[i];
            if (l == null) {
                l = new Light();
                m3gLights[i] = l;
            }
            float[] d = lDif[i];
            float mx = Math.max(d[0], Math.max(d[1], d[2]));
            float inten = mx > 1f ? mx : 1f;
            l.setColor(argb(new float[] { d[0] / inten, d[1] / inten, d[2] / inten, 1f }) & 0xffffff);
            l.setIntensity(inten);
            float[] p = lPos[i];
            lightXf.setIdentity();
            if (p[3] == 0f) {
                l.setMode(Light.DIRECTIONAL);
                orient(lightXf, -p[0], -p[1], -p[2]);
            } else {
                lightXf.postTranslate(p[0] / p[3], p[1] / p[3], p[2] / p[3]);
                if (lCut[i] < 180f) {
                    l.setMode(Light.SPOT);
                    l.setSpotAngle(lCut[i] > 90f ? 90f : lCut[i]);
                    l.setSpotExponent(lExp[i] > 128f ? 128f : lExp[i]);
                    orient(lightXf, lDir[i][0], lDir[i][1], lDir[i][2]);
                } else {
                    l.setMode(Light.OMNI);
                }
                l.setAttenuation(lAtt[i][0], lAtt[i][1], lAtt[i][2]);
            }
            g3d.addLight(l, lightXf);
        }
        ambientLight.setColor(argb(new float[] { Math.min(ar, 1f), Math.min(ag, 1f), Math.min(ab, 1f), 1f }) & 0xffffff);
        lightXf.setIdentity();
        g3d.addLight(ambientLight, lightXf);
    }

    /** rotates -Z onto (x, y, z) */
    private static void orient(Transform t, float x, float y, float z) {
        float len = (float) Math.sqrt(x * x + y * y + z * z);
        if (len == 0f) {
            return;
        }
        x /= len;
        y /= len;
        z /= len;
        /* axis = (-Z) x d, angle = acos(-Z . d) */
        float ax = y;
        float ay = -x;
        float az = 0f;
        float dot = -z;
        float s = (float) Math.sqrt(ax * ax + ay * ay);
        if (s < 1e-6f) {
            if (dot < 0f) {
                t.postRotate(180f, 1f, 0f, 0f);
            }
            return;
        }
        double ang = Math.toDegrees(com.funkey.doja.FMath.atan2(s, dot));
        t.postRotate((float) ang, ax / s, ay / s, az);
    }

    /** sends the expanded triangles (n vertices) and draws them */
    private void render(int n, float posScale, boolean colors, boolean normals, boolean textured,
            float texScale, float texBiasS, float texBiasT, Tex t) {
        vaPos.set(0, n, pos);
        vb.setPositions(vaPos, posScale, null);
        if (colors) {
            vaCol.set(0, n, col);
            vb.setColors(vaCol);
        } else {
            vb.setColors(null);
            vb.setDefaultColor(argb(color));
        }
        if (normals) {
            vaNrm.set(0, n, nrm);
            vb.setNormals(vaNrm);
        } else {
            vb.setNormals(null);
        }
        if (textured) {
            vaTex.set(0, n, tex);
            vb.setTexCoords(0, vaTex, texScale, new float[] { texBiasS, texBiasT });
        } else {
            vb.setTexCoords(0, null, 1f, null);
        }
        prepareAppearance(t);
        if (lighting && !normals) {
            /* no normals: the current normal for every vertex */
            for (int i = 0; i < n; i++) {
                nrm[i * 3] = (short) (normal[0] * 32767f);
                nrm[i * 3 + 1] = (short) (normal[1] * 32767f);
                nrm[i * 3 + 2] = (short) (normal[2] * 32767f);
            }
            vaNrm.set(0, n, nrm);
            vb.setNormals(vaNrm);
        }
        g3d.render(vb, strip(n / 3), app, xf);
    }

    private int[] corners = new int[3072];

    /**
     * Draws vertices of the current arrays (or the given immediate arrays)
     * in a primitive mode. idx: vertex numbers (null = first..first+count).
     */
    public void draw(int mode, int[] idx, int idxOff, int first, int count) {
        if (!vaOn) {
            return;
        }
        useC = caOn;
        useN = naOn;
        useT = taOn;
        drawWith(mode, idx, idxOff, first, count);
    }

    private void drawWith(int mode, int[] idx, int idxOff, int first, int count) {
        if (count <= 0 || !curV.valid()) {
            return;
        }
        if (mode == POINTS || mode == LINES || mode == LINE_LOOP || mode == LINE_STRIP) {
            drawLinesPoints(mode, idx, idxOff, first, count);
            return;
        }
        /* corner list */
        int tris;
        switch (mode) {
        case TRIANGLES: tris = count / 3; break;
        case TRIANGLE_STRIP:
        case TRIANGLE_FAN: tris = count - 2; break;
        case QUADS: tris = count / 4 * 2; break;
        default: setError(0x0500); return;
        }
        if (tris <= 0) {
            return;
        }
        int nc = tris * 3;
        if (corners.length < nc) {
            corners = new int[nc * 2];
        }
        int c = 0;
        for (int k = 0; k < tris; k++) {
            int a;
            int b;
            int d;
            switch (mode) {
            case TRIANGLES:
                a = k * 3; b = k * 3 + 1; d = k * 3 + 2;
                break;
            case TRIANGLE_STRIP:
                if ((k & 1) == 0) {
                    a = k; b = k + 1; d = k + 2;
                } else {
                    a = k + 1; b = k; d = k + 2;
                }
                break;
            case TRIANGLE_FAN:
                a = 0; b = k + 1; d = k + 2;
                break;
            default: {
                int q = (k >> 1) * 4;
                if ((k & 1) == 0) {
                    a = q; b = q + 1; d = q + 2;
                } else {
                    a = q; b = q + 2; d = q + 3;
                }
                break;
            }
            }
            corners[c++] = idx == null ? first + a : idx[idxOff + a];
            corners[c++] = idx == null ? first + b : idx[idxOff + b];
            corners[c++] = idx == null ? first + d : idx[idxOff + d];
        }
        if (nc > cap) {
            grow(nc);
        }
        float ps = fillPositions(nc);
        boolean colors = useC && curC.valid();
        if (colors) {
            fillColors(nc);
        }
        boolean normals = lighting && useN && curN.valid();
        if (normals) {
            fillNormals(nc);
        }
        Tex t = texture2d ? bound() : null;
        boolean textured = t != null && t.img != null && useT && curT.valid();
        if (textured) {
            fillTexCoords(nc);
        } else if (t != null && t.img != null && texture2d) {
            /* constant texture coordinate */
            textured = true;
            tScale = 1f;
            tBiasS = texCoord[0];
            tBiasT = texCoord[1];
            for (int i = 0; i < nc; i++) {
                tex[i * 2] = 0;
                tex[i * 2 + 1] = 0;
            }
        }
        if (!bind()) {
            return;
        }
        try {
            setupView();
            prepareLights();
            render(nc, ps, colors, normals, textured, tScale, tBiasS, tBiasT, t);
        } finally {
            unbind();
        }
    }

    /* split out of draw(): the JIT miscompiles very long float methods */
    private float tScale;
    private float tBiasS;
    private float tBiasT;

    private float fillPositions(int nc) {
        Attrib va = curV;
        int vs = va.size;
        float max = 0f;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            float w = vs == 4 ? va.get(v, 3, false) : 1f;
            if (w == 0f) {
                w = 1f;
            }
            for (int k = 0; k < 3 && k < vs; k++) {
                float x = va.get(v, k, false) / w;
                if (x < 0f) {
                    x = -x;
                }
                if (x > max) {
                    max = x;
                }
            }
        }
        float ps = max > 0f ? max / 32767f : 1f;
        float inv = 1f / ps;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            float w = vs == 4 ? va.get(v, 3, false) : 1f;
            if (w == 0f) {
                w = 1f;
            }
            float k = inv / w;
            pos[i * 3] = (short) (va.get(v, 0, false) * k);
            pos[i * 3 + 1] = (short) (vs > 1 ? va.get(v, 1, false) * k : 0f);
            pos[i * 3 + 2] = (short) (vs > 2 ? va.get(v, 2, false) * k : 0f);
        }
        return ps;
    }

    private void fillColors(int nc) {
        Attrib ca = curC;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            col[i * 4] = (byte) (clamp01(ca.get(v, 0, true)) * 255f);
            col[i * 4 + 1] = (byte) (clamp01(ca.get(v, 1, true)) * 255f);
            col[i * 4 + 2] = (byte) (clamp01(ca.get(v, 2, true)) * 255f);
            col[i * 4 + 3] = (byte) (ca.size > 3 ? clamp01(ca.get(v, 3, true)) * 255f : 255f);
        }
    }

    private void fillNormals(int nc) {
        Attrib na = curN;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            float x = na.get(v, 0, true);
            float y = na.get(v, 1, true);
            float z = na.get(v, 2, true);
            float l = (float) Math.sqrt(x * x + y * y + z * z);
            if (l > 0f) {
                x /= l;
                y /= l;
                z /= l;
            }
            nrm[i * 3] = (short) (x * 32767f);
            nrm[i * 3 + 1] = (short) (y * 32767f);
            nrm[i * 3 + 2] = (short) (z * 32767f);
        }
    }

    private void fillTexCoords(int nc) {
        Attrib ta = curT;
        float smin = Float.MAX_VALUE;
        float smax = -Float.MAX_VALUE;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            float s = ta.get(v, 0, false);
            float tt = ta.get(v, 1, false);
            smin = s < smin ? s : smin;
            smax = s > smax ? s : smax;
            smin = tt < smin ? tt : smin;
            smax = tt > smax ? tt : smax;
        }
        float mid = (smin + smax) / 2f;
        float range = (smax - smin) / 2f;
        tScale = range > 0f ? range / 32000f : 1f / 32000f;
        tBiasS = mid;
        tBiasT = mid;
        float ti = 1f / tScale;
        for (int i = 0; i < nc; i++) {
            int v = corners[i];
            tex[i * 2] = (short) ((ta.get(v, 0, false) - mid) * ti);
            tex[i * 2 + 1] = (short) ((ta.get(v, 1, false) - mid) * ti);
        }
    }

    private void setupView() {
        int y0 = H - vpY - vpH;
        if (vpW > 0 && vpH > 0) {
            g3d.setViewport(vpX, y0, vpW, vpH);
        }
        g3d.setDepthRange(depthNear, depthFar);
        Mat.toRowMajor(prStack[prTop], rowMajor);
        xf.set(rowMajor);
        camera.setGeneric(xf);
        g3d.setCamera(camera, null);
        Mat.toRowMajor(mvStack[mvTop], rowMajor);
        xf.set(rowMajor);
    }

    /* points and lines: 1-pixel screen-space quads */
    private final float[] clipA = new float[4];
    private final float[] clipB = new float[4];
    private final float[] mvp = new float[16];

    private boolean toScreen(int v, float[] out) {
        Attrib va = curV;
        float x = va.get(v, 0, false);
        float y = va.size > 1 ? va.get(v, 1, false) : 0f;
        float z = va.size > 2 ? va.get(v, 2, false) : 0f;
        Mat.transform(mvp, x, y, z, 1f, out);
        if (out[3] <= 0f) {
            return false;
        }
        float nx = out[0] / out[3];
        float ny = out[1] / out[3];
        float nz = out[2] / out[3];
        out[0] = vpX + (nx + 1f) * 0.5f * vpW;
        out[1] = H - (vpY + (ny + 1f) * 0.5f * vpH);
        out[2] = nz;
        return nz >= -1f && nz <= 1f;
    }

    private void drawLinesPoints(int mode, int[] idx, int idxOff, int first, int count) {
        Mat.mulTo(mvp, prStack[prTop], mvStack[mvTop]);
        int segs;
        if (mode == POINTS) {
            segs = count;
        } else if (mode == LINES) {
            segs = count / 2;
        } else if (mode == LINE_STRIP) {
            segs = count - 1;
        } else {
            segs = count;
        }
        if (segs <= 0) {
            return;
        }
        int nv = segs * 6;
        if (nv > cap) {
            grow(nv);
        }
        boolean colors = useC && curC.valid();
        Attrib colorArray = curC;
        int n = 0;
        for (int k = 0; k < segs; k++) {
            int a;
            int b;
            if (mode == POINTS) {
                a = b = k;
            } else if (mode == LINES) {
                a = k * 2;
                b = k * 2 + 1;
            } else {
                a = k;
                b = (k + 1) % count;
            }
            int va = idx == null ? first + a : idx[idxOff + a];
            int vb2 = idx == null ? first + b : idx[idxOff + b];
            if (!toScreen(va, clipA) || !toScreen(vb2, clipB)) {
                continue;
            }
            float dx = clipB[0] - clipA[0];
            float dy = clipB[1] - clipA[1];
            float len = (float) Math.sqrt(dx * dx + dy * dy);
            float nx;
            float ny;
            if (len < 0.5f) {
                dx = 1f;
                dy = 0f;
                len = 1f;
                clipB[0] = clipA[0] + 1f;
                clipB[1] = clipA[1];
            }
            nx = -dy / len * 0.5f;
            ny = dx / len * 0.5f;
            float[][] q = {
                { clipA[0] + nx, clipA[1] + ny, clipA[2] }, { clipB[0] + nx, clipB[1] + ny, clipB[2] },
                { clipB[0] - nx, clipB[1] - ny, clipB[2] }, { clipA[0] - nx, clipA[1] - ny, clipA[2] } };
            int[] fan = { 0, 1, 2, 0, 2, 3 };
            for (int f = 0; f < 6; f++) {
                float[] p = q[fan[f]];
                pos[n * 3] = (short) p[0];
                pos[n * 3 + 1] = (short) p[1];
                pos[n * 3 + 2] = (short) (p[2] * 16384f);
                int src = fan[f] == 0 || fan[f] == 3 ? va : vb2;
                if (colors) {
                    col[n * 4] = (byte) (clamp01(colorArray.get(src, 0, true)) * 255f);
                    col[n * 4 + 1] = (byte) (clamp01(colorArray.get(src, 1, true)) * 255f);
                    col[n * 4 + 2] = (byte) (clamp01(colorArray.get(src, 2, true)) * 255f);
                    col[n * 4 + 3] = (byte) (colorArray.size > 3 ? clamp01(colorArray.get(src, 3, true)) * 255f : 255f);
                }
                n++;
            }
        }
        if (n == 0 || !bind()) {
            return;
        }
        try {
            g3d.setViewport(0, 0, W, H);
            g3d.setDepthRange(depthNear, depthFar);
            for (int i = 0; i < 16; i++) {
                rowMajor[i] = 0f;
            }
            rowMajor[0] = 2f / W;
            rowMajor[3] = -1f;
            rowMajor[5] = -2f / H;
            rowMajor[7] = 1f;
            rowMajor[10] = 1f / 16384f;
            rowMajor[15] = 1f;
            xf.set(rowMajor);
            camera.setGeneric(xf);
            g3d.setCamera(camera, null);
            xf.setIdentity();
            g3d.resetLights();
            boolean savedLight = lighting;
            boolean savedTex = texture2d;
            lighting = false;
            texture2d = false;
            try {
                render(n, 1f, colors, false, false, 1f, 0f, 0f, null);
            } finally {
                lighting = savedLight;
                texture2d = savedTex;
            }
        } finally {
            unbind();
        }
    }
}
