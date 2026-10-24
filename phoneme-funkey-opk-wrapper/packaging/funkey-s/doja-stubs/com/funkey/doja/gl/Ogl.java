package com.funkey.doja.gl;

import com.nttdocomo.ui.ogl.ByteBuffer;
import com.nttdocomo.ui.ogl.DirectBuffer;
import com.nttdocomo.ui.ogl.DirectBufferFactory;
import com.nttdocomo.ui.ogl.ShortBuffer;

/** com.nttdocomo.ui.ogl.GraphicsOGL calls on an FkGL (generated delegation from Graphics). */
public final class Ogl {
    private Ogl() {
    }

    public static void beginDrawing(FkGL gl) {
    }

    public static void endDrawing(FkGL gl) {
    }

    public static void glActiveTexture(FkGL gl, int texture) {
    }

    public static void glAlphaFunc(FkGL gl, int func, float ref) {
        gl.alphaFunc(func, ref);
    }

    public static void glBindTexture(FkGL gl, int target, int texture) {
        gl.bindTexture(texture);
    }

    public static void glBlendFunc(FkGL gl, int sfactor, int dfactor) {
        gl.blendFunc(sfactor, dfactor);
    }

    public static void glClear(FkGL gl, int mask) {
        gl.clear(mask);
    }

    public static void glClearColor(FkGL gl, float red, float green, float blue, float alpha) {
        gl.clearColor(red, green, blue, alpha);
    }

    public static void glClearDepthf(FkGL gl, float depth) {
        gl.clearDepth(depth);
    }

    public static void glClearStencil(FkGL gl, int s) {
    }

    public static void glClientActiveTexture(FkGL gl, int texture) {
    }

    public static void glColor4f(FkGL gl, float red, float green, float blue, float alpha) {
        gl.color(red, green, blue, alpha);
    }

    public static void glColorMask(FkGL gl, boolean red, boolean green, boolean blue, boolean alpha) {
    }

    public static void glColorPointer(FkGL gl, int size, int type, int stride, DirectBuffer pointer) {
        pointer(gl.colorArray, size, type, stride, pointer);
    }

    public static void glColorPointer(FkGL gl, int size, int type, int stride, int pointer) {
        vbo(gl, gl.colorArray, size, type, stride, pointer);
    }

    public static void glCompressedTexImage2D(FkGL gl, int target, int level, int internalformat, int width, int height, int border, int imageSize, DirectBuffer data) {
    }

    public static void glCompressedTexSubImage2D(FkGL gl, int target, int level, int xoffset, int yoffset, int width, int height, int format, int imageSize, DirectBuffer data) {
    }

    public static void glCopyTexImage2D(FkGL gl, int target, int level, int internalformat, int x, int y, int width, int height, int border) {
    }

    public static void glCopyTexSubImage2D(FkGL gl, int target, int level, int xoffset, int yoffset, int x, int y, int width, int height) {
    }

    public static void glCullFace(FkGL gl, int mode) {
        gl.cullFace(mode);
    }

    public static void glDeleteTextures(FkGL gl, int n, int[] textures) {
        gl.deleteTextures(textures, 0, n);
    }

    public static void glDepthFunc(FkGL gl, int func) {
        gl.depthFunc(func);
    }

    public static void glDepthMask(FkGL gl, boolean flag) {
        gl.depthMask(flag);
    }

    public static void glDepthRangef(FkGL gl, float zNear, float zFar) {
        gl.depthRange(zNear, zFar);
    }

    public static void glDisable(FkGL gl, int cap) {
        gl.enable(cap, false);
    }

    public static void glDisableClientState(FkGL gl, int array) {
        gl.clientState(array, false);
    }

    public static void glDrawArrays(FkGL gl, int mode, int first, int count) {
        gl.draw(mode, null, 0, first, count);
    }

    public static void glDrawElements(FkGL gl, int mode, int count, int type, DirectBuffer indices) {
        drawElements(gl, mode, count, type, indices, 0);
    }

