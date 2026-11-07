package com.mascotcapsule.micro3d.v3;

import javax.microedition.lcdui.Graphics;

import com.funkey.mc3.Prim;
import com.funkey.mc3.Renderer;
import com.funkey.mc3.TexData;

/**
 * MascotCapsule v3 rendering into a bound lcdui Graphics, through the
 * native M3G rasterizer. The 3D screen is the Graphics clip rectangle.
 *
 * renderFigure / renderPrimitives / drawCommandList draw at once but share
 * one depth buffer until flush(), which gives the handset's result of
 * collecting them and drawing at flush (2D drawn in between ends up under
 * them here). drawFigure draws with a depth buffer of its own.
 */
public class Graphics3D {
    public static final int COMMAND_LIST_VERSION_1_0 = 0xfe000001;
    public static final int COMMAND_END = 0x80000000;
    public static final int COMMAND_NOP = 0x81000000;
    public static final int COMMAND_FLUSH = 0x82000000;
    public static final int COMMAND_ATTRIBUTE = 0x83000000;
    public static final int COMMAND_CLIP = 0x84000000;
    public static final int COMMAND_CENTER = 0x85000000;
    public static final int COMMAND_TEXTURE_INDEX = 0x86000000;
    public static final int COMMAND_AFFINE_INDEX = 0x87000000;
    public static final int COMMAND_PARALLEL_SCALE = 0x90000000;
    public static final int COMMAND_PARALLEL_SIZE = 0x91000000;
    public static final int COMMAND_PERSPECTIVE_FOV = 0x92000000;
    public static final int COMMAND_PERSPECTIVE_WH = 0x93000000;
    public static final int COMMAND_AMBIENT_LIGHT = 0xa0000000;
    public static final int COMMAND_DIRECTION_LIGHT = 0xa1000000;
    public static final int COMMAND_THRESHOLD = 0xaf000000;

    public static final int ENV_ATTR_LIGHTING = 1;
    public static final int ENV_ATTR_SPHERE_MAP = 2;
    public static final int ENV_ATTR_TOON_SHADING = 4;
    public static final int ENV_ATTR_SEMI_TRANSPARENT = 8;

    public static final int PATTR_LIGHTING = 0x01;
    public static final int PATTR_SPHERE_MAP = 0x02;
    public static final int PATTR_COLORKEY = 0x10;
    public static final int PATTR_BLEND_NORMAL = 0x00;
    public static final int PATTR_BLEND_HALF = 0x20;
    public static final int PATTR_BLEND_ADD = 0x40;
    public static final int PATTR_BLEND_SUB = 0x60;

    public static final int PDATA_NORMAL_NONE = 0x0000;
    public static final int PDATA_NORMAL_PER_FACE = 0x0200;
    public static final int PDATA_NORMAL_PER_VERTEX = 0x0300;
    public static final int PDATA_COLOR_NONE = 0x0000;
    public static final int PDATA_COLOR_PER_COMMAND = 0x0400;
    public static final int PDATA_COLOR_PER_FACE = 0x0800;
    public static final int PDATA_TEXURE_COORD_NONE = 0x0000;
    public static final int PDATA_POINT_SPRITE_PARAMS_PER_CMD = 0x1000;
    public static final int PDATA_POINT_SPRITE_PARAMS_PER_FACE = 0x2000;
    public static final int PDATA_POINT_SPRITE_PARAMS_PER_VERTEX = 0x3000;
    public static final int PDATA_TEXURE_COORD = 0x3000;

    public static final int PRIMITVE_POINTS = 0x01000000;
    public static final int PRIMITVE_LINES = 0x02000000;
    public static final int PRIMITVE_TRIANGLES = 0x03000000;
    public static final int PRIMITVE_QUADS = 0x04000000;
    public static final int PRIMITVE_POINT_SPRITES = 0x05000000;

    public static final int POINT_SPRITE_LOCAL_SIZE = 0;
    public static final int POINT_SPRITE_PIXEL_SIZE = 1;
    public static final int POINT_SPRITE_PERSPECTIVE = 0;
    public static final int POINT_SPRITE_NO_PERS = 2;

    private final Renderer r = new Renderer();
    private Graphics bound;
    private boolean disposed;

    public Graphics3D() {
    }

    public final synchronized void bind(Graphics graphics) {
        if (graphics == null) {
            throw new NullPointerException();
        }
        if (bound != null) {
            throw new IllegalStateException("already bound");
        }
        bound = graphics;
        r.setGraphics(graphics);
    }

    public final synchronized void release(Graphics graphics) {
        if (graphics == null) {
            throw new NullPointerException();
        }
        if (graphics != bound) {
            throw new IllegalArgumentException("not the bound Graphics");
        }
        r.flush();
        r.setGraphics(null);
        bound = null;
    }

    private void checkBound() {
        if (bound == null) {
            throw new IllegalStateException("not bound");
        }
    }

