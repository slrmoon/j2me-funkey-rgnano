package com.nttdocomo.opt.ui.j3d2;

import java.util.Hashtable;
import java.util.Vector;

import com.funkey.doja.gl.FkGL;
import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.Graphics;

/**
 * C3D: an OpenGL-1.x-like immediate API with 16.16 fixed-point values
 * and its own enum numbering (mapped to GL as games use it), drawn by the
 * shared GL layer on the canvas surface. 2D drawing is the ordinary
 * Graphics. Display lists record the calls and replay them.
 */
public final class C3DGraphics extends Graphics {
    private static final int[] C3DL = {
        0x0000, 0x0200, 0x0207, 0x0201, 0x0203, 0x0202, 0x0206, 0x0204,
        0x0205, 0x0000, 0x0001, 0x0003, 0x0002, 0x0004, 0x0005, 0x0006,
        0x0007, 0x0008, 0x0009, 0x0000, 0x0001, 0x0300, 0x0301, 0x0306,
        0x0302, 0x0303, 0x0307, 0x4000, 0x0100, 0x0400, 0x0001, 0x0000,
        0x0408, 0x1200, 0x1201, 0x1602, 0x1202, 0x1600, 0x0BE2, 0x0BC0,
        0x0B57, 0x0B44, 0x0B71, 0x0BD0, 0x0B60, 0x4000, 0x4001, 0x4002,
        0x4003, 0x4004, 0x4005, 0x4006, 0x4007, 0x0B50, 0x0B20, 0x0C11,
        0x0B90, 0x0DE1, 0x0C00, 0x0C01, 0x0B63, 0x0B64, 0x0B66, 0x0900,
        0x0901, 0x1F00, 0x1F02, 0x1203, 0x1206, 0x1204, 0x1205, 0x1207,
        0x1208, 0x1601, 0x1700, 0x1701, 0x1702, 0x1300, 0x0000, 0x0000,
        0x1E00, 0x1E01, 0x1E02, 0x1E03, 0x150A, 0x2300, 0x2201, 0x2200,
        0x2100, 0x2101, 0x2501, 0x2502, 0x2500, 0x2401, 0x2400, 0x2000,
        0x2001, 0x1907, 0x1908, 0x804F, 0x8050, 0x8051, 0x8056, 0x8058,
        0x8057, 0x1401, 0x2801, 0x2800, 0x2802, 0x2803, 0x2600, 0x2601,
        0x2900, 0x2901, 0x0000, 0x0501, 0x0502, 0x0505, 0x2402, 0x81F8,
        0x81FA, 0x81F9, 0x0B65, 0x0800, 0x0801, 0x0B62
    };
    private static final int[] CLEAR = { 0, 0x4000, 0x0100, 0, 0x0400 };

    private final C3DCanvas owner;
    private FkGL gl;

    /* immediate mode */
    private int beginMode = -1;
    private float[] vp = new float[96];
    private float[] vc = new float[128];
    private float[] vn = new float[96];
    private float[] vt = new float[64];
    private int nv;
    private final float[] color = { 1f, 1f, 1f, 1f };
    private final float[] normal = { 0f, 0f, 1f };
    private final float[] texc = { 0f, 0f };
    private boolean anyTex;

    /* display lists */
    private final Hashtable lists = new Hashtable();
    private Vector recording;
    private int recordingId;

    protected C3DGraphics() {
        owner = null;
    }

    C3DGraphics(C3DCanvas c) {
        owner = c;
        FkBridge.initOn(this, c);
    }

    private static final boolean TRACE = System.getProperty("doja.c3dtrace") != null;
    private static int traceLeft = 300000;

    static void tr(String s) {
        if (TRACE && traceLeft > 0) {
            traceLeft--;
            System.out.println("C3D " + s);
        }
    }

    private FkGL gl() {
        if (gl == null) {
            gl = FkBridge.glOf(this);
        }
        return gl;
    }

    static float fx(int v) {
        return v / 65536f;
    }