    public static void glDrawElements(FkGL gl, int mode, int count, int type, int indices) {
        drawElementsVbo(gl, mode, count, type, indices);
    }

    public static void glEnable(FkGL gl, int cap) {
        gl.enable(cap, true);
    }

    public static void glEnableClientState(FkGL gl, int array) {
        gl.clientState(array, true);
    }

    public static void glFlush(FkGL gl) {
    }

    public static void glFogf(FkGL gl, int pname, float param) {
        gl.fog(pname, new float[] { param });
    }

    public static void glFogfv(FkGL gl, int pname, float[] params) {
        gl.fog(pname, params);
    }

    public static void glFrontFace(FkGL gl, int mode) {
        gl.frontFace(mode);
    }

    public static void glFrustumf(FkGL gl, float left, float right, float bottom, float top, float zNear, float zFar) {
        gl.frustum(left, right, bottom, top, zNear, zFar);
    }

    public static void glGenTextures(FkGL gl, int n, int[] textures) {
        gl.genTextures(textures, 0, n);
    }

    public static int glGetError(FkGL gl) {
        return gl.getError();
    }

    public static void glGetIntegerv(FkGL gl, int pname, int[] params) {
        getIntegerv(gl, pname, params);
    }

    public static void glHint(FkGL gl, int target, int mode) {
    }

    public static void glLightModelf(FkGL gl, int pname, float param) {
        gl.lightModel(pname, new float[] { param });
    }

    public static void glLightModelfv(FkGL gl, int pname, float[] params) {
        gl.lightModel(pname, params);
    }

    public static void glLightf(FkGL gl, int light, int pname, float param) {
        gl.light(light, pname, new float[] { param });
    }

    public static void glLightfv(FkGL gl, int light, int pname, float[] params) {
        gl.light(light, pname, params);
    }

    public static void glLineWidth(FkGL gl, float width) {
    }

    public static void glLoadIdentity(FkGL gl) {
        gl.loadIdentity();
    }

    public static void glLoadMatrixf(FkGL gl, float[] m) {
        gl.loadMatrix(m);
    }

    public static void glLogicOp(FkGL gl, int opcode) {
    }

    public static void glMaterialf(FkGL gl, int face, int pname, float param) {
        gl.material(face, pname, new float[] { param });
    }

    public static void glMaterialfv(FkGL gl, int face, int pname, float[] params) {
        gl.material(face, pname, params);
    }

    public static void glMatrixMode(FkGL gl, int mode) {
        gl.matrixMode(mode);
    }

    public static void glMultMatrixf(FkGL gl, float[] m) {
        gl.multMatrix(m);
    }

    public static void glMultiTexCoord4f(FkGL gl, int target, float s, float t, float r, float q) {
        gl.texCoord(s, t);
    }

    public static void glNormal3f(FkGL gl, float nx, float ny, float nz) {
        gl.normal(nx, ny, nz);
    }

    public static void glNormalPointer(FkGL gl, int type, int stride, DirectBuffer pointer) {
        pointer(gl.normalArray, 3, type, stride, pointer);
    }

    public static void glNormalPointer(FkGL gl, int type, int stride, int pointer) {
        vbo(gl, gl.normalArray, 3, type, stride, pointer);
    }

    public static void glOrthof(FkGL gl, float left, float right, float bottom, float top, float zNear, float zFar) {
        gl.ortho(left, right, bottom, top, zNear, zFar);
    }

    public static void glPixelStorei(FkGL gl, int pname, int param) {
    }

    public static void glPointSize(FkGL gl, float size) {
    }

    public static void glPolygonOffset(FkGL gl, float factor, float units) {
        gl.polygonOffset(factor, units);
    }

    public static void glPopMatrix(FkGL gl) {
        gl.popMatrix();
    }

    public static void glPushMatrix(FkGL gl) {
        gl.pushMatrix();
    }

    public static void glRotatef(FkGL gl, float angle, float x, float y, float z) {
        gl.rotate(angle, x, y, z);
    }

    public static void glSampleCoverage(FkGL gl, float value, boolean invert) {
    }