    /** layout, effect and draw position into the renderer */
    private void setup(int x, int y, FigureLayout layout, Effect3D effect) {
        if (layout == null || effect == null) {
            throw new NullPointerException();
        }
        AffineTrans t = layout.getAffineTrans();
        if (t == null) {
            r.setViewTrans(4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0);
        } else {
            r.setViewTrans(t.m00, t.m01, t.m02, t.m03, t.m10, t.m11, t.m12, t.m13,
                    t.m20, t.m21, t.m22, t.m23);
        }
        switch (layout.mode) {
        case FigureLayout.PARALLEL_SIZE:
            r.setScreenView(layout.parallelW, layout.parallelH);
            break;
        case FigureLayout.PERSPECTIVE_FOV:
            r.setPerspective(layout.near, layout.far, layout.angle);
            break;
        case FigureLayout.PERSPECTIVE_WH:
            r.setPerspective(layout.near, layout.far, layout.perspW, layout.perspH);
            break;
        default:
            r.setScreenScale(layout.scaleX, layout.scaleY);
            break;
        }
        r.setScreenCenter(layout.centerX, layout.centerY);
        r.setCenterOffset(x, y);

        Light l = effect.light;
        r.enableLight(l != null);
        if (l != null) {
            Vector3D d = l.getParallelLightDirection();
            r.setAmbientLight(l.getAmbientIntensity());
            r.setDirectionLight(d.x, d.y, d.z, l.getParallelLightIntensity());
        }
        r.enableToonShader(effect.shading == Effect3D.TOON_SHADING);
        r.setToonParam(effect.toonThreshold, effect.toonHigh, effect.toonLow);
        r.enableSemiTransparent(effect.transparency);
        TexData sphere = effect.sphere == null ? null : effect.sphere.data;
        r.enableSphereMap(sphere != null);
        if (sphere != null) {
            r.setSphereTexture(sphere);
        }
    }

    public final void renderFigure(Figure figure, int x, int y, FigureLayout layout, Effect3D effect) {
        if (figure == null) {
            throw new NullPointerException();
        }
        checkBound();
        if (disposed || figure.pose == null) {
            return;
        }
        setup(x, y, layout, effect);
        r.drawFigure(figure.pose);
    }

    public final void drawFigure(Figure figure, int x, int y, FigureLayout layout, Effect3D effect) {
        if (figure == null) {
            throw new NullPointerException();
        }
        checkBound();
        if (disposed || figure.pose == null) {
            return;
        }
        setup(x, y, layout, effect);
        r.renderFigure(figure.pose);
    }

    public final void renderPrimitives(Texture texture, int x, int y, FigureLayout layout,
            Effect3D effect, int command, int numPrimitives, int[] vertexCoords, int[] normals,
            int[] textureCoords, int[] colors) {
        if (layout == null || effect == null || vertexCoords == null) {
            throw new NullPointerException();
        }
        if (command < 0 || numPrimitives <= 0 || numPrimitives >= 256) {
            throw new IllegalArgumentException();
        }
        checkBound();
        if (disposed) {
            return;
        }
        int type = command >>> 24;
        int param = command & 0x3f00;
        Prim p = new Prim(type, param, numPrimitives, vertexCoords, normals, textureCoords, colors);
        setup(x, y, layout, effect);
        r.setPrimitiveTextureArray(texture == null ? null : texture.data);
        r.renderPrimitives(p, 0, numPrimitives, command & 0x73, true);
    }

    public final void drawCommandList(Texture[] textures, int x, int y, FigureLayout layout,
            Effect3D effect, int[] commandList) {
        if (textures == null || layout == null || effect == null || commandList == null) {
            throw new NullPointerException();
        }
        checkBound();
        if (disposed) {
            return;
        }
        setup(x, y, layout, effect);
        TexData[] d = new TexData[textures.length];
        for (int i = 0; i < d.length; i++) {
            d[i] = textures[i] == null ? null : textures[i].data;
        }
        r.setPrimitiveTextureArray(d);
        AffineTrans[] a = layout.transArray;
        if (a != null) {
            int[][] m = new int[a.length][12];
            for (int i = 0; i < a.length; i++) {
                a[i].get(m[i]);
            }
            r.setViewTransArray(m);
        } else {
            AffineTrans t = layout.getAffineTrans();
            int[][] m = new int[1][12];
            if (t != null) {
                t.get(m[0]);
            } else {
                m[0][0] = m[0][5] = m[0][10] = 4096;
            }
            r.setViewTransArray(m);
        }
        r.executeCommandList(commandList, true);
    }

    public final void drawCommandList(Texture texture, int x, int y, FigureLayout layout,
            Effect3D effect, int[] commandList) {
        drawCommandList(new Texture[] { texture }, x, y, layout, effect, commandList);
    }

    public final void flush() {
        checkBound();
        r.flush();
    }

    public final void dispose() {
        disposed = true;
    }
}
