package com.nttdocomo.opt.ui.j3d;

import java.io.IOException;

/**
 * MascotCapsule v3 model (MBAC, versions 2..5): vertices and normals
 * grouped by bone, colored and textured triangles/quads, patterns.
 */
final class FkModel {
    /* material bits */
    static final int MAT_COLOR_KEY = 1;
    static final int MAT_BLEND = 6;      /* 0 normal, 2 half, 4 add, 6 sub */
    static final int MAT_DOUBLE = 16;

    int numVertices;
    /** model-space vertices x,y,z */
    int[] vertices;
    /** normals x,y,z, 4096 = 1.0; null when the file has none */
    int[] normals;
    int numPatterns = 1;
    int numTextures = 1;

    /* polygons: polyVerts has 4 indices per polygon (d = -1 for triangles) */
    int numPolys;
    int[] polyVerts;
    /** texel coordinates u,v x4 per polygon (textured ones) */
    int[] polyUV;
    /** 0xRRGGBB, -1 for textured polygons */
    int[] polyColor;
    int[] polyMaterial;
    int[] polyTexture;
    int[] polyPattern;

    /* bones: vertex count, parent (-1 = root), 3x4 matrix */
    int numBones;
    int[] boneLength;
    int[] boneParent;
    int[] boneMatrix;

    private static final int[] SIZES = { 8, 10, 13, 16 };

    static FkModel load(byte[] data) throws IOException {
        FkModel m = new FkModel();
        m.parse(new FkBits(data));
        return m;
    }