    private static int map(int v) {
        return v >= 0 && v < C3DL.length ? C3DL[v] : v;
    }

    /* state, light, material and texture enums line up one slot later from 31 */
    private static int mapShifted(int v) {
        if (v >= 31 && v + 1 < C3DL.length) {
            return C3DL[v + 1];
        }
        return map(v);
    }

    private static int mapMode(int v) {
        switch (v) {
        case 8: return FkGL.POINTS;
        case 9: return FkGL.LINES;
        case 12: return FkGL.TRIANGLES;
        case 14: return FkGL.TRIANGLE_STRIP;
        case 15: return FkGL.QUADS;
        default: return map(v);
        }
    }

    private static int mapCompare(int v) {
        switch (v) {
        case 1: return FkGL.ALWAYS;
        case 2: return FkGL.NEVER;
        case 3: return FkGL.LESS;
        case 4: return FkGL.LEQUAL;
        case 5: return FkGL.GREATER;
        case 6: return FkGL.GEQUAL;
        case 7: return FkGL.EQUAL;
        case 8: return FkGL.NOTEQUAL;
        default: return map(v);
        }
    }

    private static int mapCap(int v) {
        switch (v) {
        case 37: return FkGL.ALPHA_TEST;
        case 38: return FkGL.BLEND;
        case 39: return FkGL.COLOR_MATERIAL;
        case 40: return FkGL.DEPTH_TEST;
        case 41: return FkGL.CULL_FACE;
        case 52: return FkGL.LIGHTING;
        case 56: return FkGL.TEXTURE_2D;
        default:
            if (v >= 44 && v <= 51) {
                return FkGL.LIGHT0 + (v - 44);
            }
            return mapShifted(v);
        }
    }

    private static int mapLight(int v) {
        if (v >= 44 && v <= 51) {
            return FkGL.LIGHT0 + (v - 44);
        }
        return mapShifted(v);
    }

    private static int mapLightParam(int v) {
        switch (v) {
        case 32: return FkGL.AMBIENT;
        case 33: return FkGL.DIFFUSE;
        case 35: return FkGL.SPECULAR;
        case 66: return FkGL.POSITION;
        default: return mapShifted(v);
        }
    }

    private static int mapMaterialParam(int v) {
        switch (v) {
        case 34: return FkGL.AMBIENT_AND_DIFFUSE;
        case 35: return FkGL.SPECULAR;
        case 36: return FkGL.EMISSION;
        case 72: return FkGL.SHININESS;
        default: return mapShifted(v);
        }
    }

    private static int mapMatrix(int v) {
        switch (v) {
        case 73: return FkGL.MODELVIEW;
        case 74: return FkGL.PROJECTION;
        case 75: return FkGL.TEXTURE;
        default: return map(v);
        }
    }

    private static int mapTexParamName(int v) {
        switch (v) {
        case 105: return FkGL.TEXTURE_WRAP_S;
        case 106: return FkGL.TEXTURE_WRAP_T;
        default: return mapShifted(v);
        }
    }

    private static int mapTexParamValue(int v) {
        switch (v) {
        case 109: return FkGL.REPEAT;
        case 110: return FkGL.CLAMP_TO_EDGE;
        default: {
            int g = mapShifted(v);
            return g == 0x2900 ? FkGL.CLAMP_TO_EDGE : g;
        }
        }
    }

    private static float[] fx(int[] v) {
        float[] f = new float[v.length];
        for (int i = 0; i < v.length; i++) {
            f[i] = v[i] / 65536f;
        }
        return f;
    }

    /* ------------------------------------------------------------------ */
    /* display lists                                                       */

