package com.nttdocomo.ui.graphics3d;

import java.io.IOException;

/** Converts the M3G object records in a DoJa D4D scene to Star graphics objects. */
final class D4GroupLoader {
    private D4GroupLoader() {}

    static Group load(byte[] data) throws IOException {
        javax.microedition.m3g.Object3D[] roots = javax.microedition.m3g.Loader.loadD4(data);
        Group out = new Group();
        for (int i = 0; i < roots.length; i++) {
            if (roots[i] instanceof javax.microedition.m3g.Node) {
                addNode(out, (javax.microedition.m3g.Node) roots[i]);
            }
        }
        if (out.getNumElements() == 0) throw new IOException("D4D scene has no drawable root");
        return out;
    }

    private static void addNode(Group parent, javax.microedition.m3g.Node node) throws IOException {
        Group target = parent;
        javax.microedition.m3g.Transform mt = new javax.microedition.m3g.Transform();
        node.getTransform(mt);
        float[] matrix = new float[16];
        mt.get(matrix);
        if (!identity(matrix)) {
            target = new Group();
            com.nttdocomo.ui.util3d.Transform t = new com.nttdocomo.ui.util3d.Transform();
            t.set(matrix);
            target.setTransform(t);
            parent.addElement(target);
        }
        if (node instanceof javax.microedition.m3g.Mesh) {
            addMesh(target, (javax.microedition.m3g.Mesh) node);
        }
        if (node instanceof javax.microedition.m3g.Group) {
            javax.microedition.m3g.Group g = (javax.microedition.m3g.Group) node;
            for (int i = 0; i < g.getChildCount(); i++) addNode(target, g.getChild(i));
        }
    }

    private static void addMesh(Group group, javax.microedition.m3g.Mesh mesh) throws IOException {
        javax.microedition.m3g.VertexBuffer vb = mesh.getVertexBuffer();
        float[] sb = new float[4];
        javax.microedition.m3g.VertexArray positions = vb.getPositions(sb);
        if (positions == null || positions.getComponentCount() < 3) return;
        float[] tsb = new float[4];
        javax.microedition.m3g.VertexArray tex = vb.getTexCoords(0, tsb);
        for (int part = 0; part < mesh.getSubmeshCount(); part++) {
            javax.microedition.m3g.IndexBuffer ib = mesh.getIndexBuffer(part);
            int n = ib.getIndexCount();
            if (n < 3) continue;
            int[] indices = new int[n];
            ib.getIndices(indices);
            int[] strips = ib instanceof javax.microedition.m3g.TriangleStripArray
                    ? ((javax.microedition.m3g.TriangleStripArray)ib).fkGetStripLengths() : null;
            if (strips == null) strips = new int[] { n };
            int triangleCount = 0;
            for (int i = 0; i < strips.length; i++) triangleCount += Math.max(0, strips[i] - 2);
            Primitive p = new Primitive(Primitive.PRIMITIVE_TRIANGLES,
                    tex == null ? Primitive.NORMAL_NONE : Primitive.TEXTURE_COORD_PER_VERTEX,
                    triangleCount);
            javax.microedition.m3g.Appearance appearance = mesh.getAppearance(part);
            if (appearance != null && appearance.getTexture(0) != null) {
                Texture texture = texture(appearance.getTexture(0).getImage());
                if (texture != null) p.setTexture(texture);
            }
            int[] dst = p.getVertexArray();
            int[] uv = p.getTextureCoordArray();
            int base = 0, triangle = 0;
            for (int strip = 0; strip < strips.length; strip++) {
                int stripLength = strips[strip];
                for (int tri = 0; tri < stripLength - 2; tri++, triangle++) {
                    int a = indices[base + tri], b = indices[base + tri + 1], c = indices[base + tri + 2];
                    if ((tri & 1) != 0) { int swap = a; a = b; b = swap; }
                    putVertex(positions, sb, a, dst, triangle * 9);
                    putVertex(positions, sb, b, dst, triangle * 9 + 3);
                    putVertex(positions, sb, c, dst, triangle * 9 + 6);
                    if (uv != null) {
                        putTex(tex, tsb, a, uv, triangle * 6);
                        putTex(tex, tsb, b, uv, triangle * 6 + 2);
                        putTex(tex, tsb, c, uv, triangle * 6 + 4);
                    }
                }
                base += stripLength;
            }
            group.addElement(p);
        }
    }

    private static Texture texture(javax.microedition.m3g.Image2D image) {
        if (image == null || image.fkGetPixels() == null) return null;
        byte[] src = image.fkGetPixels();
        byte[] palette = image.fkGetPalette();
        int format = image.getFormat();
        int pixels = image.getWidth() * image.getHeight();
        byte[] rgba = new byte[pixels * 4];
        for (int i = 0; i < pixels; i++) {
            int index = palette == null ? i : src[i] & 255;
            byte[] from = palette == null ? src : palette;
            int components = format == javax.microedition.m3g.Image2D.RGBA ? 4
                    : format == javax.microedition.m3g.Image2D.RGB ? 3
                    : format == javax.microedition.m3g.Image2D.LUMINANCE_ALPHA ? 2 : 1;
            int at = index * components;
            if (at + components > from.length) return null;
            if (format == javax.microedition.m3g.Image2D.RGBA || format == javax.microedition.m3g.Image2D.RGB) {
                rgba[i * 4] = from[at]; rgba[i * 4 + 1] = from[at + 1]; rgba[i * 4 + 2] = from[at + 2];
                rgba[i * 4 + 3] = format == javax.microedition.m3g.Image2D.RGBA ? from[at + 3] : (byte)255;
            } else {
                rgba[i * 4] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = from[at];
                rgba[i * 4 + 3] = format == javax.microedition.m3g.Image2D.LUMINANCE_ALPHA ? from[at + 1] : (byte)255;
            }
        }
        return new Texture(image.getWidth(), image.getHeight(), rgba);
    }

    private static void putVertex(javax.microedition.m3g.VertexArray a, float[] sb,
                                  int index, int[] out, int at) throws IOException {
        int[] v = read(a, index);
        out[at] = round(v[0] * sb[0] + sb[1]);
        out[at + 1] = round(v[1] * sb[0] + sb[2]);
        out[at + 2] = round(v[2] * sb[0] + sb[3]);
    }

    private static void putTex(javax.microedition.m3g.VertexArray a, float[] sb,
                               int index, int[] out, int at) throws IOException {
        int[] v = read(a, index);
        out[at] = round(v[0] * sb[0] + sb[1]);
        out[at + 1] = round(v[1] * sb[0] + sb[2]);
    }

    private static int[] read(javax.microedition.m3g.VertexArray a, int index) throws IOException {
        int count = a.getComponentCount();
        int[] out = new int[count];
        if (a.getComponentType() == 1) {
            byte[] values = new byte[count];
            a.get(index, 1, values);
            for (int i = 0; i < count; i++) out[i] = values[i];
        } else if (a.getComponentType() == 2) {
            short[] values = new short[count];
            a.get(index, 1, values);
            for (int i = 0; i < count; i++) out[i] = values[i];
        } else throw new IOException("D4D vertex component size");
        return out;
    }

    private static boolean identity(float[] m) {
        for (int i = 0; i < 16; i++) if (m[i] != ((i % 5) == 0 ? 1f : 0f)) return false;
        return true;
    }

    private static int round(float value) {
        return (int)(value < 0 ? value - 0.5f : value + 0.5f);
    }
}