    public static void glScalef(FkGL gl, float x, float y, float z) {
        gl.scale(x, y, z);
    }

    public static void glScissor(FkGL gl, int x, int y, int width, int height) {
        gl.scissor(x, y, width, height);
    }

    public static void glShadeModel(FkGL gl, int mode) {
        gl.shadeModel(mode);
    }

    public static void glStencilFunc(FkGL gl, int func, int ref, int mask) {
    }

    public static void glStencilMask(FkGL gl, int mask) {
    }

    public static void glStencilOp(FkGL gl, int fail, int zfail, int zpass) {
    }

    public static void glTexCoordPointer(FkGL gl, int size, int type, int stride, DirectBuffer pointer) {
        pointer(gl.texArray, size, type, stride, pointer);
    }

    public static void glTexCoordPointer(FkGL gl, int size, int type, int stride, int pointer) {
        vbo(gl, gl.texArray, size, type, stride, pointer);
    }

    public static void glTexEnvf(FkGL gl, int target, int pname, float param) {
        gl.texEnv(pname, new float[] { param });
    }

    public static void glTexEnvfv(FkGL gl, int target, int pname, float[] params) {
        gl.texEnv(pname, params);
    }

    public static void glTexImage2D(FkGL gl, int target, int level, int internalformat, int width, int height, int border, int format, int type, DirectBuffer pixels) {
        texImage(gl, level, width, height, format, type, pixels);
    }

    public static void glTexParameterf(FkGL gl, int target, int pname, float param) {
        gl.texParameter(pname, (int) param);
    }

