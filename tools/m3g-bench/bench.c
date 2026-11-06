/* Host M3G benchmark: Nokia core + FunKey soft NGL, RGB565 320x240.
 * Scene: lit textured "cars" (sphere-ish grids), unlit textured ground,
 * vertex-coloured untextured mesh, a blended layer.  Writes the last
 * frame to out.raw and prints a checksum over all frames. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef NO_ZLIB
#include <zlib.h>
#endif
#include "m3g_core.h"
#include "ngl.h"

NGLContext ngl_sw;

M3Gsizei m3gSymbianInflateBlock(M3Gsizei srcLength, const M3Gubyte *src,
                                M3Gsizei dstLength, M3Gubyte *dst) {
#ifdef NO_ZLIB
    (void)srcLength; (void)src; (void)dstLength; (void)dst; return 0;
#else
    uLongf len = (uLongf) dstLength;
    if (uncompress(dst, &len, src, srcLength) != Z_OK) return 0;
    return (M3Gsizei) len;
#endif
}

static void *my_malloc(M3Gpointer n) { return malloc(n); }
static void my_free(void *p) { free(p); }
static void my_err(M3Genum e, M3GInterface m) { (void)m; fprintf(stderr, "M3G error %d\n", e); }

#define W 320
#define H 240
static unsigned short fb[W * H];
static int opt_lin, opt_fog;  /* 1 linear fog, 2 exponential */

#define GN 17   /* grid vertices per side */

static M3GVertexBuffer make_grid(M3GInterface m3g, int sphere, int with_colors,
                                 M3GIndexBuffer *ib_out) {
    static short pos[GN * GN * 3], nrm[GN * GN * 3], tc[GN * GN * 2];
    static unsigned char col[GN * GN * 4];
    static int idx[(GN - 1) * GN * 2];
    static M3Gsizei lens[GN - 1];
    int i, j, k = 0;
    M3GVertexArray va, na, ta, ca;
    M3GVertexBuffer vb;
    float bias[3] = {0, 0, 0};
    for (j = 0; j < GN; j++) for (i = 0; i < GN; i++, k++) {
        float u = (float) i / (GN - 1), v = (float) j / (GN - 1);
        float x, y, z;
        if (sphere) {
            float th = u * 6.2831853f, ph = v * 3.1415926f;
            x = sinf(ph) * cosf(th); y = cosf(ph); z = sinf(ph) * sinf(th);
        } else {
            x = u * 2 - 1; y = 0; z = v * 2 - 1;
        }
        pos[k*3] = (short)(x * 1000); pos[k*3+1] = (short)(y * 1000); pos[k*3+2] = (short)(z * 1000);
        nrm[k*3] = (short)(sphere ? x * 127 : 0); nrm[k*3+1] = (short)(sphere ? y * 127 : 127);
        nrm[k*3+2] = (short)(sphere ? z * 127 : 0);
        tc[k*2] = (short)(u * 255 * (sphere ? 2 : 4)); tc[k*2+1] = (short)(v * 255 * (sphere ? 1 : 4));
        col[k*4] = (unsigned char)(u * 255); col[k*4+1] = (unsigned char)(v * 255);
        col[k*4+2] = (unsigned char)(128); col[k*4+3] = 255;
    }
    k = 0;
    for (j = 0; j < GN - 1; j++) {
        for (i = 0; i < GN; i++) { idx[k++] = j * GN + i; idx[k++] = (j + 1) * GN + i; }
        lens[j] = GN * 2;
    }
    va = m3gCreateVertexArray(m3g, GN * GN, 3, M3G_SHORT);
    m3gSetVertexArrayElements(va, 0, GN * GN, GN * GN * 3, M3G_SHORT, pos);
    na = m3gCreateVertexArray(m3g, GN * GN, 3, M3G_SHORT);
    m3gSetVertexArrayElements(na, 0, GN * GN, GN * GN * 3, M3G_SHORT, nrm);
    ta = m3gCreateVertexArray(m3g, GN * GN, 2, M3G_SHORT);
    m3gSetVertexArrayElements(ta, 0, GN * GN, GN * GN * 2, M3G_SHORT, tc);
    vb = m3gCreateVertexBuffer(m3g);
    m3gSetVertexArray(vb, va, 0.001f, bias, 3);
    m3gSetNormalArray(vb, na);
    m3gSetTexCoordArray(vb, 0, ta, 1.0f / 255, bias, 2);
    if (with_colors) {
        ca = m3gCreateVertexArray(m3g, GN * GN, 4, M3G_BYTE);
        m3gSetVertexArrayElements(ca, 0, GN * GN, GN * GN * 4, M3G_BYTE, col);
        m3gSetColorArray(vb, ca);
    }
    *ib_out = m3gCreateStripBuffer(m3g, M3G_TRIANGLE_STRIPS, GN - 1, lens, M3G_INT,
                                   (GN - 1) * GN * 2, idx);
    return vb;
}

