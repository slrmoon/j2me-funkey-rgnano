package com.nttdocomo.opt.ui.j3d;

/**
 * Graphics3D command lists: a version word, then commands whose top byte
 * selects the command; render commands carry the primitive count in bits
 * 16..23, the PrimitiveArray parameter bits and the attributes, followed
 * by vertices, normals, texture coordinates / point sprites and colors.
 */
final class FkCommands {
    private FkCommands() {
    }

    private static int take(int[] c, int i, int n) {
        if (c.length - i < n) {
            throw new IllegalArgumentException("command list truncated");
        }
        return i;
    }

    private static int copy(int[] c, int i, int[] dst) {
        if (dst == null || dst.length == 0) {
            return i;
        }
        take(c, i, dst.length);
        System.arraycopy(c, i, dst, 0, dst.length);
        return i + dst.length;
    }

    static void run(FkRender r, int[] c) {
        if (c == null) {
            throw new NullPointerException("commands");
        }
        /* version: COMMAND_LIST_VERSION_1, or a plain 1 from older handsets */
        if (c.length == 0 || (c[0] != 0xfe000001 && c[0] != 1)) {
            throw new IllegalArgumentException("command list version");
        }
        int i = 1;
        while (i < c.length) {
            int cmd = c[i++];
            if (cmd == 0x80000000) {          /* END */
                return;
            }
            int inline = cmd & 0x00ffffff;
            switch (cmd & 0xff000000) {
            case 0x81000000:                  /* NOP: skips its operands */
                i = take(c, i, inline) + inline;
                break;
            case 0x82000000:                  /* FLUSH */
                r.flush();
                break;
            case 0x83000000:                  /* ATTRIBUTE */
                r.enableLight((inline & 1) != 0);
                r.enableSphereMap((inline & 2) != 0);
                r.enableToonShader((inline & 4) != 0);
                r.enableSemiTransparent((inline & 8) != 0);
                break;
            case 0x84000000: {                /* CLIP_RECT: left, top, right, bottom */
                take(c, i, 4);
                int l = c[i];
                int t = c[i + 1];
                int rt = c[i + 2];
                int b = c[i + 3];
                i += 4;
                r.setClipRect3D(l, t, rt > l ? rt - l : 0, b > t ? b - t : 0);
                break;
            }
            case 0x85000000:                  /* SCREEN_CENTER */
                take(c, i, 2);
                r.setScreenCenter(c[i], c[i + 1]);
                i += 2;
                break;
            case 0x86000000:                  /* TEXTURE */
                r.setPrimitiveTexture(inline);
                break;
            case 0x87000000:                  /* VIEW_TRANS (index into the array) */
                r.setViewTrans(inline);
                break;
            case 0x90000000:                  /* SCREEN_SCALE */
                take(c, i, 2);
                r.setScreenScale(c[i], c[i + 1]);
                i += 2;
                break;
            case 0x91000000:                  /* SCREEN_VIEW */
                take(c, i, 2);
                r.setScreenView(c[i], c[i + 1]);
                i += 2;
                break;
            case 0x92000000:                  /* PERSPECTIVE1: near, far, angle */
                take(c, i, 3);
                r.setPerspective(c[i], c[i + 1], c[i + 2]);
                i += 3;
                break;
            case 0x93000000:                  /* PERSPECTIVE2: near, far, width, height */
                take(c, i, 4);
                r.setPerspective(c[i], c[i + 1], c[i + 2], c[i + 3]);
                i += 4;
                break;
            case 0xa0000000:                  /* AMBIENT_LIGHT */
                take(c, i, 1);
                r.setAmbientLight(c[i++]);
                break;
            case 0xa1000000:                  /* DIRECTION_LIGHT: x, y, z, intensity */
                take(c, i, 4);
                r.setDirectionLight(new Vector3D(c[i], c[i + 1], c[i + 2]), c[i + 3]);
                i += 4;
                break;
            case 0xaf000000:                  /* TOON_PARAM */
                take(c, i, 3);
                r.setToonParam(c[i], c[i + 1], c[i + 2]);
                i += 3;
                break;
            case 0x01000000:
            case 0x02000000:
            case 0x03000000:
            case 0x04000000:
            case 0x05000000: {
                int type = cmd >>> 24;
                int count = (cmd >> 16) & 255;
                if (count == 0) {
                    throw new IllegalArgumentException("primitive count");
                }
                int param = cmd & 0x3f00;
                if (type == 5) {
                    param |= cmd & 3;
                }
                PrimitiveArray p = new PrimitiveArray(type, param, count);
                i = copy(c, i, p.getVertexArray());
                i = copy(c, i, p.getNormalArray());
                i = copy(c, i, p.getTextureCoordArray());
                i = copy(c, i, p.getPointSpriteArray());
                i = copy(c, i, p.getColorArray());
                r.renderPrimitives(p, 0, count, cmd & 0x73);
                break;
            }
            default:
                throw new IllegalArgumentException("command " + Integer.toHexString(cmd));
            }
        }
    }
}