    public static void glTexSubImage2D(FkGL gl, int target, int level, int xoffset, int yoffset, int width, int height, int format, int type, DirectBuffer pixels) {
        texSubImage(gl, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    public static void glTranslatef(FkGL gl, float x, float y, float z) {
        gl.translate(x, y, z);
    }

    public static void glVertexPointer(FkGL gl, int size, int type, int stride, DirectBuffer pointer) {
        pointer(gl.vertexArray, size, type, stride, pointer);
    }

    public static void glVertexPointer(FkGL gl, int size, int type, int stride, int pointer) {
        vbo(gl, gl.vertexArray, size, type, stride, pointer);
    }

    public static void glViewport(FkGL gl, int x, int y, int width, int height) {
        gl.viewport(x, y, width, height);
    }

    public static void glCompressedTexImage2D(FkGL gl, int target, int level, int internalformat, int width, int height, int border, ByteBuffer data) {
    }

    public static void glCompressedTexSubImage2D(FkGL gl, int target, int level, int xoffset, int yoffset, int width, int height, int format, ByteBuffer data) {
    }

    public static void glDeleteTextures(FkGL gl, int[] textures) {
        gl.deleteTextures(textures, 0, textures.length);
    }

    public static void glDrawElements(FkGL gl, int mode, int count, DirectBuffer indices) {
        drawElements(gl, mode, count, indices instanceof ByteBuffer ? 0x1401 : 0x1403, indices, 0);
    }

    public static void glGenTextures(FkGL gl, int[] textures) {
        gl.genTextures(textures, 0, textures.length);
    }

    public static String glGetString(FkGL gl, int name) {
        return getString(name);
    }

    public static void glBindBuffer(FkGL gl, int target, int buffer) {
        bindBuffer(gl, target, buffer);
    }

    public static void glBufferData(FkGL gl, int target, DirectBuffer data, int usage) {
        bufferData(gl, target, data);
    }

    public static void glBufferSubData(FkGL gl, int target, int offset, DirectBuffer data) {
        bufferSubData(gl, target, offset, data);
    }

    public static void glClipPlanef(FkGL gl, int plane, float[] equation) {
    }

    public static void glColor4ub(FkGL gl, short red, short green, short blue, short alpha) {
        gl.color((red & 255) / 255f, (green & 255) / 255f, (blue & 255) / 255f, (alpha & 255) / 255f);
    }

    public static void glCurrentPaletteMatrixOES(FkGL gl, int matrixpaletteindex) {
    }

    public static void glDeleteBuffers(FkGL gl, int[] buffers) {
        deleteBuffers(gl, buffers);
    }

    public static void glDrawTexfOES(FkGL gl, float x, float y, float z, float width, float height) {
    }

    public static void glDrawTexiOES(FkGL gl, int x, int y, int z, int width, int height) {
    }

    public static void glDrawTexsOES(FkGL gl, short x, short y, short z, short width, short height) {
    }

    public static void glGenBuffers(FkGL gl, int[] buffers) {
        genBuffers(gl, buffers);
    }

    public static void glGetBooleanv(FkGL gl, int pname, boolean[] params) {
        if (params != null && params.length > 0) { params[0] = gl.isEnabled(pname); }
    }

    public static void glGetBufferParameteriv(FkGL gl, int target, int pname, int[] params) {
    }

    public static void glGetClipPlanef(FkGL gl, int pname, float[] eqn) {
    }

    public static void glGetFloatv(FkGL gl, int pname, float[] params) {
        getFloatv(gl, pname, params);
    }

    public static void glGetLightfv(FkGL gl, int light, int pname, float[] params) {
    }

    public static void glGetMaterialfv(FkGL gl, int face, int pname, float[] params) {
    }

    public static void glGetTexEnvfv(FkGL gl, int env, int pname, float[] params) {
    }

    public static void glGetTexEnviv(FkGL gl, int env, int pname, int[] params) {
    }

    public static void glGetTexParameterfv(FkGL gl, int target, int pname, float[] params) {
    }

    public static void glGetTexParameteriv(FkGL gl, int target, int pname, int[] params) {
    }

    public static boolean glIsBuffer(FkGL gl, int buffer) {
        return isBuffer(gl, buffer);
    }

    public static boolean glIsEnabled(FkGL gl, int cap) {
        return gl.isEnabled(cap);
    }

    public static boolean glIsTexture(FkGL gl, int texture) {
        return gl.isTexture(texture);
    }

    public static void glLoadPaletteFromModelViewMatrixOES(FkGL gl) {
    }

    public static void glMatrixIndexPointerOES(FkGL gl, int size, int type, int stride, DirectBuffer pointer) {
    }

    public static void glMatrixIndexPointerOES(FkGL gl, int size, int type, int stride, int pointer) {
    }

    public static void glPointParameterf(FkGL gl, int pname, float param) {
    }

    public static void glPointParameterfv(FkGL gl, int pname, float[] params) {
    }

    public static void glPointSizePointerOES(FkGL gl, int type, int stride, DirectBuffer pointer) {
    }

    public static void glPointSizePointerOES(FkGL gl, int type, int stride, int pointer) {
    }

    public static void glTexEnvi(FkGL gl, int target, int pname, int param) {
        gl.texEnv(pname, new float[] { param });
    }

    public static void glTexEnviv(FkGL gl, int target, int pname, int[] params) {
        gl.texEnv(pname, toFloat(params));
    }

    public static void glTexParameterfv(FkGL gl, int target, int pname, float[] params) {
        gl.texParameter(pname, (int) params[0]);
    }

    public static void glTexParameteri(FkGL gl, int target, int pname, int param) {
        gl.texParameter(pname, param);
    }

    public static void glTexParameteriv(FkGL gl, int target, int pname, int[] params) {
        gl.texParameter(pname, params[0]);
    }

    public static void glWeightPointerOES(FkGL gl, int size, int type, int stride, DirectBuffer pointer) {
    }

    public static void glWeightPointerOES(FkGL gl, int size, int type, int stride, int pointer) {
    }

    /* ------------------------------------------------------------------ */

    private static void pointer(Attrib a, int size, int type, int stride, DirectBuffer p) {
        if (p == null) {
            a.set(size, type, stride, null, 0);
            return;
        }
        a.set(size, type, stride, DirectBufferFactory.array(p), DirectBufferFactory.getSegmentOffset(p));
    }

    /** pointer into the bound array buffer: a byte offset */
    private static void vbo(FkGL gl, Attrib a, int size, int type, int stride, int offset) {
        DirectBuffer b = gl.buffer(gl.arrayBuffer);
        if (b == null) {
            gl.setError(0x0502);
            return;
        }
        Object arr = DirectBufferFactory.array(b);
        int elem = arr instanceof byte[] ? 1 : arr instanceof short[] ? 2 : 4;
        a.set(size, type, stride, arr, offset / elem);
    }

    private static int[] idx = new int[1024];

    private static void drawElements(FkGL gl, int mode, int count, int type, DirectBuffer indices, int byteOff) {
        if (indices == null || count <= 0) {
            return;
        }
        Object arr = DirectBufferFactory.array(indices);
        int off = DirectBufferFactory.getSegmentOffset(indices);
        if (idx.length < count) {
            idx = new int[count * 2];
        }
        if (arr instanceof short[]) {
            short[] s = (short[]) arr;
            int o = off + byteOff / 2;
            if (o + count > s.length) {
                gl.setError(0x0501);
                return;
            }
            for (int i = 0; i < count; i++) {
                idx[i] = s[o + i] & 0xffff;
            }
        } else if (arr instanceof byte[]) {
            byte[] b = (byte[]) arr;
            int o = off + byteOff;
            if (type == 0x1403) {
                if (o + count * 2 > b.length) {
                    gl.setError(0x0501);
                    return;
                }
                for (int i = 0; i < count; i++) {
                    idx[i] = (b[o + i * 2] & 255) | ((b[o + i * 2 + 1] & 255) << 8);
                }
            } else {
                if (o + count > b.length) {
                    gl.setError(0x0501);
                    return;
                }
                for (int i = 0; i < count; i++) {
                    idx[i] = b[o + i] & 255;
                }
            }
        } else if (arr instanceof int[]) {
            int[] s = (int[]) arr;
            int o = off + byteOff / 4;
            if (o + count > s.length) {
                gl.setError(0x0501);
                return;
            }
            System.arraycopy(s, o, idx, 0, count);
        } else {
            return;
        }
        gl.draw(mode, idx, 0, 0, count);
    }

    private static void drawElementsVbo(FkGL gl, int mode, int count, int type, int offset) {
        DirectBuffer b = gl.buffer(gl.elementBuffer);
        if (b == null) {
            gl.setError(0x0502);
            return;
        }
        drawElements(gl, mode, count, type, b, offset);
    }

    private static float[] toFloat(int[] v) {
        float[] f = new float[v.length];
        for (int i = 0; i < v.length; i++) {
            f[i] = v[i];
        }
        return f;
    }

    private static void getIntegerv(FkGL gl, int pname, int[] p) {
        if (p == null || p.length == 0) {
            return;
        }
        switch (pname) {
        case 0x0BA2: {                    /* VIEWPORT */
            int[] v = gl.viewportArray();
            for (int i = 0; i < 4 && i < p.length; i++) {
                p[i] = v[i];
            }
            break;
        }
        case 0x0D33:                      /* MAX_TEXTURE_SIZE */
            p[0] = 256;
            break;
        case 0x0D3A:                      /* MAX_VIEWPORT_DIMS */
            p[0] = gl.width();
            if (p.length > 1) {
                p[1] = gl.height();
            }
            break;
        case 0x0D31:                      /* MAX_LIGHTS */
            p[0] = 8;
            break;
        case 0x0D36:                      /* MAX_MODELVIEW_STACK_DEPTH */
            p[0] = 32;
            break;
        case 0x0D38:                      /* MAX_PROJECTION_STACK_DEPTH */
        case 0x0D39:                      /* MAX_TEXTURE_STACK_DEPTH */
            p[0] = 8;
            break;
        case 0x84E2:                      /* MAX_TEXTURE_UNITS */
            p[0] = 1;
            break;
        case 0x0D55:                      /* ALPHA_BITS */
        case 0x0D57:                      /* STENCIL_BITS */
            p[0] = 0;
            break;
        case 0x0D52:                      /* RED_BITS */
        case 0x0D54:                      /* BLUE_BITS */
            p[0] = 5;
            break;
        case 0x0D53:                      /* GREEN_BITS */
            p[0] = 6;
            break;
        case 0x0D56:                      /* DEPTH_BITS */
            p[0] = 16;
            break;
        default:
            p[0] = 0;
            break;
        }
    }

    private static void getFloatv(FkGL gl, int pname, float[] p) {
        if (p == null) {
            return;
        }
        float[] m = null;
        if (pname == 0x0BA6) {            /* MODELVIEW_MATRIX */
            m = gl.modelview();
        } else if (pname == 0x0BA7) {     /* PROJECTION_MATRIX */
            m = gl.projection();
        }
        if (m != null) {
            System.arraycopy(m, 0, p, 0, java.lang.Math.min(16, p.length));
            return;
        }
        if (pname == 0x0B00 && p.length >= 4) {   /* CURRENT_COLOR */
            System.arraycopy(gl.currentColor(), 0, p, 0, 4);
            return;
        }
        int[] i = new int[p.length];
        getIntegerv(gl, pname, i);
        for (int k = 0; k < p.length; k++) {
            p[k] = i[k];
        }
    }

    private static String getString(int name) {
        switch (name) {
        case 0x1F00: return "FunKey";
        case 0x1F01: return "M3G software";
        case 0x1F02: return "OpenGL ES-CM 1.1";
        case 0x1F03: return "";
        default: return null;
        }
    }

    /** pixels to RGBA bytes */
    private static byte[] rgba(int w, int h, int format, int type, DirectBuffer pixels) {
        byte[] out = new byte[w * h * 4];
        if (pixels == null) {
            return out;
        }
        Object arr = DirectBufferFactory.array(pixels);
        int off = DirectBufferFactory.getSegmentOffset(pixels);
        int n = w * h;
        if (type == 0x1401 && arr instanceof byte[]) {      /* UNSIGNED_BYTE */
            byte[] b = (byte[]) arr;
            int comps = format == 0x1908 ? 4 : format == 0x1907 ? 3 : format == 0x190A ? 2 : 1;
            for (int i = 0; i < n; i++) {
                int p = off + i * comps;
                if (p + comps > b.length) {
                    break;
                }
                int r;
                int g;
                int bl;
                int a = 255;
                switch (format) {
                case 0x1908: r = b[p]; g = b[p + 1]; bl = b[p + 2]; a = b[p + 3] & 255; break;
                case 0x1907: r = b[p]; g = b[p + 1]; bl = b[p + 2]; break;
                case 0x190A: r = g = bl = b[p]; a = b[p + 1] & 255; break;
                case 0x1906: r = g = bl = (byte) 255; a = b[p] & 255; break;
                default: r = g = bl = b[p]; break;
                }
                out[i * 4] = (byte) r;
                out[i * 4 + 1] = (byte) g;
                out[i * 4 + 2] = (byte) bl;
                out[i * 4 + 3] = (byte) a;
            }
        } else {
            /* packed 16-bit pixels, in a short buffer or as byte pairs */
            for (int i = 0; i < n; i++) {
                int v;
                if (arr instanceof short[]) {
                    short[] s = (short[]) arr;
                    if (off + i >= s.length) {
                        break;
                    }
                    v = s[off + i] & 0xffff;
                } else if (arr instanceof byte[]) {
                    byte[] b = (byte[]) arr;
                    int p = off + i * 2;
                    if (p + 1 >= b.length) {
                        break;
                    }
                    v = (b[p] & 255) | ((b[p + 1] & 255) << 8);
                } else {
                    break;
                }
                int r;
                int g;
                int bl;
                int a;
                if (type == 0x8363) {                       /* 5_6_5 */
                    r = (v >> 11) << 3; g = ((v >> 5) & 63) << 2; bl = (v & 31) << 3; a = 255;
                } else if (type == 0x8033) {                /* 4_4_4_4 */
                    r = (v >> 12) * 17; g = ((v >> 8) & 15) * 17; bl = ((v >> 4) & 15) * 17; a = (v & 15) * 17;
                } else {                                    /* 5_5_5_1 */
                    r = (v >> 11) << 3; g = ((v >> 6) & 31) << 3; bl = ((v >> 1) & 31) << 3; a = (v & 1) * 255;
                }
                out[i * 4] = (byte) r;
                out[i * 4 + 1] = (byte) g;
                out[i * 4 + 2] = (byte) bl;
                out[i * 4 + 3] = (byte) a;
            }
        }
        return out;
    }

    private static boolean hasAlpha(int format, int type) {
        return format == 0x1908 || format == 0x1906 || format == 0x190A
                || type == 0x8033 || type == 0x8034;
    }

    private static void texImage(FkGL gl, int level, int w, int h, int format, int type, DirectBuffer pixels) {
        gl.texImage(level, w, h, rgba(w, h, format, type, pixels), hasAlpha(format, type));
    }

    private static void texSubImage(FkGL gl, int level, int x, int y, int w, int h, int format, int type,
            DirectBuffer pixels) {
        gl.texSubImage(level, x, y, w, h, rgba(w, h, format, type, pixels));
    }

    private static void genBuffers(FkGL gl, int[] ids) {
        for (int i = 0; ids != null && i < ids.length; i++) {
            ids[i] = gl.newBufferName();
        }
    }

    private static void deleteBuffers(FkGL gl, int[] ids) {
        for (int i = 0; ids != null && i < ids.length; i++) {
            gl.buffers.remove(new Integer(ids[i]));
        }
    }

    private static boolean isBuffer(FkGL gl, int id) {
        return gl.buffers.containsKey(new Integer(id));
    }

    private static void bindBuffer(FkGL gl, int target, int id) {
        if (target == 0x8892) {           /* ARRAY_BUFFER */
            gl.arrayBuffer = id;
        } else if (target == 0x8893) {    /* ELEMENT_ARRAY_BUFFER */
            gl.elementBuffer = id;
        }
    }

    private static void bufferData(FkGL gl, int target, DirectBuffer data) {
        int id = target == 0x8892 ? gl.arrayBuffer : gl.elementBuffer;
        if (id == 0 || data == null) {
            gl.setError(0x0502);
            return;
        }
        gl.buffers.put(new Integer(id), copyBuffer(data));
    }

    private static void bufferSubData(FkGL gl, int target, int offset, DirectBuffer data) {
        int id = target == 0x8892 ? gl.arrayBuffer : gl.elementBuffer;
        DirectBuffer dst = gl.buffer(id);
        if (dst == null || data == null) {
            gl.setError(0x0502);
            return;
        }
        Object a = DirectBufferFactory.array(dst);
        Object b = DirectBufferFactory.array(data);
        int boff = DirectBufferFactory.getSegmentOffset(data);
        int blen = DirectBufferFactory.getSegmentLength(data);
        int elem = a instanceof byte[] ? 1 : a instanceof short[] ? 2 : 4;
        try {
            System.arraycopy(b, boff, a, offset / elem, blen);
        } catch (RuntimeException e) {
            gl.setError(0x0501);
        }
    }

    private static DirectBuffer copyBuffer(DirectBuffer d) {
        DirectBufferFactory f = DirectBufferFactory.getFactory();
        Object a = DirectBufferFactory.array(d);
        int off = DirectBufferFactory.getSegmentOffset(d);
        int len = DirectBufferFactory.getSegmentLength(d);
        if (a instanceof float[]) {
            float[] c = new float[len];
            System.arraycopy(a, off, c, 0, len);
            return f.allocateFloatBuffer(c);
        }
        if (a instanceof short[]) {
            short[] c = new short[len];
            System.arraycopy(a, off, c, 0, len);
            return f.allocateShortBuffer(c);
        }
        if (a instanceof int[]) {
            int[] c = new int[len];
            System.arraycopy(a, off, c, 0, len);
            return f.allocateIntBuffer(c);
        }
        byte[] c = new byte[len];
        System.arraycopy(a, off, c, 0, len);
        return f.allocateByteBuffer(c);
    }
}