static M3GTexture make_tex(M3GInterface m3g, int seed, int alpha) {
    static unsigned char px[64 * 64 * 4];
    int i, n = alpha ? 4 : 3;
    M3GImage img;
    M3GTexture t;
    for (i = 0; i < 64 * 64; i++) {
        int x = i & 63, y = i >> 6, c = ((x >> 3) ^ (y >> 3)) & 1;
        px[i*n] = (unsigned char)(c ? 220 : (x * 4 + seed));
        px[i*n+1] = (unsigned char)(c ? 200 : y * 4);
        px[i*n+2] = (unsigned char)((x ^ y) * 4 + seed * 3);
        if (alpha) px[i*n+3] = (unsigned char)((x + y) * 2);
    }
    img = m3gCreateImage(m3g, alpha ? M3G_RGBA : M3G_RGB, 64, 64, 0);
    m3gSetImage(img, px);
    t = m3gCreateTexture(m3g, img);
    m3gSetFiltering(t, M3G_FILTER_BASE_LEVEL, opt_lin ? M3G_FILTER_LINEAR : M3G_FILTER_NEAREST);
    return t;
}

int main(int argc, char **argv) {
    int frames = argc > 1 ? atoi(argv[1]) : 20, f, i;
    M3Gparams p;
    M3GInterface m3g;
    M3GRenderContext ctx;
    M3GCamera cam;
    M3GLight amb, dir;
    M3GBackground bg;
    M3GMatrix m;
    M3GVertexBuffer vsph, vgrid, vcol;
    M3GIndexBuffer isph, igrid, icol;
    M3GAppearance acar, aground, acol, ablend;
    M3GMaterial mat;
    M3GPolygonMode pm;
    M3GCompositingMode cm;
    unsigned long sum = 0;
    FILE *fo;

    if (argc > 2) { opt_lin = strchr(argv[2], 'l') != 0; opt_fog = strchr(argv[2], 'f') ? 1 : strchr(argv[2], 'e') ? 2 : 0; }
    memset(&p, 0, sizeof(p));
    p.mallocFunc = my_malloc; p.freeFunc = my_free; p.errorFunc = my_err;
    m3g = m3gCreateInterface(&p);
    ctx = m3gCreateContext(m3g);
    cam = m3gCreateCamera(m3g);
    m3gSetPerspective(cam, 60, (float) W / H, 0.5f, 100);
    amb = m3gCreateLight(m3g); m3gSetLightMode(amb, M3G_AMBIENT); m3gSetLightColor(amb, 0x404040);
    dir = m3gCreateLight(m3g); m3gSetLightMode(dir, M3G_DIRECTIONAL); m3gSetLightColor(dir, 0xffffe0);
    bg = m3gCreateBackground(m3g); m3gSetBgColor(bg, 0xff3060a0);

    vsph = make_grid(m3g, 1, 0, &isph);
    vgrid = make_grid(m3g, 0, 0, &igrid);
    vcol = make_grid(m3g, 1, 1, &icol);

    mat = m3gCreateMaterial(m3g);
    m3gSetColor(mat, M3G_DIFFUSE_BIT, 0xffc0c0c0);
    m3gSetColor(mat, M3G_SPECULAR_BIT, 0xff808080);
    m3gSetShininess(mat, 16);
    pm = m3gCreatePolygonMode(m3g);
    m3gSetShading(pm, M3G_SHADE_SMOOTH);

    acar = m3gCreateAppearance(m3g);
    m3gSetMaterial(acar, mat);
    m3gSetPolygonMode(acar, pm);
    m3gSetTexture(acar, 0, make_tex(m3g, 10, 0));

    aground = m3gCreateAppearance(m3g);
    m3gSetTexture(aground, 0, make_tex(m3g, 40, 0));
    { M3GPolygonMode p2 = m3gCreatePolygonMode(m3g); m3gSetCulling(p2, M3G_CULL_NONE); m3gSetPolygonMode(aground, p2); }

    acol = m3gCreateAppearance(m3g);

    ablend = m3gCreateAppearance(m3g);
    cm = m3gCreateCompositingMode(m3g);
    m3gSetBlending(cm, M3G_ALPHA_BLEND);
    m3gSetCompositingMode(ablend, cm);
    { M3GTexture t = make_tex(m3g, 70, 1); m3gTextureSetBlending(t, M3G_FUNC_REPLACE); m3gSetTexture(ablend, 0, t); }

    { M3GObject keep[] = { (M3GObject)cam,(M3GObject)amb,(M3GObject)dir,(M3GObject)bg,
        (M3GObject)vsph,(M3GObject)isph,(M3GObject)vgrid,(M3GObject)igrid,(M3GObject)vcol,(M3GObject)icol,
        (M3GObject)acar,(M3GObject)aground,(M3GObject)acol,(M3GObject)ablend };
      for (i = 0; i < (int)(sizeof(keep)/sizeof(keep[0])); i++) m3gAddRef(keep[i]); }
    if (opt_fog) {
        M3GFog fg = m3gCreateFog(m3g);
        M3GAppearance all[4]; all[0] = acar; all[1] = aground; all[2] = acol; all[3] = ablend;
        m3gSetFogColor(fg, 0x8090a0);
        if (opt_fog == 1) { m3gSetFogMode(fg, M3G_LINEAR_FOG); m3gSetFogLinear(fg, 4, 30); }
        else { m3gSetFogMode(fg, M3G_EXPONENTIAL_FOG); m3gSetFogDensity(fg, 0.08f); }
        for (i = 0; i < 4; i++) m3gSetFog(all[i], fg);
    }
    for (f = 0; f < frames; f++) {
        float a = f * 3.0f;
        m3gBindMemoryTarget(ctx, fb, W, H, M3G_RGB565, W, 0);
        m3gSetViewport(ctx, 0, 0, W, H);
        m3gClear(ctx, bg);
        m3gIdentityMatrix(&m);
        m3gTranslateMatrix(&m, 0, 2.0f, 8.0f);
        m3gPostRotateMatrix(&m, -12, 1, 0, 0);
        m3gSetCamera(ctx, cam, &m);
        m3gClearLights(ctx);
        m3gAddLight(ctx, amb, 0);
        m3gIdentityMatrix(&m); m3gPostRotateMatrix(&m, -45 + a, 1, 1, 0);
        m3gAddLight(ctx, dir, &m);

        m3gIdentityMatrix(&m);
        m3gTranslateMatrix(&m, 0, -1, 0);
        m3gScaleMatrix(&m, 20, 1, 40);
        m3gRender(ctx, vgrid, igrid, aground, &m, 1.0f, -1);

        for (i = 0; i < 24; i++) {
            m3gIdentityMatrix(&m);
            m3gTranslateMatrix(&m, (i % 6 - 2.5f) * 2.2f, 0, -(i / 6) * 4.0f);
            m3gPostRotateMatrix(&m, a + i * 15, 0, 1, 0);
            m3gScaleMatrix(&m, 0.9f, 0.6f, 1.3f);
            m3gRender(ctx, vsph, isph, acar, &m, 1.0f, -1);
        }
        for (i = 0; i < 6; i++) {
            m3gIdentityMatrix(&m);
            m3gTranslateMatrix(&m, (i - 2.5f) * 2.5f, 2.5f, -6);
            m3gPostRotateMatrix(&m, a * 2 + i * 30, 1, 0, 0);
            m3gRender(ctx, vcol, icol, acol, &m, 1.0f, -1);
        }
        m3gIdentityMatrix(&m);
        m3gTranslateMatrix(&m, 0, 1, 3);
        m3gPostRotateMatrix(&m, 80, 1, 0, 0);
        m3gScaleMatrix(&m, 2, 1, 1.5f);
        m3gRender(ctx, vgrid, igrid, ablend, &m, 0.8f, -1);
        m3gReleaseTarget(ctx);
        for (i = 0; i < W * H; i++) sum = sum * 31 + fb[i];
    }
    printf("checksum %08lx\n", sum & 0xffffffffUL);
    fo = fopen("out.raw", "wb");
    if (fo) { fwrite(fb, 2, W * H, fo); fclose(fo); }
    return 0;
}