    private void parse(FkBits r) throws IOException {
        if (r.u8() != 'M' || r.u8() != 'B') {
            throw new IOException("not an MBAC model");
        }
        int version = r.u8();
        if (r.u8() != 0 || version < 2 || version > 5) {
            throw new IOException("MBAC version " + version);
        }
        int vFmt = 1;
        int nFmt = 0;
        int pFmt = 1;
        int bFmt = 1;
        if (version > 3) {
            vFmt = r.u8();
            nFmt = r.u8();
            pFmt = r.u8();
            bFmt = r.u8();
        }
        if (bFmt != 1) {
            throw new IOException("MBAC bone format " + bFmt);
        }
        numVertices = r.u16();
        int t3 = r.u16();
        int t4 = r.u16();
        numBones = r.u16();
        int c3 = 0;
        int c4 = 0;
        int numColors = 0;
        if (pFmt >= 3) {
            c3 = r.u16();
            c4 = r.u16();
            numTextures = r.u16();
            numPatterns = r.u16();
            numColors = r.u16();
        }
        if (numPatterns < 1) {
            numPatterns = 1;
        }
        /* per pattern: color tris, color quads, then per texture tris, quads */
        int[][] pat = new int[numPatterns][(numTextures + 1) * 2];
        if (version == 5) {
            for (int i = 0; i < numPatterns; i++) {
                for (int j = 0; j < (numTextures + 1) * 2; j++) {
                    pat[i][j] = r.u16();
                }
            }
        } else {
            pat[0] = new int[] { c3, c4, t3, t4 };
            numTextures = 1;
        }

        vertices = new int[numVertices * 3];
        if (vFmt == 1) {
            for (int i = 0; i < vertices.length; i++) {
                vertices[i] = r.s16();
            }
        } else if (vFmt == 2) {
            int i = 0;
            while (i < vertices.length) {
                int chunk = r.ubits(8);
                int size = SIZES[chunk >> 6];
                int count = (chunk & 63) + 1;
                for (int k = 0; k < count && i < vertices.length; k++) {
                    vertices[i++] = r.sbits(size);
                    vertices[i++] = r.sbits(size);
                    vertices[i++] = r.sbits(size);
                }
            }
        } else {
            throw new IOException("MBAC vertex format " + vFmt);
        }
        r.align();

        if (nFmt == 1) {
            normals = new int[numVertices * 3];
            for (int i = 0; i < normals.length; i++) {
                normals[i] = r.s16();
            }
            r.align();
        } else if (nFmt == 2) {
            normals = new int[numVertices * 3];
            for (int i = 0; i < numVertices; i++) {
                int x = r.ubits(7);
                int nx;
                int ny;
                int nz;
                if (x == 64) {
                    /* axis normal: +z +y +x -z -y -x */
                    int t = r.ubits(3);
                    nx = ny = nz = 0;
                    switch (t) {
                    case 0: nz = 4096; break;
                    case 1: ny = 4096; break;
                    case 2: nx = 4096; break;
                    case 3: nz = -4096; break;
                    case 4: ny = -4096; break;
                    default: nx = -4096; break;
                    }
                } else {
                    x = (x << 25) >> 25;
                    int y = r.sbits(7);
                    int neg = r.ubits(1);
                    int zz = 4096 - x * x - y * y;
                    int z = zz > 0 ? (int) java.lang.Math.sqrt(zz) : 0;
                    nx = x * 64;
                    ny = y * 64;
                    nz = (neg != 0 ? -z : z) * 64;
                }
                normals[i * 3] = nx;
                normals[i * 3 + 1] = ny;
                normals[i * 3 + 2] = nz;
            }
            r.align();
        }

        int nColor = c3 + c4;
        int nTex = t3 + t4;
        numPolys = nColor + nTex;
        polyVerts = new int[numPolys * 4];
        polyUV = new int[numPolys * 8];
        polyColor = new int[numPolys];
        polyMaterial = new int[numPolys];
        polyTexture = new int[numPolys];
        polyPattern = new int[numPolys];

        /* textured polygons first (index 0..nTex-1), colored after */
        if (nColor > 0) {
            int matBits = r.u8();
            int idxBits = r.u8();
            int colBits = r.u8();
            int colIdBits = r.u8();
            r.u8();
            int[] colors = new int[numColors];
            for (int i = 0; i < numColors; i++) {
                int cr = r.ubits(colBits);
                int cg = r.ubits(colBits);
                int cb = r.ubits(colBits);
                colors[i] = (cr << 16) | (cg << 8) | cb;
            }
            for (int i = 0; i < nColor; i++) {
                int p = nTex + i;
                int corners = i < c3 ? 3 : 4;
                polyMaterial[p] = r.ubits(matBits) << 1;
                for (int k = 0; k < 4; k++) {
                    polyVerts[p * 4 + k] = k < corners ? r.ubits(idxBits) : -1;
                }
                int cid = r.ubits(colIdBits);
                polyColor[p] = cid < numColors ? colors[cid] : 0xffffff;
                polyTexture[p] = -1;
            }
        }
        if (nTex > 0) {
            int matBits = 0;
            int idxBits = 0;
            int uvBits = 7;
            if (pFmt == 2) {
                matBits = r.u8();
                idxBits = r.u8();
            } else if (pFmt == 3) {
                matBits = r.ubits(8);
                idxBits = r.ubits(8);
                uvBits = r.ubits(8);
                r.ubits(8);
            } else if (pFmt != 1) {
                throw new IOException("MBAC polygon format " + pFmt);
            }
            for (int p = 0; p < nTex; p++) {
                int corners = p < t3 ? 3 : 4;
                if (pFmt == 1) {
                    polyMaterial[p] = r.u16();
                    for (int k = 0; k < 4; k++) {
                        polyVerts[p * 4 + k] = k < corners ? r.u16() : -1;
                    }
                    for (int k = 0; k < corners * 2; k++) {
                        polyUV[p * 8 + k] = r.u8();
                    }
                } else {
                    polyMaterial[p] = r.ubits(matBits);
                    for (int k = 0; k < 4; k++) {
                        polyVerts[p * 4 + k] = k < corners ? r.ubits(idxBits) : -1;
                    }
                    for (int k = 0; k < corners * 2; k++) {
                        polyUV[p * 8 + k] = r.ubits(uvBits);
                    }
                }
                polyColor[p] = -1;
            }
        }
        r.align();
        for (int i = 0; i < numPolys * 4; i++) {
            if (polyVerts[i] >= numVertices) {
                throw new IOException("MBAC vertex index");
            }
        }

        /* patterns and texture numbers, in file order of each group */
        int ct = nTex;             /* next color triangle */
        int cq = nTex + c3;        /* next color quad */
        int tt = 0;                /* next textured triangle */
        int tq = t3;               /* next textured quad */
        for (int i = 0; i < numPatterns; i++) {
            int mask = i == 0 ? 0 : 1 << i;
            int[] pp = pat[i];
            for (int k = 0; k < pp[0] && ct < nTex + c3; k++) {
                polyPattern[ct++] = mask;
            }
            for (int k = 0; k < pp[1] && cq < numPolys; k++) {
                polyPattern[cq++] = mask;
            }
            for (int t = 0; t < numTextures; t++) {
                for (int k = 0; k < pp[2 + t * 2] && tt < t3; k++) {
                    polyPattern[tt] = mask;
                    polyTexture[tt++] = t;
                }
                for (int k = 0; k < pp[3 + t * 2] && tq < nTex; k++) {
                    polyPattern[tq] = mask;
                    polyTexture[tq++] = t;
                }
            }
        }

        boneLength = new int[numBones];
        boneParent = new int[numBones];
        boneMatrix = new int[numBones * 12];
        int total = 0;
        for (int i = 0; i < numBones; i++) {
            boneLength[i] = r.u16();
            boneParent[i] = r.s16();
            for (int k = 0; k < 12; k++) {
                boneMatrix[i * 12 + k] = r.s16();
            }
            total += boneLength[i];
        }
        if (numBones > 0 && total != numVertices) {
            throw new IOException("MBAC bones cover " + total + " of " + numVertices + " vertices");
        }
    }
}