    private static final int OP_BEGIN = 1, OP_END = 2, OP_VERTEX = 3, OP_COLOR = 4, OP_NORMAL = 5,
            OP_TEXCOORD = 6, OP_MATRIXMODE = 7, OP_LOADIDENTITY = 8, OP_LOADMATRIX = 9,
            OP_MULTMATRIX = 10, OP_LOOKAT = 11, OP_PERSPECTIVE = 12, OP_PUSH = 13, OP_POP = 14,
            OP_TRANSLATE = 15, OP_ROTATE = 16, OP_SCALE = 17, OP_ENABLE = 18, OP_DISABLE = 19,
            OP_COLORMATERIAL = 20, OP_DEPTHMASK = 21, OP_DEPTHFUNC = 22, OP_BLENDFUNC = 23,
            OP_FOG = 24, OP_FOGV = 25, OP_LIGHT = 26, OP_MATERIAL = 27, OP_LIGHTMODEL = 28,
            OP_FRONTFACE = 29, OP_BINDTEXTURE = 30, OP_TEXPARAM = 31, OP_TEXENV = 32,
            OP_VIEWPORT = 33, OP_CALLLIST = 34, OP_CLEARCOLOR = 35, OP_ALPHAFUNC = 36,
            OP_CLEAR = 37, OP_CLEAR2 = 38, OP_TEXIMAGE = 39;

    private boolean rec(int op, int[] args, Object data) {
        if (recording == null) {
            return false;
        }
        recording.addElement(new Object[] { new Integer(op), args, data });
        return true;
    }

    private boolean rec(int op) {
        return rec(op, null, null);
    }

    private boolean rec(int op, int a) {
        return recording != null && rec(op, new int[] { a }, null);
    }

    private boolean rec(int op, int a, int b) {
        return recording != null && rec(op, new int[] { a, b }, null);
    }

    private boolean rec(int op, int a, int b, int c) {
        return recording != null && rec(op, new int[] { a, b, c }, null);
    }

    private boolean rec(int op, int a, int b, int c, int d) {
        return recording != null && rec(op, new int[] { a, b, c, d }, null);
    }

    private static int[] copy(int[] v) {
        if (v == null) {
            return null;
        }
        int[] c = new int[v.length];
        System.arraycopy(v, 0, c, 0, v.length);
        return c;
    }

    private void replay(Object[] c) {
        int op = ((Integer) c[0]).intValue();
        int[] a = (int[]) c[1];
        switch (op) {
        case OP_BEGIN: begin(a[0]); break;
        case OP_END: end(); break;
        case OP_VERTEX: vertex(a[0], a[1], a[2]); break;
        case OP_COLOR: color(a[0], a[1], a[2], a[3]); break;
        case OP_NORMAL: normal(a[0], a[1], a[2]); break;
        case OP_TEXCOORD: texCoord(a[0], a[1]); break;
        case OP_MATRIXMODE: matrixMode(a[0]); break;
        case OP_LOADIDENTITY: loadIdentity(); break;
        case OP_LOADMATRIX: loadMatrix(a); break;
        case OP_MULTMATRIX: multiMatrix(a); break;
        case OP_LOOKAT: lookAt(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); break;
        case OP_PERSPECTIVE: perspective(a[0], a[1], a[2], a[3]); break;
        case OP_PUSH: pushMatrix(); break;
        case OP_POP: popMatrix(); break;
        case OP_TRANSLATE: translate(a[0], a[1], a[2]); break;
        case OP_ROTATE: rotate(a[0], a[1], a[2], a[3]); break;
        case OP_SCALE: scale(a[0], a[1], a[2]); break;
        case OP_ENABLE: enable(a[0]); break;
        case OP_DISABLE: disable(a[0]); break;
        case OP_COLORMATERIAL: colorMaterial(a[0], a[1]); break;
        case OP_DEPTHMASK: depthMask(a[0]); break;
        case OP_DEPTHFUNC: depthFunc(a[0]); break;
        case OP_BLENDFUNC: blendFunc(a[0], a[1]); break;
        case OP_FOG: fog(a[0], a[1]); break;
        case OP_FOGV: fogv(a[0], (int[]) c[2]); break;
        case OP_LIGHT: light(a[0], a[1], (int[]) c[2]); break;
        case OP_MATERIAL: material(a[0], a[1], (int[]) c[2]); break;
        case OP_LIGHTMODEL: lightModeli(a[0], a[1]); break;
        case OP_FRONTFACE: frontFace(a[0]); break;
        case OP_BINDTEXTURE: bindTexture(a[0], a[1]); break;
        case OP_TEXPARAM: texParameter(a[0], a[1], a[2]); break;
        case OP_TEXENV: texEnvi(a[0], a[1], a[2]); break;
        case OP_VIEWPORT: viewport(a[0], a[1], a[2], a[3]); break;
        case OP_CALLLIST: callList(a[0]); break;
        case OP_CLEARCOLOR: clearColor(a[0], a[1], a[2], a[3]); break;
        case OP_ALPHAFUNC: alphaFunc(a[0], a[1]); break;
        case OP_CLEAR: clear(a[0]); break;
        case OP_CLEAR2: clear(a[0], a[1]); break;
        case OP_TEXIMAGE:
            texImage2D(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], (byte[]) c[2]);
            break;
        default: break;
        }
    }

    public void newList(int id, int mode) {
        tr("newList " + id + " " + mode);
        recording = new Vector();
        recordingId = id;
    }

    public void endList() {
        tr("endList " + "");
        if (recording != null) {
            lists.put(new Integer(recordingId), recording);
            recording = null;
        }
    }

    public void deleteLists(int id) {
        tr("deleteLists " + id);
        lists.remove(new Integer(id));
    }

    public void deleteLists(int id, int range) {
        tr("deleteLists " + id + " " + range);
        for (int i = 0; i < range; i++) {
            lists.remove(new Integer(id + i));
        }
    }

    public void callList(int id) {
        tr("callList " + id);
        if (rec(OP_CALLLIST, id)) {
            return;
        }
        Vector v = (Vector) lists.get(new Integer(id));
        if (v == null) {
            return;
        }
        for (int i = 0; i < v.size(); i++) {
            replay((Object[]) v.elementAt(i));
        }
    }

    public void callList(C3DFigure figure) {
        tr("callListFigure");
        if (figure != null) {
            figure.render(this);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 2D / frame                                                          */

    public void set3DMode(boolean enabled) {
        tr("set3DMode " + enabled);
    }

    public void beginDrawing() {
        tr("beginDrawing " + "");
    }

    public void endDrawing() {
        tr("endDrawing " + "");
    }

    public void finish() {
        tr("finish " + "");
    }

    public void flush() {
        tr("flush " + "");
        super.flush();
    }

    /* ------------------------------------------------------------------ */
    /* 3D                                                                  */

    public void clear(int mask) {
        tr("clear " + mask);
        if (rec(OP_CLEAR, mask)) {
            return;
        }
        int m = mask >= 0 && mask < CLEAR.length ? CLEAR[mask] : 0;
        if (mask == 3) {
            m = 0x4100;
        }
        gl().clear(m);
    }

    public void clear(int maskA, int maskB) {
        tr("clear " + maskA + " " + maskB);
        if (rec(OP_CLEAR2, maskA, maskB)) {
            return;
        }
        int a = maskA >= 0 && maskA < CLEAR.length ? CLEAR[maskA] : 0;
        int b = maskB >= 0 && maskB < CLEAR.length ? CLEAR[maskB] : 0;
        gl().clear(a | b);
    }

    public void clearColor(int r, int g, int b, int a) {
        tr("clearColor " + r + " " + g + " " + b + " " + a);
        if (rec(OP_CLEARCOLOR, r, g, b, a)) {
            return;
        }
        gl().clearColor(fx(r), fx(g), fx(b), fx(a));
    }

    public void clearDepth(int depth) {
        tr("clearDepth " + depth);
        gl().clearDepth(fx(depth));
    }

    public void clearStencil(int stencil) {
        tr("clearStencil " + stencil);
    }

    public void alphaFunc(int func, int ref) {
        tr("alphaFunc " + func + " " + ref);
        if (rec(OP_ALPHAFUNC, func, ref)) {
            return;
        }
        gl().alphaFunc(mapCompare(func), fx(ref));
    }

    public void begin(int mode) {
        tr("begin " + mode);
        if (rec(OP_BEGIN, mode)) {
            return;
        }
        beginMode = mapMode(mode);
        nv = 0;
        anyTex = false;
    }

    public void end() {
        tr("end " + "");
        if (rec(OP_END)) {
            return;
        }
        if (beginMode < 0) {
            return;
        }
        int mode = beginMode;
        beginMode = -1;
        if (nv == 0) {
            return;
        }
        gl().drawImmediate(mode, nv, vp, vc, vn, anyTex ? vt : null);
        nv = 0;
    }

    private void grow() {
        if (nv * 3 + 3 > vp.length) {
            int n = vp.length / 3 * 2;
            float[] a = new float[n * 3];
            System.arraycopy(vp, 0, a, 0, vp.length);
            vp = a;
            a = new float[n * 4];
            System.arraycopy(vc, 0, a, 0, vc.length);
            vc = a;
            a = new float[n * 3];
            System.arraycopy(vn, 0, a, 0, vn.length);
            vn = a;
            a = new float[n * 2];
            System.arraycopy(vt, 0, a, 0, vt.length);
            vt = a;
        }
    }

    public void vertex(int x, int y, int z) {
        tr("vertex " + x + " " + y + " " + z);
        if (rec(OP_VERTEX, x, y, z)) {
            return;
        }
        if (beginMode < 0) {
            return;
        }
        grow();
        vp[nv * 3] = fx(x);
        vp[nv * 3 + 1] = fx(y);
        vp[nv * 3 + 2] = fx(z);
        vc[nv * 4] = color[0];
        vc[nv * 4 + 1] = color[1];
        vc[nv * 4 + 2] = color[2];
        vc[nv * 4 + 3] = color[3];
        vn[nv * 3] = normal[0];
        vn[nv * 3 + 1] = normal[1];
        vn[nv * 3 + 2] = normal[2];
        vt[nv * 2] = texc[0];
        vt[nv * 2 + 1] = texc[1];
        nv++;
    }

    public void vertexv(int[] v) {
        tr("vertexv " + (v == null ? "null" : "[" + v.length + "]"));
        vertex(v[0], v[1], v.length > 2 ? v[2] : 0);
    }

    public void color(int r, int g, int b, int a) {
        tr("color " + r + " " + g + " " + b + " " + a);
        if (rec(OP_COLOR, r, g, b, a)) {
            return;
        }
        color[0] = fx(r);
        color[1] = fx(g);
        color[2] = fx(b);
        color[3] = fx(a);
        if (beginMode < 0) {
            gl().color(color[0], color[1], color[2], color[3]);
        }
    }

    public void color(int[] v) {
        tr("color " + (v == null ? "null" : "[" + v.length + "]"));
        color(v[0], v[1], v[2], v.length > 3 ? v[3] : 65536);
    }

    public void colorv(int[] v) {
        tr("colorv " + (v == null ? "null" : "[" + v.length + "]"));
        color(v);
    }

    public void normal(int x, int y, int z) {
        tr("normal " + x + " " + y + " " + z);
        if (rec(OP_NORMAL, x, y, z)) {
            return;
        }
        normal[0] = fx(x);
        normal[1] = fx(y);
        normal[2] = fx(z);
        gl().normal(normal[0], normal[1], normal[2]);
    }

    public void normalv(int[] v) {
        tr("normalv " + (v == null ? "null" : "[" + v.length + "]"));
        normal(v[0], v[1], v[2]);
    }

    public void texCoord(int s, int t) {
        tr("texCoord " + s + " " + t);
        if (rec(OP_TEXCOORD, s, t)) {
            return;
        }
        texc[0] = fx(s);
        texc[1] = fx(t);
        anyTex = true;
    }

    public void texCoordv(int[] v) {
        tr("texCoordv " + (v == null ? "null" : "[" + v.length + "]"));
        texCoord(v[0], v[1]);
    }

    public void matrixMode(int mode) {
        tr("matrixMode " + mode);
        if (rec(OP_MATRIXMODE, mode)) {
            return;
        }
        gl().matrixMode(mapMatrix(mode));
    }

    public void loadIdentity() {
        tr("loadIdentity " + "");
        if (rec(OP_LOADIDENTITY)) {
            return;
        }
        gl().loadIdentity();
    }

    public void loadMatrix(int[] m) {
        tr("loadMatrix " + (m == null ? "null" : "[" + m.length + "]"));
        if (rec(OP_LOADMATRIX, copy(m), null)) {
            return;
        }
        gl().loadMatrix(fx(m));
    }

    public void multiMatrix(int[] m) {
        tr("multiMatrix " + (m == null ? "null" : "[" + m.length + "]"));
        if (rec(OP_MULTMATRIX, copy(m), null)) {
            return;
        }
        gl().multMatrix(fx(m));
    }

    public void lookAt(int ex, int ey, int ez, int cx, int cy, int cz, int ux, int uy, int uz) {
        tr("lookAt " + ex + " " + ey + " " + ez + " " + cx + " " + cy + " " + cz + " " + ux + " " + uy + " " + uz);
        if (rec(OP_LOOKAT, new int[] { ex, ey, ez, cx, cy, cz, ux, uy, uz }, null)) {
            return;
        }
        gl().matrixMode(FkGL.MODELVIEW);
        gl().loadIdentity();
        gl().lookAt(fx(ex), fx(ey), fx(ez), fx(cx), fx(cy), fx(cz), fx(ux), fx(uy), fx(uz));
    }

    public void perspective(int fovy, int aspect, int zNear, int zFar) {
        tr("perspective " + fovy + " " + aspect + " " + zNear + " " + zFar);
        if (rec(OP_PERSPECTIVE, fovy, aspect, zNear, zFar)) {
            return;
        }
        float n = fx(zNear);
        float top = (float) (n * Math.tan(fx(fovy) * Math.PI / 360.0));
        float right = top * fx(aspect);
        FkGL g = gl();
        g.matrixMode(FkGL.PROJECTION);
        g.loadIdentity();
        g.frustum(-right, right, -top, top, n, fx(zFar));
        g.matrixMode(FkGL.MODELVIEW);
        g.loadIdentity();
    }

    public void pushMatrix() {
        tr("pushMatrix " + "");
        if (rec(OP_PUSH)) {
            return;
        }
        gl().pushMatrix();
    }

    public void popMatrix() {
        tr("popMatrix " + "");
        if (rec(OP_POP)) {
            return;
        }
        gl().popMatrix();
    }

    public void translate(int x, int y, int z) {
        tr("translate " + x + " " + y + " " + z);
        if (rec(OP_TRANSLATE, x, y, z)) {
            return;
        }
        gl().translate(fx(x), fx(y), fx(z));
    }

    public void rotate(int angle, int x, int y, int z) {
        tr("rotate " + angle + " " + x + " " + y + " " + z);
        if (rec(OP_ROTATE, angle, x, y, z)) {
            return;
        }
        gl().rotate(fx(angle), fx(x), fx(y), fx(z));
    }

    public void scale(int x, int y, int z) {
        tr("scale " + x + " " + y + " " + z);
        if (rec(OP_SCALE, x, y, z)) {
            return;
        }
        gl().scale(fx(x), fx(y), fx(z));
    }

    public void enable(int cap) {
        tr("enable " + cap);
        if (rec(OP_ENABLE, cap)) {
            return;
        }
        gl().enable(mapCap(cap), true);
    }

    public void disable(int cap) {
        tr("disable " + cap);
        if (rec(OP_DISABLE, cap)) {
            return;
        }
        gl().enable(mapCap(cap), false);
    }

    public void colorMaterial(int face, int mode) {
        tr("colorMaterial " + face + " " + mode);
        if (rec(OP_COLORMATERIAL, face, mode)) {
            return;
        }
        gl().color(color[0], color[1], color[2], color[3]);
    }

    public void depthMask(int enabled) {
        tr("depthMask " + enabled);
        if (rec(OP_DEPTHMASK, enabled)) {
            return;
        }
        gl().depthMask(map(enabled) != 0);
    }

    public void depthFunc(int func) {
        tr("depthFunc " + func);
        if (rec(OP_DEPTHFUNC, func)) {
            return;
        }
        gl().depthFunc(mapCompare(func));
    }

    public void blendFunc(int src, int dst) {
        tr("blendFunc " + src + " " + dst);
        if (rec(OP_BLENDFUNC, src, dst)) {
            return;
        }
        gl().blendFunc(map(src), map(dst));
    }

    public void fog(int pname, int value) {
        tr("fog " + pname + " " + value);
        if (rec(OP_FOG, pname, value)) {
            return;
        }
        int p = mapShifted(pname);
        float v = p == FkGL.FOG_MODE ? (float) mapShifted(value) : fx(value);
        gl().fog(p, new float[] { v });
    }

    public void fogv(int pname, int[] values) {
        tr("fogv " + pname + " " + (values == null ? "null" : "[" + values.length + "]"));
        if (rec(OP_FOGV, new int[] { pname }, copy(values))) {
            return;
        }
        gl().fog(mapShifted(pname), fx(values));
    }

    public void light(int light, int pname, int[] values) {
        tr("light " + light + " " + pname + " " + (values == null ? "null" : "[" + values.length + "]"));
        if (rec(OP_LIGHT, new int[] { light, pname }, copy(values))) {
            return;
        }
        gl().light(mapLight(light), mapLightParam(pname), fx(values));
    }

    public void material(int face, int pname, int[] values) {
        tr("material " + face + " " + pname + " " + (values == null ? "null" : "[" + values.length + "]"));
        if (rec(OP_MATERIAL, new int[] { face, pname }, copy(values))) {
            return;
        }
        gl().material(FkGL.FRONT_AND_BACK, mapMaterialParam(pname), fx(values));
    }

    public void lightModeli(int pname, int value) {
        tr("lightModeli " + pname + " " + value);
        if (rec(OP_LIGHTMODEL, pname, value)) {
            return;
        }
        if (pname == 118) {
            gl().lightModel(FkGL.LIGHT_MODEL_TWO_SIDE, new float[] { value == 120 ? 1f : 0f });
        }
    }

    public void frontFace(int mode) {
        tr("frontFace " + mode);
        if (rec(OP_FRONTFACE, mode)) {
            return;
        }
        gl().frontFace(mode == 62 ? FkGL.CW : mode == 63 ? FkGL.CCW : mapShifted(mode));
    }

    public int getError() {
        tr("getError " + "");
        return gl().getError();
    }

    public String getString(int name) {
        tr("getString " + name);
        int g = mapShifted(name);
        if (g == 0x1F00) {
            return "FunKey";
        }
        if (g == 0x1F02) {
            return "1.0";
        }
        return "";
    }

    public void bindTexture(int target, int textureId) {
        tr("bindTexture " + target + " " + textureId);
        if (rec(OP_BINDTEXTURE, target, textureId)) {
            return;
        }
        gl().bindTexture(textureId == 0 ? 0 : textureId + 0x10000);
    }

    public void texImage2D(int target, int level, int internalFormat, int width, int height, int border,
            int format, int type, byte[] data) {
        tr("texImage2D " + target + " " + level + " " + internalFormat + " " + width + " " + height + " " + border + " " + format + " " + type + " " + (data == null ? "null" : "[" + data.length + "]"));
        if (data == null) {
            throw new NullPointerException("data");
        }
        if (recording != null) {
            rec(OP_TEXIMAGE, new int[] { target, level, internalFormat, width, height, border, format, type }, data);
            return;
        }
        int n = width * height;
        if (n <= 0) {
            return;
        }
        int bpp = data.length / n;
        if (bpp != 3 && bpp != 4) {
            bpp = format == 98 ? 4 : 3;
        }
        byte[] rgba = new byte[n * 4];
        for (int i = 0; i < n && i * bpp + bpp <= data.length; i++) {
            rgba[i * 4] = data[i * bpp];
            rgba[i * 4 + 1] = data[i * bpp + 1];
            rgba[i * 4 + 2] = data[i * bpp + 2];
            rgba[i * 4 + 3] = bpp == 4 ? data[i * bpp + 3] : (byte) 255;
        }
        gl().texImage(level, width, height, rgba, bpp == 4);
    }

    public void texSubImage2D0(int target, int level, int xoffset, int yoffset, int width, int height,
            int format, int type, byte[] data) {
        tr("texSubImage2D0 " + target + " " + level + " " + xoffset + " " + yoffset + " " + width + " " + height + " " + format + " " + type + " " + (data == null ? "null" : "[" + data.length + "]"));
    }

    public void texParameter(int target, int pname, int param) {
        tr("texParameter " + target + " " + pname + " " + param);
        if (rec(OP_TEXPARAM, target, pname, param)) {
            return;
        }
        gl().texParameter(mapTexParamName(pname), mapTexParamValue(param));
    }

    public void texEnvf(int target, int pname, int param) {
        tr("texEnvf " + target + " " + pname + " " + param);
        texEnvi(target, pname, param);
    }

    public void texEnvi(int target, int pname, int param) {
        tr("texEnvi " + target + " " + pname + " " + param);
        if (rec(OP_TEXENV, target, pname, param)) {
            return;
        }
        gl().texEnv(mapShifted(pname), new float[] { (float) mapShifted(param) });
    }

    public void texGenf(int coord, int pname, int param) {
        tr("texGenf " + coord + " " + pname + " " + param);
    }

    public void texGeni0(int coord, int pname, int param) {
        tr("texGeni0 " + coord + " " + pname + " " + param);
    }

    public void colorMask(int r, int g, int b, int a) {
        tr("colorMask " + r + " " + g + " " + b + " " + a);
    }

    public void scissorMethod(int x, int y, int width, int height) {
        tr("scissorMethod " + x + " " + y + " " + width + " " + height);
        gl().scissor(x, y, width, height);
    }

    public void stencilFunc(int func, int ref, int mask) {
        tr("stencilFunc " + func + " " + ref + " " + mask);
    }

    public void stencilMask(int mask) {
        tr("stencilMask " + mask);
    }

    public void stencilOp(int fail, int zFail, int zPass) {
        tr("stencilOp " + fail + " " + zFail + " " + zPass);
    }

    public void viewport(int x, int y, int width, int height) {
        tr("viewport " + x + " " + y + " " + width + " " + height);
        if (rec(OP_VIEWPORT, x, y, width, height)) {
            return;
        }
        gl().viewport(x, y, width, height);
    }

    public void deleteTextures(int[] ids) {
        tr("deleteTextures " + (ids == null ? "null" : "[" + ids.length + "]"));
        deleteTextures(ids == null ? 0 : ids.length, ids);
    }

    public void deleteTextures(int count, int[] ids) {
        tr("deleteTextures " + count + " " + (ids == null ? "null" : "[" + ids.length + "]"));
        if (ids == null) {
            return;
        }
        int[] g = new int[count];
        for (int i = 0; i < count && i < ids.length; i++) {
            g[i] = ids[i] + 0x10000;
        }
        gl().deleteTextures(g, 0, g.length);
    }

    public void drawImage(com.nttdocomo.ui.Image img, int x, int y) {
        tr("drawImage " + x + " " + y);
        super.drawImage(img, x, y);
    }

    public void fillRect(int x, int y, int w, int h) {
        tr("fillRect " + x + " " + y + " " + w + " " + h);
        super.fillRect(x, y, w, h);
    }

    public void unlock(boolean forced) {
        tr("unlock " + forced);
        super.unlock(forced);
    }

    public void sphere() {
        tr("sphere " + "");
    }
}
