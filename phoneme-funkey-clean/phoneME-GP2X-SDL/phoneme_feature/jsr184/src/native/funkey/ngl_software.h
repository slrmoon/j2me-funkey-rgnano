/*
 * Software fixed-function backend for m3gcore's NGL path.
 *
 * m3gcore already owns scene traversal, camera setup, appearances, texture
 * binding and draw submission. This file implements the GL ES 1.x-shaped
 * backend it calls into, writing directly to the RGB565 memory target that
 * phoneME binds for LCDUI Graphics.
 */
#ifndef FUNKEY_M3G_NGL_SOFTWARE_H
#define FUNKEY_M3G_NGL_SOFTWARE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NGL_SW_MAX_TEXTURES 256
#define NGL_SW_STACK_DEPTH 16
#define NGL_SW_TEXTURE_UNITS 2

typedef struct {
    GLint size;
    GLenum type;
    GLsizei stride;
    const GLvoid *ptr;
    GLboolean enabled;
} NGLArray;

typedef struct {
    GLuint id;
    int width;
    int height;
    unsigned int *argb;
    GLint wrap_s;
    GLint wrap_t;
    GLint min_filter;
    GLint mag_filter;
} NGLTexture;

typedef struct {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat spot_direction[3];
    GLfloat spot_exponent;
    GLfloat spot_cutoff;
    GLfloat constant_attenuation;
    GLfloat linear_attenuation;
    GLfloat quadratic_attenuation;
} NGLLight;

typedef struct {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat emission[4];
    GLfloat shininess;
} NGLMaterial;

typedef struct {
    unsigned short *pixels;
    int width;
    int height;
    int stride;
    float *depth;
    int depth_count;
    GLenum matrix_mode;
    float modelview[16];
    float projection[16];
    float texture_matrix[NGL_SW_TEXTURE_UNITS][16];
    float model_stack[NGL_SW_STACK_DEPTH][16];
    float proj_stack[NGL_SW_STACK_DEPTH][16];
    float tex_stack[NGL_SW_TEXTURE_UNITS][NGL_SW_STACK_DEPTH][16];
    int model_top;
    int proj_top;
    int tex_top[NGL_SW_TEXTURE_UNITS];
    GLint viewport[4];
    GLint scissor[4];
    GLboolean scissor_test;
    GLboolean depth_test;
    GLboolean depth_mask;
    GLboolean texture_2d[NGL_SW_TEXTURE_UNITS];
    GLboolean blend;
    GLboolean alpha_test;
    GLboolean fog;
    GLboolean lighting;
    GLboolean light_enabled[8];
    GLboolean color_material;
    GLboolean polygon_offset_fill;
    GLboolean color_mask[4];
    GLboolean cull_face;
    GLenum cull_mode;
    GLenum front_face;
    GLenum depth_func;
    GLenum alpha_func;
    GLenum blend_src;
    GLenum blend_dst;
    GLenum fog_mode;
    GLenum shade_model;
    float alpha_ref;
    float fog_density;
    float fog_start;
    float fog_end;
    unsigned int fog_color;
    unsigned int clear_color;
    float clear_depth;
    float depth_near;
    float depth_far;
    float polygon_offset_factor;
    float polygon_offset_units;
    NGLLight lights[8];
    NGLMaterial material;
    GLfloat light_model_ambient[4];
    GLboolean light_model_two_side;
    GLenum color_material_face;
    GLenum color_material_mode;
    unsigned int current_color;
    GLenum active_texture;
    GLenum client_texture;
    NGLArray vertex;
    NGLArray color;
    NGLArray texcoord[NGL_SW_TEXTURE_UNITS];
    NGLArray normal;
    NGLTexture textures[NGL_SW_MAX_TEXTURES];
    GLuint bound_texture[NGL_SW_TEXTURE_UNITS];
    GLenum tex_env_mode[NGL_SW_TEXTURE_UNITS];
    unsigned int tex_env_color[NGL_SW_TEXTURE_UNITS];
    int trace_draw_budget;
    int trace_upload_budget;
    int trace_current_draw;
    int trace_triangles;
    int trace_culled;
    int trace_pixels;
    GLenum error;
} NGLContext;

extern NGLContext ngl_sw;

static int ngl_sw_texture_unit(GLenum texture);

static void ngl_sw_identity(float *m) {
    int i;
    for (i = 0; i < 16; ++i) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void ngl_sw_mul(float *out, const float *a, const float *b) {
    int r;
    int c;
    int k;
    float tmp[16];
    for (c = 0; c < 4; ++c) {
        for (r = 0; r < 4; ++r) {
            float v = 0.0f;
            for (k = 0; k < 4; ++k) {
                v += a[k * 4 + r] * b[c * 4 + k];
            }
            tmp[c * 4 + r] = v;
        }
    }
    memcpy(out, tmp, sizeof(tmp));
}

static float *ngl_sw_matrix(void) {
    int unit;
    if (ngl_sw.matrix_mode == GL_PROJECTION) {
        return ngl_sw.projection;
    }
    if (ngl_sw.matrix_mode == GL_TEXTURE) {
        unit = ngl_sw_texture_unit(ngl_sw.active_texture);
        if (unit < 0) unit = 0;
        return ngl_sw.texture_matrix[unit];
    }
    return ngl_sw.modelview;
}

static unsigned short ngl_sw_rgb565(unsigned int argb) {
    unsigned int r = (argb >> 16) & 0xffU;
    unsigned int g = (argb >> 8) & 0xffU;
    unsigned int b = argb & 0xffU;
    return (unsigned short)(((r & 0xf8U) << 8) |
                            ((g & 0xfcU) << 3) |
                            (b >> 3));
}

static unsigned int ngl_sw_pack(unsigned int r, unsigned int g,
                                unsigned int b, unsigned int a) {
    if (r > 255U) r = 255U;
    if (g > 255U) g = 255U;
    if (b > 255U) b = 255U;
    if (a > 255U) a = 255U;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static float ngl_sw_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void ngl_sw_color_to_float(unsigned int argb, float *out) {
    out[0] = (float)((argb >> 16) & 0xffU) / 255.0f;
    out[1] = (float)((argb >> 8) & 0xffU) / 255.0f;
    out[2] = (float)(argb & 0xffU) / 255.0f;
    out[3] = (float)((argb >> 24) & 0xffU) / 255.0f;
}

static unsigned int ngl_sw_float_to_color(const float *c) {
    return ngl_sw_pack((unsigned int)(ngl_sw_clampf(c[0], 0.0f, 1.0f) * 255.0f + 0.5f),
                       (unsigned int)(ngl_sw_clampf(c[1], 0.0f, 1.0f) * 255.0f + 0.5f),
                       (unsigned int)(ngl_sw_clampf(c[2], 0.0f, 1.0f) * 255.0f + 0.5f),
                       (unsigned int)(ngl_sw_clampf(c[3], 0.0f, 1.0f) * 255.0f + 0.5f));
}

static unsigned int ngl_sw_rgb565_to_argb(unsigned short p) {
    unsigned int r = (p >> 11) & 0x1fU;
    unsigned int g = (p >> 5) & 0x3fU;
    unsigned int b = p & 0x1fU;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return 0xff000000U | (r << 16) | (g << 8) | b;
}

static unsigned int ngl_sw_apply_color_mask(unsigned int src, unsigned int dst) {
    unsigned int out = dst;
    if (ngl_sw.color_mask[0]) out = (out & ~0x00ff0000U) | (src & 0x00ff0000U);
    if (ngl_sw.color_mask[1]) out = (out & ~0x0000ff00U) | (src & 0x0000ff00U);
    if (ngl_sw.color_mask[2]) out = (out & ~0x000000ffU) | (src & 0x000000ffU);
    if (ngl_sw.color_mask[3]) out = (out & ~0xff000000U) | (src & 0xff000000U);
    return out;
}

static float ngl_sw_component(const void *base, GLenum type) {
    if (type == GL_BYTE) return (float)(*((const signed char *)base));
    if (type == GL_UNSIGNED_BYTE) return (float)(*((const unsigned char *)base));
    if (type == GL_SHORT) return (float)(*((const short *)base));
    if (type == GL_UNSIGNED_SHORT) return (float)(*((const unsigned short *)base));
    if (type == GL_FIXED) return (float)(*((const int *)base)) / 65536.0f;
    return *((const float *)base);
}

static int ngl_sw_type_size(GLenum type) {
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE) return 1;
    if (type == GL_SHORT || type == GL_UNSIGNED_SHORT) return 2;
    return 4;
}

static const unsigned char *ngl_sw_array_ptr(const NGLArray *a, int index) {
    int stride;
    if (a == NULL || a->ptr == NULL) {
        return NULL;
    }
    stride = a->stride != 0 ? a->stride : a->size * ngl_sw_type_size(a->type);
    return ((const unsigned char *)a->ptr) + index * stride;
}

static void ngl_sw_vec(const NGLArray *a, int index, float *v, int want) {
    const unsigned char *p;
    int i;
    int step;
    p = ngl_sw_array_ptr(a, index);
    for (i = 0; i < want; ++i) {
        v[i] = (i == 3) ? 1.0f : 0.0f;
    }
    if (p == NULL) {
        return;
    }
    step = ngl_sw_type_size(a->type);
    for (i = 0; i < a->size && i < want; ++i) {
        v[i] = ngl_sw_component(p + i * step, a->type);
    }
}

static unsigned int ngl_sw_color(int index) {
    const unsigned char *p;
    int step;
    float c[4];
    if (!ngl_sw.color.enabled || ngl_sw.color.ptr == NULL) {
        return ngl_sw.current_color;
    }
    p = ngl_sw_array_ptr(&ngl_sw.color, index);
    if (p == NULL) {
        return ngl_sw.current_color;
    }
    step = ngl_sw_type_size(ngl_sw.color.type);
    c[0] = c[1] = c[2] = 255.0f;
    c[3] = 255.0f;
    if (ngl_sw.color.type == GL_UNSIGNED_BYTE || ngl_sw.color.type == GL_BYTE) {
        int i;
        for (i = 0; i < ngl_sw.color.size && i < 4; ++i) {
            c[i] = ngl_sw_component(p + i * step, ngl_sw.color.type);
        }
    }
    return ngl_sw_pack((unsigned int)c[0], (unsigned int)c[1],
                       (unsigned int)c[2], (unsigned int)c[3]);
}

static void ngl_sw_transform_vec4(float *out, const float *m, const float *v) {
    int r;
    for (r = 0; r < 4; ++r) {
        out[r] = m[0 * 4 + r] * v[0] +
                 m[1 * 4 + r] * v[1] +
                 m[2 * 4 + r] * v[2] +
                 m[3 * 4 + r] * v[3];
    }
}

static void ngl_sw_transform_vec3(float *out, const float *m, const float *v) {
    int r;
    for (r = 0; r < 3; ++r) {
        out[r] = m[0 * 4 + r] * v[0] +
                 m[1 * 4 + r] * v[1] +
                 m[2 * 4 + r] * v[2];
    }
}

static void ngl_sw_normalize3(float *v) {
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 0.000001f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

static float ngl_sw_dot3(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static int ngl_sw_light_index(GLenum light) {
    int index = (int)light - (int)GL_LIGHT0;
    return index >= 0 && index < 8 ? index : -1;
}

static unsigned int ngl_sw_lit_color(int index, const float *eye,
                                     unsigned int base_color) {
    float normal[4];
    float n[3];
    float material_ambient[4];
    float material_diffuse[4];
    float result[4];
    int i;
    if (!ngl_sw.lighting || !ngl_sw.normal.enabled || ngl_sw.normal.ptr == NULL) {
        return base_color;
    }
    ngl_sw_vec(&ngl_sw.normal, index, normal, 3);
    ngl_sw_transform_vec3(n, ngl_sw.modelview, normal);
    ngl_sw_normalize3(n);
    if (ngl_sw.color_material) {
        ngl_sw_color_to_float(base_color, material_ambient);
        ngl_sw_color_to_float(base_color, material_diffuse);
    } else {
        memcpy(material_ambient, ngl_sw.material.ambient, sizeof(material_ambient));
        memcpy(material_diffuse, ngl_sw.material.diffuse, sizeof(material_diffuse));
    }
    for (i = 0; i < 3; ++i) {
        result[i] = ngl_sw.material.emission[i] +
                    material_ambient[i] * ngl_sw.light_model_ambient[i];
    }
    result[3] = material_diffuse[3];
    for (i = 0; i < 8; ++i) {
        NGLLight *light = &ngl_sw.lights[i];
        float l[3];
        float attenuation = 1.0f;
        float ndotl;
        int c;
        if (!ngl_sw.light_enabled[i]) {
            continue;
        }
        for (c = 0; c < 3; ++c) {
            result[c] += material_ambient[c] * light->ambient[c];
        }
        if (light->position[3] == 0.0f) {
            l[0] = light->position[0];
            l[1] = light->position[1];
            l[2] = light->position[2];
            ngl_sw_normalize3(l);
        } else {
            float lx = light->position[0] / light->position[3] - eye[0];
            float ly = light->position[1] / light->position[3] - eye[1];
            float lz = light->position[2] / light->position[3] - eye[2];
            float dist = sqrtf(lx * lx + ly * ly + lz * lz);
            l[0] = lx; l[1] = ly; l[2] = lz;
            if (dist > 0.000001f) {
                float denom = light->constant_attenuation +
                              light->linear_attenuation * dist +
                              light->quadratic_attenuation * dist * dist;
                if (denom > 0.000001f) attenuation = 1.0f / denom;
            }
            ngl_sw_normalize3(l);
            if (light->spot_cutoff < 180.0f) {
                float to_vertex[3];
                float cutoff = cosf(light->spot_cutoff * 3.14159265358979323846f / 180.0f);
                float spot;
                to_vertex[0] = -l[0];
                to_vertex[1] = -l[1];
                to_vertex[2] = -l[2];
                ngl_sw_normalize3(to_vertex);
                spot = ngl_sw_dot3(to_vertex, light->spot_direction);
                if (spot < cutoff) {
                    attenuation = 0.0f;
                } else if (light->spot_exponent > 0.0f) {
                    attenuation *= powf(spot, light->spot_exponent);
                }
            }
        }
        ndotl = ngl_sw_dot3(n, l);
        if (ndotl < 0.0f && ngl_sw.light_model_two_side) {
            ndotl = -ndotl;
        }
        if (ndotl > 0.0f && attenuation > 0.0f) {
            float v[3];
            float h[3];
            float spec = 0.0f;
            v[0] = -eye[0];
            v[1] = -eye[1];
            v[2] = -eye[2];
            ngl_sw_normalize3(v);
            h[0] = l[0] + v[0];
            h[1] = l[1] + v[1];
            h[2] = l[2] + v[2];
            ngl_sw_normalize3(h);
            spec = ngl_sw_dot3(n, h);
            if (spec < 0.0f) spec = 0.0f;
            if (ngl_sw.material.shininess > 0.0f) {
                spec = powf(spec, ngl_sw.material.shininess);
            }
            for (c = 0; c < 3; ++c) {
                result[c] += attenuation *
                             (material_diffuse[c] * light->diffuse[c] * ndotl +
                              ngl_sw.material.specular[c] * light->specular[c] * spec);
            }
        }
    }
    return ngl_sw_float_to_color(result);
}

static NGLTexture *ngl_sw_texture(GLuint id) {
    int i;
    if (id == 0) {
        return &ngl_sw.textures[0];
    }
    for (i = 1; i < NGL_SW_MAX_TEXTURES; ++i) {
        if (ngl_sw.textures[i].id == id) {
            return &ngl_sw.textures[i];
        }
    }
    for (i = 1; i < NGL_SW_MAX_TEXTURES; ++i) {
        if (ngl_sw.textures[i].id == 0) {
            ngl_sw.textures[i].id = id;
            ngl_sw.textures[i].wrap_s = GL_CLAMP_TO_EDGE;
            ngl_sw.textures[i].wrap_t = GL_CLAMP_TO_EDGE;
            ngl_sw.textures[i].min_filter = GL_NEAREST;
            ngl_sw.textures[i].mag_filter = GL_NEAREST;
            return &ngl_sw.textures[i];
        }
    }
    ngl_sw.error = GL_OUT_OF_MEMORY;
    return &ngl_sw.textures[0];
}

static int ngl_sw_texture_unit(GLenum texture) {
    int unit = (int)texture - (int)GL_TEXTURE0;
    return unit >= 0 && unit < NGL_SW_TEXTURE_UNITS ? unit : -1;
}

static int ngl_sw_pixel_bytes(GLenum format) {
    switch (format) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_M3G_LUMINANCE_ALPHA4:
        return 1;
    case GL_LUMINANCE_ALPHA:
    case GL_M3G_RGB565:
        return 2;
    case GL_RGB:
        return 3;
    case GL_RGBA:
    case GL_M3G_RGB8_32:
    case GL_M3G_BGR8_32:
    case GL_M3G_BGRA8:
    case GL_M3G_ARGB8:
        return 4;
    default:
        return 0;
    }
}

static unsigned int ngl_sw_unpack_texel(const unsigned char *p,
                                        GLenum format) {
    unsigned int l;
    unsigned int a;
    unsigned int v;
    switch (format) {
    case GL_ALPHA:
        return ngl_sw_pack(255U, 255U, 255U, p[0]);
    case GL_LUMINANCE:
        return ngl_sw_pack(p[0], p[0], p[0], 255U);
    case GL_M3G_LUMINANCE_ALPHA4:
        l = (unsigned int)(p[0] & 0xf0U) | ((unsigned int)p[0] >> 4);
        a = (unsigned int)(p[0] & 0x0fU) * 17U;
        return ngl_sw_pack(l, l, l, a);
    case GL_LUMINANCE_ALPHA:
        return ngl_sw_pack(p[0], p[0], p[0], p[1]);
    case GL_RGB:
        return ngl_sw_pack(p[0], p[1], p[2], 255U);
    case GL_M3G_RGB565:
        v = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        return ngl_sw_pack(((v >> 11) & 0x1fU) * 255U / 31U,
                           ((v >> 5) & 0x3fU) * 255U / 63U,
                           (v & 0x1fU) * 255U / 31U, 255U);
    case GL_M3G_RGB8_32:
        return ngl_sw_pack(p[0], p[1], p[2], 255U);
    case GL_M3G_BGR8_32:
        return ngl_sw_pack(p[2], p[1], p[0], 255U);
    case GL_RGBA:
        return ngl_sw_pack(p[0], p[1], p[2], p[3]);
    case GL_M3G_BGRA8:
        return ngl_sw_pack(p[2], p[1], p[0], p[3]);
    case GL_M3G_ARGB8:
        return ngl_sw_pack(p[1], p[2], p[3], p[0]);
    default:
        return 0xffffffffU;
    }
}

static void ngl_sw_upload_pixels(NGLTexture *t, int xoffset, int yoffset,
                                 int width, int height, GLenum format,
                                 const unsigned char *src) {
    int bpp = ngl_sw_pixel_bytes(format);
    int x;
    int y;
    if (bpp == 0 || src == NULL || t == NULL || t->argb == NULL) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            const unsigned char *p = src + (y * width + x) * bpp;
            t->argb[(y + yoffset) * t->width + x + xoffset] =
                ngl_sw_unpack_texel(p, format);
        }
    }
}

static unsigned int ngl_sw_texel(NGLTexture *t, float s, float tt) {
    int x;
    int y;
    if (t == NULL || t->argb == NULL || t->width <= 0 || t->height <= 0) {
        return 0xffffffffU;
    }
    if (t->wrap_s == GL_REPEAT) {
        s = s - floorf(s);
    } else {
        if (s < 0.0f) s = 0.0f;
        if (s > 1.0f) s = 1.0f;
    }
    if (t->wrap_t == GL_REPEAT) {
        tt = tt - floorf(tt);
    } else {
        if (tt < 0.0f) tt = 0.0f;
        if (tt > 1.0f) tt = 1.0f;
    }
    x = (int)(s * (float)(t->width - 1) + 0.5f);
    y = (int)(tt * (float)(t->height - 1) + 0.5f);
    if (x < 0) x = 0;
    if (x >= t->width) x = t->width - 1;
    if (y < 0) y = 0;
    if (y >= t->height) y = t->height - 1;
    return t->argb[y * t->width + x];
}

static unsigned int ngl_sw_modulate(unsigned int a, unsigned int b) {
    unsigned int ar = (a >> 16) & 0xffU;
    unsigned int ag = (a >> 8) & 0xffU;
    unsigned int ab = a & 0xffU;
    unsigned int aa = (a >> 24) & 0xffU;
    unsigned int br = (b >> 16) & 0xffU;
    unsigned int bg = (b >> 8) & 0xffU;
    unsigned int bb = b & 0xffU;
    unsigned int ba = (b >> 24) & 0xffU;
    return ngl_sw_pack(ar * br / 255U, ag * bg / 255U,
                       ab * bb / 255U, aa * ba / 255U);
}

static unsigned int ngl_sw_apply_tex_env(int unit, unsigned int color,
                                         unsigned int texel) {
    unsigned int cr = (color >> 16) & 0xffU;
    unsigned int cg = (color >> 8) & 0xffU;
    unsigned int cb = color & 0xffU;
    unsigned int ca = (color >> 24) & 0xffU;
    unsigned int tr = (texel >> 16) & 0xffU;
    unsigned int tg = (texel >> 8) & 0xffU;
    unsigned int tb = texel & 0xffU;
    unsigned int ta = (texel >> 24) & 0xffU;
    unsigned int er;
    unsigned int eg;
    unsigned int eb;
    if (ngl_sw.tex_env_mode[unit] == GL_REPLACE) {
        return texel;
    }
    if (ngl_sw.tex_env_mode[unit] == GL_ADD) {
        return ngl_sw_pack(cr + tr, cg + tg, cb + tb, ca * ta / 255U);
    }
    if (ngl_sw.tex_env_mode[unit] == GL_BLEND) {
        unsigned int env = ngl_sw.tex_env_color[unit];
        er = (env >> 16) & 0xffU;
        eg = (env >> 8) & 0xffU;
        eb = env & 0xffU;
        return ngl_sw_pack(cr * (255U - tr) / 255U + er * tr / 255U,
                           cg * (255U - tg) / 255U + eg * tg / 255U,
                           cb * (255U - tb) / 255U + eb * tb / 255U,
                           ca * ta / 255U);
    }
    if (ngl_sw.tex_env_mode[unit] == GL_DECAL) {
        return ngl_sw_pack(cr * (255U - ta) / 255U + tr * ta / 255U,
                           cg * (255U - ta) / 255U + tg * ta / 255U,
                           cb * (255U - ta) / 255U + tb * ta / 255U,
                           ca);
    }
    return ngl_sw_modulate(color, texel);
}

static int ngl_sw_compare(GLenum func, float lhs, float rhs) {
    switch (func) {
    case GL_NEVER: return 0;
    case GL_LESS: return lhs < rhs;
    case GL_EQUAL: return fabsf(lhs - rhs) <= 1.0f / 255.0f;
    case GL_LEQUAL: return lhs <= rhs || fabsf(lhs - rhs) <= 1.0f / 255.0f;
    case GL_GREATER: return lhs > rhs;
    case GL_NOTEQUAL: return fabsf(lhs - rhs) > 1.0f / 255.0f;
    case GL_GEQUAL: return lhs >= rhs || fabsf(lhs - rhs) <= 1.0f / 255.0f;
    case GL_ALWAYS:
    default:
        return 1;
    }
}

static unsigned int ngl_sw_channel_factor(GLenum factor, unsigned int src,
                                          unsigned int dst, int channel) {
    unsigned int sa = (src >> 24) & 0xffU;
    unsigned int da = (dst >> 24) & 0xffU;
    unsigned int sr = (src >> 16) & 0xffU;
    unsigned int sg = (src >> 8) & 0xffU;
    unsigned int sb = src & 0xffU;
    unsigned int dr = (dst >> 16) & 0xffU;
    unsigned int dg = (dst >> 8) & 0xffU;
    unsigned int db = dst & 0xffU;
    unsigned int sc = channel == 0 ? sr : (channel == 1 ? sg : sb);
    unsigned int dc = channel == 0 ? dr : (channel == 1 ? dg : db);
    switch (factor) {
    case GL_ZERO: return 0;
    case GL_ONE: return 255;
    case GL_SRC_ALPHA: return sa;
    case GL_ONE_MINUS_SRC_ALPHA: return 255U - sa;
    case GL_DST_ALPHA: return da;
    case GL_ONE_MINUS_DST_ALPHA: return 255U - da;
    case GL_SRC_COLOR: return sc;
    case GL_ONE_MINUS_SRC_COLOR: return 255U - sc;
    case GL_DST_COLOR: return dc;
    case GL_ONE_MINUS_DST_COLOR: return 255U - dc;
    case GL_SRC_ALPHA_SATURATE:
        return sa < 255U - da ? sa : 255U - da;
    default:
        return 255;
    }
}

static unsigned int ngl_sw_blend(unsigned int src, unsigned int dst) {
    unsigned int sr;
    unsigned int sg;
    unsigned int sb;
    unsigned int dr;
    unsigned int dg;
    unsigned int db;
    unsigned int sf;
    unsigned int df;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    if (!ngl_sw.blend) {
        return src;
    }
    sr = (src >> 16) & 0xffU;
    sg = (src >> 8) & 0xffU;
    sb = src & 0xffU;
    dr = (dst >> 16) & 0xffU;
    dg = (dst >> 8) & 0xffU;
    db = dst & 0xffU;
    sf = ngl_sw_channel_factor(ngl_sw.blend_src, src, dst, 0);
    df = ngl_sw_channel_factor(ngl_sw.blend_dst, src, dst, 0);
    r = sr * sf / 255U + dr * df / 255U;
    sf = ngl_sw_channel_factor(ngl_sw.blend_src, src, dst, 1);
    df = ngl_sw_channel_factor(ngl_sw.blend_dst, src, dst, 1);
    g = sg * sf / 255U + dg * df / 255U;
    sf = ngl_sw_channel_factor(ngl_sw.blend_src, src, dst, 2);
    df = ngl_sw_channel_factor(ngl_sw.blend_dst, src, dst, 2);
    b = sb * sf / 255U + db * df / 255U;
    return ngl_sw_pack(r, g, b, 255U);
}

static unsigned int ngl_sw_apply_fog(unsigned int color, float z) {
    float factor;
    unsigned int cr;
    unsigned int cg;
    unsigned int cb;
    unsigned int ca;
    unsigned int fr;
    unsigned int fg;
    unsigned int fb;
    if (!ngl_sw.fog) {
        return color;
    }
    if (ngl_sw.fog_mode == GL_EXP) {
        factor = expf(-ngl_sw.fog_density * z);
    } else if (ngl_sw.fog_mode == GL_EXP2) {
        float d = ngl_sw.fog_density * z;
        factor = expf(-(d * d));
    } else {
        float span = ngl_sw.fog_end - ngl_sw.fog_start;
        factor = span != 0.0f ? (ngl_sw.fog_end - z) / span : 1.0f;
    }
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    cr = (color >> 16) & 0xffU;
    cg = (color >> 8) & 0xffU;
    cb = color & 0xffU;
    ca = (color >> 24) & 0xffU;
    fr = (ngl_sw.fog_color >> 16) & 0xffU;
    fg = (ngl_sw.fog_color >> 8) & 0xffU;
    fb = ngl_sw.fog_color & 0xffU;
    return ngl_sw_pack((unsigned int)(fr + ((float)cr - (float)fr) * factor),
                       (unsigned int)(fg + ((float)cg - (float)fg) * factor),
                       (unsigned int)(fb + ((float)cb - (float)fb) * factor),
                       ca);
}

typedef struct {
    float x, y, z, w;
    float fog;
    float u[NGL_SW_TEXTURE_UNITS], v[NGL_SW_TEXTURE_UNITS];
    unsigned int color;
} NGLVertex;

static void ngl_sw_transform_vertex(int index, NGLVertex *out) {
    float p[4];
    float mv[4];
    float clip[4];
    float tc[4];
    int unit;
    ngl_sw_vec(&ngl_sw.vertex, index, p, 4);
    ngl_sw_transform_vec4(mv, ngl_sw.modelview, p);
    ngl_sw_transform_vec4(clip, ngl_sw.projection, mv);
    out->x = clip[0];
    out->y = clip[1];
    out->z = clip[2];
    out->w = clip[3];
    out->fog = fabsf(mv[2]);
    out->color = ngl_sw_lit_color(index, mv, ngl_sw_color(index));
    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        out->u[unit] = 0.0f;
        out->v[unit] = 0.0f;
        if (ngl_sw.texcoord[unit].enabled && ngl_sw.texcoord[unit].ptr != NULL) {
            ngl_sw_vec(&ngl_sw.texcoord[unit], index, tc, 4);
            out->u[unit] = ngl_sw.texture_matrix[unit][0] * tc[0] +
                           ngl_sw.texture_matrix[unit][4] * tc[1] +
                           ngl_sw.texture_matrix[unit][12];
            out->v[unit] = ngl_sw.texture_matrix[unit][1] * tc[0] +
                           ngl_sw.texture_matrix[unit][5] * tc[1] +
                           ngl_sw.texture_matrix[unit][13];
        }
    }
}

static float ngl_sw_edge(float ax, float ay, float bx, float by,
                         float cx, float cy) {
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

static unsigned int ngl_sw_lerp_color(unsigned int a, unsigned int b,
                                      float t) {
    unsigned int ar = (a >> 16) & 0xffU;
    unsigned int ag = (a >> 8) & 0xffU;
    unsigned int ab = a & 0xffU;
    unsigned int aa = (a >> 24) & 0xffU;
    unsigned int br = (b >> 16) & 0xffU;
    unsigned int bg = (b >> 8) & 0xffU;
    unsigned int bb = b & 0xffU;
    unsigned int ba = (b >> 24) & 0xffU;
    return ngl_sw_pack((unsigned int)((float)ar + ((float)br - (float)ar) * t),
                       (unsigned int)((float)ag + ((float)bg - (float)ag) * t),
                       (unsigned int)((float)ab + ((float)bb - (float)ab) * t),
                       (unsigned int)((float)aa + ((float)ba - (float)aa) * t));
}

static unsigned int ngl_sw_bary_color(unsigned int a, unsigned int b,
                                      unsigned int c, float wa,
                                      float wb, float wc) {
    unsigned int ar = (a >> 16) & 0xffU;
    unsigned int ag = (a >> 8) & 0xffU;
    unsigned int ab = a & 0xffU;
    unsigned int aa = (a >> 24) & 0xffU;
    unsigned int br = (b >> 16) & 0xffU;
    unsigned int bg = (b >> 8) & 0xffU;
    unsigned int bb = b & 0xffU;
    unsigned int ba = (b >> 24) & 0xffU;
    unsigned int cr = (c >> 16) & 0xffU;
    unsigned int cg = (c >> 8) & 0xffU;
    unsigned int cb = c & 0xffU;
    unsigned int ca = (c >> 24) & 0xffU;
    return ngl_sw_pack((unsigned int)(wa * ar + wb * br + wc * cr + 0.5f),
                       (unsigned int)(wa * ag + wb * bg + wc * cg + 0.5f),
                       (unsigned int)(wa * ab + wb * bb + wc * cb + 0.5f),
                       (unsigned int)(wa * aa + wb * ba + wc * ca + 0.5f));
}

static NGLVertex ngl_sw_lerp_vertex(NGLVertex a, NGLVertex b, float t) {
    NGLVertex out;
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = a.z + (b.z - a.z) * t;
    out.w = a.w + (b.w - a.w) * t;
    out.fog = a.fog + (b.fog - a.fog) * t;
    {
        int unit;
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            out.u[unit] = a.u[unit] + (b.u[unit] - a.u[unit]) * t;
            out.v[unit] = a.v[unit] + (b.v[unit] - a.v[unit]) * t;
        }
    }
    out.color = ngl_sw_lerp_color(a.color, b.color, t);
    return out;
}

static float ngl_sw_clip_distance(const NGLVertex *v, int plane) {
    switch (plane) {
    case 0: return v->w - 0.0001f;
    case 1: return v->x + v->w;
    case 2: return v->w - v->x;
    case 3: return v->y + v->w;
    case 4: return v->w - v->y;
    case 5: return v->z + v->w;
    default: return v->w - v->z;
    }
}

static int ngl_sw_clip_plane(const NGLVertex *input, int count,
                             NGLVertex *output, int plane) {
    int i;
    int out_count = 0;
    NGLVertex prev = input[count - 1];
    float prev_d = ngl_sw_clip_distance(&prev, plane);
    int prev_in = prev_d >= 0.0f;
    for (i = 0; i < count; ++i) {
        NGLVertex curr = input[i];
        float curr_d = ngl_sw_clip_distance(&curr, plane);
        int curr_in = curr_d >= 0.0f;
        if (curr_in != prev_in && out_count < 12) {
            float denom = prev_d - curr_d;
            float t = denom != 0.0f ? prev_d / denom : 0.0f;
            output[out_count++] = ngl_sw_lerp_vertex(prev, curr, t);
        }
        if (curr_in && out_count < 12) {
            output[out_count++] = curr;
        }
        prev = curr;
        prev_d = curr_d;
        prev_in = curr_in;
    }
    return out_count;
}

static void ngl_sw_raster_tri(NGLVertex a, NGLVertex b, NGLVertex c) {
    float ax, ay, bx, by, cx, cy;
    float aw, bw, cw;
    float az, bz, cz;
    float area;
    int minx, maxx, miny, maxy;
    int x, y;
    int scx0, scy0, scx1, scy1;
    float depth_offset = 0.0f;
    if (ngl_sw.pixels == NULL || ngl_sw.width <= 0 || ngl_sw.height <= 0) {
        return;
    }
    if (a.w == 0.0f || b.w == 0.0f || c.w == 0.0f) {
        return;
    }
    if (a.w < 0.001f || b.w < 0.001f || c.w < 0.001f) {
        return;
    }
    aw = 1.0f / a.w;
    bw = 1.0f / b.w;
    cw = 1.0f / c.w;
    ax = (a.x * aw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    ay = (a.y * aw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];
    bx = (b.x * bw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    by = (b.y * bw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];
    cx = (c.x * cw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    cy = (c.y * cw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];
    az = a.z * aw * 0.5f + 0.5f;
    bz = b.z * bw * 0.5f + 0.5f;
    cz = c.z * cw * 0.5f + 0.5f;
    area = ngl_sw_edge(ax, ay, bx, by, cx, cy);
    if (area == 0.0f) return;
    if (ngl_sw.trace_current_draw) {
        ++ngl_sw.trace_triangles;
    }
    if (ngl_sw.cull_face) {
        int front = (area < 0.0f) == (ngl_sw.front_face == GL_CCW);
        if ((front && ngl_sw.cull_mode == GL_FRONT) ||
            (!front && ngl_sw.cull_mode == GL_BACK)) {
            if (ngl_sw.trace_current_draw) {
                ++ngl_sw.trace_culled;
            }
            return;
        }
    }
    if (area < 0.0f) {
        area = -area;
        { NGLVertex tv = b; b = c; c = tv; }
        { float tf = bx; bx = cx; cx = tf; }
        { float tf = by; by = cy; cy = tf; }
        { float tf = bz; bz = cz; cz = tf; }
        { float tf = bw; bw = cw; cw = tf; }
    }
    if (ngl_sw.polygon_offset_fill) {
        float span_x = fmaxf(ax, fmaxf(bx, cx)) - fminf(ax, fminf(bx, cx));
        float span_y = fmaxf(ay, fmaxf(by, cy)) - fminf(ay, fminf(by, cy));
        float span = fmaxf(1.0f, fmaxf(span_x, span_y));
        float slope = fmaxf(fabsf(bz - az), fabsf(cz - az)) / span;
        depth_offset = ngl_sw.polygon_offset_factor * slope +
                       ngl_sw.polygon_offset_units / 65535.0f;
    }
    minx = (int)floorf(fminf(ax, fminf(bx, cx)));
    maxx = (int)ceilf(fmaxf(ax, fmaxf(bx, cx)));
    miny = (int)floorf(fminf(ay, fminf(by, cy)));
    maxy = (int)ceilf(fmaxf(ay, fmaxf(by, cy)));
    scx0 = ngl_sw.scissor_test ? ngl_sw.scissor[0] : 0;
    scy0 = ngl_sw.scissor_test ? ngl_sw.scissor[1] : 0;
    scx1 = scx0 + (ngl_sw.scissor_test ? ngl_sw.scissor[2] : ngl_sw.width);
    scy1 = scy0 + (ngl_sw.scissor_test ? ngl_sw.scissor[3] : ngl_sw.height);
    if (minx < scx0) minx = scx0;
    if (maxx > scx1) maxx = scx1;
    if (miny < scy0) miny = scy0;
    if (maxy > scy1) maxy = scy1;
    for (y = miny; y < maxy; ++y) {
        for (x = minx; x < maxx; ++x) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;
            float w0 = ngl_sw_edge(bx, by, cx, cy, px, py) / area;
            float w1 = ngl_sw_edge(cx, cy, ax, ay, px, py) / area;
            float w2 = ngl_sw_edge(ax, ay, bx, by, px, py) / area;
            int fy;
            int offset;
            float z;
            float fog_coord;
            unsigned int color;
            float denom;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
            z = w0 * az + w1 * bz + w2 * cz;
            z = ngl_sw.depth_near + z * (ngl_sw.depth_far - ngl_sw.depth_near);
            z += depth_offset;
            fy = ngl_sw.height - 1 - y;
            if (fy < 0 || fy >= ngl_sw.height || x < 0 || x >= ngl_sw.width) continue;
            offset = fy * ngl_sw.stride + x;
            if (ngl_sw.depth_test && ngl_sw.depth != NULL) {
                if (!ngl_sw_compare(ngl_sw.depth_func, z, ngl_sw.depth[offset])) {
                    continue;
                }
            }
            denom = w0 * aw + w1 * bw + w2 * cw;
            fog_coord = denom != 0.0f ?
                (w0 * a.fog * aw + w1 * b.fog * bw +
                 w2 * c.fog * cw) / denom :
                w0 * a.fog + w1 * b.fog + w2 * c.fog;
            if (ngl_sw.shade_model == GL_SMOOTH && denom != 0.0f) {
                color = ngl_sw_bary_color(a.color, b.color, c.color,
                                          w0 * aw / denom,
                                          w1 * bw / denom,
                                          w2 * cw / denom);
            } else {
                color = c.color;
            }
            {
                int unit;
                for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
                    NGLTexture *tex = ngl_sw.texture_2d[unit] ?
                                      ngl_sw_texture(ngl_sw.bound_texture[unit]) :
                                      NULL;
                    if (tex != NULL && tex->argb != NULL) {
                        float u = 0.0f;
                        float v = 0.0f;
                        unsigned int texel;
                        if (denom != 0.0f) {
                            u = (w0 * a.u[unit] * aw +
                                 w1 * b.u[unit] * bw +
                                 w2 * c.u[unit] * cw) / denom;
                            v = (w0 * a.v[unit] * aw +
                                 w1 * b.v[unit] * bw +
                                 w2 * c.v[unit] * cw) / denom;
                        }
                        texel = ngl_sw_texel(tex, u, v);
                        color = ngl_sw_apply_tex_env(unit, color, texel);
                    }
                }
            }
            if (ngl_sw.alpha_test &&
                !ngl_sw_compare(ngl_sw.alpha_func,
                                (float)((color >> 24) & 0xffU) / 255.0f,
                                ngl_sw.alpha_ref)) {
                continue;
            }
            if (ngl_sw.depth_test && ngl_sw.depth != NULL && ngl_sw.depth_mask) {
                ngl_sw.depth[offset] = z;
            }
            color = ngl_sw_apply_fog(color, fog_coord);
            {
                unsigned int dst = ngl_sw_rgb565_to_argb(ngl_sw.pixels[offset]);
                color = ngl_sw_blend(color, dst);
                color = ngl_sw_apply_color_mask(color, dst);
            }
            ngl_sw.pixels[offset] = ngl_sw_rgb565(color);
            if (ngl_sw.trace_current_draw) {
                ++ngl_sw.trace_pixels;
            }
        }
    }
}

static void ngl_sw_draw_tri(NGLVertex a, NGLVertex b, NGLVertex c) {
    NGLVertex first[12];
    NGLVertex second[12];
    NGLVertex *input = first;
    NGLVertex *output = second;
    NGLVertex *swap;
    int count = 3;
    int plane;
    int i;
    first[0] = a;
    first[1] = b;
    first[2] = c;
    for (plane = 0; plane < 7 && count >= 3; ++plane) {
        count = ngl_sw_clip_plane(input, count, output, plane);
        swap = input;
        input = output;
        output = swap;
    }
    for (i = 1; i + 1 < count; ++i) {
        ngl_sw_raster_tri(input[0], input[i], input[i + 1]);
    }
}

static int ngl_sw_index(GLenum type, const GLvoid *indices, int i) {
    if (type == GL_UNSIGNED_BYTE) return ((const unsigned char *)indices)[i];
    if (type == GL_UNSIGNED_SHORT) return ((const unsigned short *)indices)[i];
    return ((const unsigned int *)indices)[i];
}

static void ngl_sw_draw_indexed(GLenum mode, GLsizei count, GLenum type,
                                const GLvoid *indices, GLint first) {
    int i;
    int trace = ngl_sw.trace_draw_budget > 0;
    NGLTexture *t0 = NULL;
    NGLTexture *t1 = NULL;
    (void)mode;
    if (!ngl_sw.vertex.enabled || ngl_sw.vertex.ptr == NULL || count < 3) return;
    if (trace) {
        ngl_sw.trace_current_draw = 1;
        ngl_sw.trace_triangles = 0;
        ngl_sw.trace_culled = 0;
        ngl_sw.trace_pixels = 0;
        if (ngl_sw.texture_2d[0]) {
            t0 = ngl_sw_texture(ngl_sw.bound_texture[0]);
        }
        if (NGL_SW_TEXTURE_UNITS > 1 && ngl_sw.texture_2d[1]) {
            t1 = ngl_sw_texture(ngl_sw.bound_texture[1]);
        }
    }
    for (i = 0; i + 2 < count; ++i) {
        int ia = indices != NULL ? ngl_sw_index(type, indices, i) : first + i;
        int ib = indices != NULL ? ngl_sw_index(type, indices, i + 1) : first + i + 1;
        int ic = indices != NULL ? ngl_sw_index(type, indices, i + 2) : first + i + 2;
        NGLVertex a, b, c;
        if ((i & 1) != 0) {
            int t = ia; ia = ib; ib = t;
        }
        ngl_sw_transform_vertex(ia, &a);
        ngl_sw_transform_vertex(ib, &b);
        ngl_sw_transform_vertex(ic, &c);
        ngl_sw_draw_tri(a, b, c);
    }
    if (trace) {
        fprintf(stderr,
                "[M3G NGL draw] idx=%d count=%d tex0=%dx%d tex1=%dx%d "
                "depth=%d/%x/%d cull=%d/%x/%x alpha=%d blend=%d "
                "offset=%d tri=%d culled=%d pixels=%d\n",
                ngl_sw.trace_draw_budget, (int)count,
                t0 != NULL ? t0->width : 0, t0 != NULL ? t0->height : 0,
                t1 != NULL ? t1->width : 0, t1 != NULL ? t1->height : 0,
                ngl_sw.depth_test, (unsigned int)ngl_sw.depth_func,
                ngl_sw.depth_mask, ngl_sw.cull_face,
                (unsigned int)ngl_sw.cull_mode,
                (unsigned int)ngl_sw.front_face,
                ngl_sw.alpha_test, ngl_sw.blend, ngl_sw.polygon_offset_fill,
                ngl_sw.trace_triangles, ngl_sw.trace_culled,
                ngl_sw.trace_pixels);
        ngl_sw.trace_current_draw = 0;
        --ngl_sw.trace_draw_budget;
    }
}

static void nglSetRenderTarget(void *pixels, int width, int height, int stride) {
    int need;
    ngl_sw.pixels = (unsigned short *)pixels;
    ngl_sw.width = width;
    ngl_sw.height = height;
    ngl_sw.stride = stride > 0 ? stride : width;
    need = ngl_sw.stride * height;
    if (need > ngl_sw.depth_count) {
        free(ngl_sw.depth);
        ngl_sw.depth = (float *)malloc((size_t)need * sizeof(float));
        ngl_sw.depth_count = ngl_sw.depth != NULL ? need : 0;
    }
}

static inline void nglTraceFrame(int draw_budget, int upload_budget) {
    ngl_sw.trace_draw_budget = draw_budget;
    ngl_sw.trace_upload_budget = upload_budget;
    ngl_sw.trace_current_draw = 0;
}

static inline void glActiveTexture(GLenum texture) {
    if (ngl_sw_texture_unit(texture) >= 0) ngl_sw.active_texture = texture;
    else ngl_sw.error = GL_INVALID_OPERATION;
}
static inline void glClientActiveTexture(GLenum texture) {
    if (ngl_sw_texture_unit(texture) >= 0) ngl_sw.client_texture = texture;
    else ngl_sw.error = GL_INVALID_OPERATION;
}
static inline void glAlphaFunc(GLenum func, GLfloat ref) {
    ngl_sw.alpha_func = func;
    ngl_sw.alpha_ref = ref;
    if (ngl_sw.alpha_ref < 0.0f) ngl_sw.alpha_ref = 0.0f;
    if (ngl_sw.alpha_ref > 1.0f) ngl_sw.alpha_ref = 1.0f;
}
static inline void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    ngl_sw.blend_src = sfactor;
    ngl_sw.blend_dst = dfactor;
}
static inline void glClearColorx(GLfixed r, GLfixed g, GLfixed b, GLfixed a) {
    ngl_sw.clear_color = ngl_sw_pack((unsigned int)(r >> 8),
                                     (unsigned int)(g >> 8),
                                     (unsigned int)(b >> 8),
                                     (unsigned int)(a >> 8));
}
static inline void glClearDepthx(GLfixed depth) { ngl_sw.clear_depth = (float)depth / 65536.0f; }
static inline void glColor4x(GLfixed r, GLfixed g, GLfixed b, GLfixed a) {
    ngl_sw.current_color = ngl_sw_pack((unsigned int)(r >> 8),
                                       (unsigned int)(g >> 8),
                                       (unsigned int)(b >> 8),
                                       (unsigned int)(a >> 8));
}
static inline void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    ngl_sw.color_mask[0] = r;
    ngl_sw.color_mask[1] = g;
    ngl_sw.color_mask[2] = b;
    ngl_sw.color_mask[3] = a;
}
static inline void glColorMaterial(GLenum face, GLenum mode) {
    ngl_sw.color_material_face = face;
    ngl_sw.color_material_mode = mode;
}
static inline void glCullFace(GLenum mode) { ngl_sw.cull_mode = mode; }
static inline void glDepthFunc(GLenum func) { ngl_sw.depth_func = func; }
static inline void glDepthMask(GLboolean flag) { ngl_sw.depth_mask = flag; }
static inline void glDepthRangef(GLfloat nearVal, GLfloat farVal) {
    ngl_sw.depth_near = nearVal;
    ngl_sw.depth_far = farVal;
}
static inline void glDisable(GLenum cap) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (cap == GL_DEPTH_TEST) ngl_sw.depth_test = GL_FALSE;
    else if (cap == GL_TEXTURE_2D && unit >= 0) ngl_sw.texture_2d[unit] = GL_FALSE;
    else if (cap == GL_BLEND) ngl_sw.blend = GL_FALSE;
    else if (cap == GL_ALPHA_TEST) ngl_sw.alpha_test = GL_FALSE;
    else if (cap == GL_FOG) ngl_sw.fog = GL_FALSE;
    else if (cap == GL_LIGHTING) ngl_sw.lighting = GL_FALSE;
    else if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) ngl_sw.light_enabled[cap - GL_LIGHT0] = GL_FALSE;
    else if (cap == GL_COLOR_MATERIAL) ngl_sw.color_material = GL_FALSE;
    else if (cap == GL_POLYGON_OFFSET_FILL) ngl_sw.polygon_offset_fill = GL_FALSE;
    else if (cap == GL_CULL_FACE) ngl_sw.cull_face = GL_FALSE;
    else if (cap == GL_SCISSOR_TEST) ngl_sw.scissor_test = GL_FALSE;
}
static inline void glEnable(GLenum cap) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (cap == GL_DEPTH_TEST) ngl_sw.depth_test = GL_TRUE;
    else if (cap == GL_TEXTURE_2D && unit >= 0) ngl_sw.texture_2d[unit] = GL_TRUE;
    else if (cap == GL_BLEND) ngl_sw.blend = GL_TRUE;
    else if (cap == GL_ALPHA_TEST) ngl_sw.alpha_test = GL_TRUE;
    else if (cap == GL_FOG) ngl_sw.fog = GL_TRUE;
    else if (cap == GL_LIGHTING) ngl_sw.lighting = GL_TRUE;
    else if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) ngl_sw.light_enabled[cap - GL_LIGHT0] = GL_TRUE;
    else if (cap == GL_COLOR_MATERIAL) ngl_sw.color_material = GL_TRUE;
    else if (cap == GL_POLYGON_OFFSET_FILL) ngl_sw.polygon_offset_fill = GL_TRUE;
    else if (cap == GL_CULL_FACE) ngl_sw.cull_face = GL_TRUE;
    else if (cap == GL_SCISSOR_TEST) ngl_sw.scissor_test = GL_TRUE;
}
static inline void glEnableClientState(GLenum array) {
    int unit = ngl_sw_texture_unit(ngl_sw.client_texture);
    if (array == GL_VERTEX_ARRAY) ngl_sw.vertex.enabled = GL_TRUE;
    else if (array == GL_COLOR_ARRAY) ngl_sw.color.enabled = GL_TRUE;
    else if (array == GL_NORMAL_ARRAY) ngl_sw.normal.enabled = GL_TRUE;
    else if (array == GL_TEXTURE_COORD_ARRAY && unit >= 0) ngl_sw.texcoord[unit].enabled = GL_TRUE;
}
static inline void glDisableClientState(GLenum array) {
    int unit = ngl_sw_texture_unit(ngl_sw.client_texture);
    if (array == GL_VERTEX_ARRAY) ngl_sw.vertex.enabled = GL_FALSE;
    else if (array == GL_COLOR_ARRAY) ngl_sw.color.enabled = GL_FALSE;
    else if (array == GL_NORMAL_ARRAY) ngl_sw.normal.enabled = GL_FALSE;
    else if (array == GL_TEXTURE_COORD_ARRAY && unit >= 0) ngl_sw.texcoord[unit].enabled = GL_FALSE;
}
static inline void glFinish(void) {}
static inline void glFogf(GLenum pname, GLfloat param) {
    if (pname == GL_FOG_DENSITY) ngl_sw.fog_density = param;
    else if (pname == GL_FOG_START) ngl_sw.fog_start = param;
    else if (pname == GL_FOG_END) ngl_sw.fog_end = param;
    else if (pname == GL_FOG_MODE) ngl_sw.fog_mode = (GLenum)param;
}
static inline void glFogxv(GLenum pname, const GLfixed *params) {
    if (params == NULL) return;
    if (pname == GL_FOG_COLOR) {
        ngl_sw.fog_color =
            ngl_sw_pack((unsigned int)(params[0] >> 8),
                        (unsigned int)(params[1] >> 8),
                        (unsigned int)(params[2] >> 8),
                        (unsigned int)(params[3] >> 8));
    } else if (pname == GL_FOG_MODE) {
        ngl_sw.fog_mode = (GLenum)params[0];
    } else if (pname == GL_FOG_DENSITY) {
        ngl_sw.fog_density = (float)params[0] / 65536.0f;
    } else if (pname == GL_FOG_START) {
        ngl_sw.fog_start = (float)params[0] / 65536.0f;
    } else if (pname == GL_FOG_END) {
        ngl_sw.fog_end = (float)params[0] / 65536.0f;
    }
}
static inline void glFrontFace(GLenum mode) { ngl_sw.front_face = mode; }
static inline void glGenTextures(GLsizei n, GLuint *textures) {
    static GLuint next = 16;
    GLsizei i;
    for (i = 0; i < n; ++i) textures[i] = next++;
}
static inline GLenum glGetError(void) { GLenum e = ngl_sw.error; ngl_sw.error = GL_NO_ERROR; return e; }
static inline void glGetIntegerv(GLenum pname, GLint *params) {
    if (params == NULL) return;
    if (pname == GL_MAX_TEXTURE_SIZE) {
        params[0] = 4096;
    } else if (pname == GL_MAX_VIEWPORT_DIMS) {
        params[0] = 4096;
        params[1] = 4096;
    } else {
        params[0] = 0;
    }
}
static inline const GLubyte *glGetString(GLenum name) { (void)name; return (const GLubyte *)"FunKey software NGL"; }
static inline void glHint(GLenum target, GLenum mode) { (void)target; (void)mode; }
static inline void glLightf(GLenum light, GLenum pname, GLfloat param) {
    int index = ngl_sw_light_index(light);
    NGLLight *l;
    if (index < 0) return;
    l = &ngl_sw.lights[index];
    if (pname == GL_SPOT_EXPONENT) l->spot_exponent = param;
    else if (pname == GL_SPOT_CUTOFF) l->spot_cutoff = param;
    else if (pname == GL_CONSTANT_ATTENUATION) l->constant_attenuation = param;
    else if (pname == GL_LINEAR_ATTENUATION) l->linear_attenuation = param;
    else if (pname == GL_QUADRATIC_ATTENUATION) l->quadratic_attenuation = param;
}
static inline void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    int index = ngl_sw_light_index(light);
    NGLLight *l;
    if (index < 0 || params == NULL) return;
    l = &ngl_sw.lights[index];
    if (pname == GL_AMBIENT) memcpy(l->ambient, params, 4 * sizeof(GLfloat));
    else if (pname == GL_DIFFUSE) memcpy(l->diffuse, params, 4 * sizeof(GLfloat));
    else if (pname == GL_SPECULAR) memcpy(l->specular, params, 4 * sizeof(GLfloat));
    else if (pname == GL_POSITION) ngl_sw_transform_vec4(l->position, ngl_sw.modelview, params);
    else if (pname == GL_SPOT_DIRECTION) {
        ngl_sw_transform_vec3(l->spot_direction, ngl_sw.modelview, params);
        ngl_sw_normalize3(l->spot_direction);
    }
}
static inline void glLightModelf(GLenum pname, GLfloat param) {
    if (pname == GL_LIGHT_MODEL_TWO_SIDE) ngl_sw.light_model_two_side = param != 0.0f;
}
static inline void glLightModelfv(GLenum pname, const GLfloat *params) {
    if (params == NULL) return;
    if (pname == GL_LIGHT_MODEL_AMBIENT) memcpy(ngl_sw.light_model_ambient, params, 4 * sizeof(GLfloat));
    else if (pname == GL_LIGHT_MODEL_TWO_SIDE) ngl_sw.light_model_two_side = params[0] != 0.0f;
}
static inline void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    (void)face;
    if (pname == GL_SHININESS) ngl_sw.material.shininess = param;
}
static inline void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    (void)face;
    if (params == NULL) return;
    if (pname == GL_AMBIENT) memcpy(ngl_sw.material.ambient, params, 4 * sizeof(GLfloat));
    else if (pname == GL_DIFFUSE) memcpy(ngl_sw.material.diffuse, params, 4 * sizeof(GLfloat));
    else if (pname == GL_SPECULAR) memcpy(ngl_sw.material.specular, params, 4 * sizeof(GLfloat));
    else if (pname == GL_EMISSION) memcpy(ngl_sw.material.emission, params, 4 * sizeof(GLfloat));
}
static inline void glMatrixMode(GLenum mode) { ngl_sw.matrix_mode = mode; }
static inline void glLoadIdentity(void) { ngl_sw_identity(ngl_sw_matrix()); }
static inline void glLoadMatrixf(const GLfloat *m) { memcpy(ngl_sw_matrix(), m, 16 * sizeof(float)); }
static inline void glMultMatrixf(const GLfloat *m) { ngl_sw_mul(ngl_sw_matrix(), ngl_sw_matrix(), m); }
static inline void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr) { ngl_sw.normal.type = type; ngl_sw.normal.stride = stride; ngl_sw.normal.ptr = ptr; ngl_sw.normal.size = 3; }
static inline void glPixelStorei(GLenum pname, GLint param) { (void)pname; (void)param; }
static inline void glPolygonOffset(GLfloat factor, GLfloat units) {
    ngl_sw.polygon_offset_factor = factor;
    ngl_sw.polygon_offset_units = units;
}
static inline void glPopMatrix(void) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (unit < 0) unit = 0;
    if (ngl_sw.matrix_mode == GL_PROJECTION && ngl_sw.proj_top > 0) memcpy(ngl_sw.projection, ngl_sw.proj_stack[--ngl_sw.proj_top], sizeof(ngl_sw.projection));
    else if (ngl_sw.matrix_mode == GL_TEXTURE && ngl_sw.tex_top[unit] > 0) memcpy(ngl_sw.texture_matrix[unit], ngl_sw.tex_stack[unit][--ngl_sw.tex_top[unit]], sizeof(ngl_sw.texture_matrix[unit]));
    else if (ngl_sw.model_top > 0) memcpy(ngl_sw.modelview, ngl_sw.model_stack[--ngl_sw.model_top], sizeof(ngl_sw.modelview));
}
static inline void glPushMatrix(void) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (unit < 0) unit = 0;
    if (ngl_sw.matrix_mode == GL_PROJECTION && ngl_sw.proj_top < NGL_SW_STACK_DEPTH) memcpy(ngl_sw.proj_stack[ngl_sw.proj_top++], ngl_sw.projection, sizeof(ngl_sw.projection));
    else if (ngl_sw.matrix_mode == GL_TEXTURE && ngl_sw.tex_top[unit] < NGL_SW_STACK_DEPTH) memcpy(ngl_sw.tex_stack[unit][ngl_sw.tex_top[unit]++], ngl_sw.texture_matrix[unit], sizeof(ngl_sw.texture_matrix[unit]));
    else if (ngl_sw.model_top < NGL_SW_STACK_DEPTH) memcpy(ngl_sw.model_stack[ngl_sw.model_top++], ngl_sw.modelview, sizeof(ngl_sw.modelview));
}
static inline void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, GLvoid *pixels) {
    unsigned char *dst = (unsigned char *)pixels;
    int px;
    int py;
    int bytes = ngl_sw_pixel_bytes(format);
    if (type != GL_UNSIGNED_BYTE || dst == NULL || bytes == 0 ||
        width < 0 || height < 0) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    for (py = 0; py < height; ++py) {
        for (px = 0; px < width; ++px) {
            unsigned int color = 0;
            int sx = x + px;
            int sy = y + py;
            if (ngl_sw.pixels != NULL && sx >= 0 && sx < ngl_sw.width &&
                sy >= 0 && sy < ngl_sw.height) {
                color = ngl_sw_rgb565_to_argb(
                    ngl_sw.pixels[(ngl_sw.height - 1 - sy) * ngl_sw.stride + sx]);
            }
            switch (format) {
            case GL_RGBA:
                dst[0] = (unsigned char)((color >> 16) & 0xffU);
                dst[1] = (unsigned char)((color >> 8) & 0xffU);
                dst[2] = (unsigned char)(color & 0xffU);
                dst[3] = (unsigned char)((color >> 24) & 0xffU);
                break;
            case GL_RGB:
                dst[0] = (unsigned char)((color >> 16) & 0xffU);
                dst[1] = (unsigned char)((color >> 8) & 0xffU);
                dst[2] = (unsigned char)(color & 0xffU);
                break;
            case GL_ALPHA:
                dst[0] = (unsigned char)((color >> 24) & 0xffU);
                break;
            case GL_LUMINANCE:
                dst[0] = (unsigned char)((((color >> 16) & 0xffU) * 77U +
                                          ((color >> 8) & 0xffU) * 150U +
                                          (color & 0xffU) * 29U) >> 8);
                break;
            default:
                ngl_sw.error = GL_INVALID_OPERATION;
                return;
            }
            dst += bytes;
        }
    }
}
static inline void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    float m[16]; ngl_sw_identity(m); m[0] = x; m[5] = y; m[10] = z; glMultMatrixf(m);
}
static inline void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    float m[16]; ngl_sw_identity(m); m[12] = x; m[13] = y; m[14] = z; glMultMatrixf(m);
}
static inline void glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal) {
    float l = (float)left / 65536.0f, r = (float)right / 65536.0f;
    float b = (float)bottom / 65536.0f, t = (float)top / 65536.0f;
    float n = (float)nearVal / 65536.0f, f = (float)farVal / 65536.0f;
    float m[16]; ngl_sw_identity(m);
    m[0] = 2.0f / (r - l); m[5] = 2.0f / (t - b); m[10] = -2.0f / (f - n);
    m[12] = -(r + l) / (r - l); m[13] = -(t + b) / (t - b); m[14] = -(f + n) / (f - n);
    glMultMatrixf(m);
}
static inline void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { ngl_sw.scissor[0] = x; ngl_sw.scissor[1] = y; ngl_sw.scissor[2] = width; ngl_sw.scissor[3] = height; }
static inline void glShadeModel(GLenum mode) { ngl_sw.shade_model = mode; }
static inline void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { ngl_sw.viewport[0] = x; ngl_sw.viewport[1] = y; ngl_sw.viewport[2] = width; ngl_sw.viewport[3] = height; }
static inline void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) { ngl_sw.vertex.size = size; ngl_sw.vertex.type = type; ngl_sw.vertex.stride = stride; ngl_sw.vertex.ptr = ptr; }
static inline void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) { ngl_sw.color.size = size; ngl_sw.color.type = type; ngl_sw.color.stride = stride; ngl_sw.color.ptr = ptr; }
static inline void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) { int unit = ngl_sw_texture_unit(ngl_sw.client_texture); if (unit < 0) return; ngl_sw.texcoord[unit].size = size; ngl_sw.texcoord[unit].type = type; ngl_sw.texcoord[unit].stride = stride; ngl_sw.texcoord[unit].ptr = ptr; }
static inline void glTexParameterx(GLenum target, GLenum pname, GLfixed param) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    NGLTexture *t;
    (void)target;
    if (unit < 0) return;
    t = ngl_sw_texture(ngl_sw.bound_texture[unit]);
    if (pname == GL_TEXTURE_WRAP_S) t->wrap_s = param;
    else if (pname == GL_TEXTURE_WRAP_T) t->wrap_t = param;
    else if (pname == GL_TEXTURE_MIN_FILTER) t->min_filter = param;
    else if (pname == GL_TEXTURE_MAG_FILTER) t->mag_filter = param;
}
static inline void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    (void)target;
    if (unit < 0 || params == NULL) return;
    if (pname == GL_TEXTURE_ENV_COLOR) {
        ngl_sw.tex_env_color[unit] =
            ngl_sw_pack((unsigned int)(ngl_sw_clampf(params[0], 0.0f, 1.0f) * 255.0f + 0.5f),
                        (unsigned int)(ngl_sw_clampf(params[1], 0.0f, 1.0f) * 255.0f + 0.5f),
                        (unsigned int)(ngl_sw_clampf(params[2], 0.0f, 1.0f) * 255.0f + 0.5f),
                        (unsigned int)(ngl_sw_clampf(params[3], 0.0f, 1.0f) * 255.0f + 0.5f));
    }
}
static inline void glTexEnvx(GLenum target, GLenum pname, GLfixed param) { int unit = ngl_sw_texture_unit(ngl_sw.active_texture); (void)target; if (unit >= 0 && pname == GL_TEXTURE_ENV_MODE) ngl_sw.tex_env_mode[unit] = (GLenum)param; }
static inline void glTexParameteri(GLenum target, GLenum pname, GLint param) { glTexParameterx(target, pname, param); }
static inline void glBindTexture(GLenum target, GLuint texture) { int unit = ngl_sw_texture_unit(ngl_sw.active_texture); (void)target; if (unit >= 0) { ngl_sw.bound_texture[unit] = texture; ngl_sw_texture(texture); } }
static inline void glCompressedTexImage2D(GLenum target, GLint level,
                                          GLenum internalformat, GLsizei width,
                                          GLsizei height, GLint border,
                                          GLsizei imageSize, const GLvoid *data) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    NGLTexture *t;
    const unsigned char *src = (const unsigned char *)data;
    int palette_bytes;
    int x;
    int y;
    (void)target;
    (void)level;
    (void)border;
    if (unit < 0 || src == NULL || width < 0 || height < 0 ||
        (internalformat != GL_PALETTE8_RGB8_OES &&
         internalformat != GL_PALETTE8_RGBA8_OES)) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    palette_bytes = internalformat == GL_PALETTE8_RGB8_OES ? 256 * 3 : 256 * 4;
    if (imageSize < palette_bytes + width * height) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    t = ngl_sw_texture(ngl_sw.bound_texture[unit]);
    free(t->argb);
    t->argb = NULL;
    t->width = width;
    t->height = height;
    if (width == 0 || height == 0) return;
    t->argb = (unsigned int *)malloc((size_t)width * (size_t)height *
                                     sizeof(unsigned int));
    if (t->argb == NULL) {
        ngl_sw.error = GL_OUT_OF_MEMORY;
        return;
    }
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            int index = src[palette_bytes + y * width + x];
            const unsigned char *p =
                src + index * (internalformat == GL_PALETTE8_RGB8_OES ? 3 : 4);
            t->argb[y * width + x] =
                ngl_sw_pack(p[0], p[1], p[2],
                            internalformat == GL_PALETTE8_RGBA8_OES ? p[3] : 255);
        }
    }
}
static inline void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                    GLint x, GLint y, GLsizei width,
                                    GLsizei height, GLint border) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    NGLTexture *t;
    int px;
    int py;
    (void)target;
    (void)level;
    (void)internalformat;
    (void)border;
    if (unit < 0 || width < 0 || height < 0) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    t = ngl_sw_texture(ngl_sw.bound_texture[unit]);
    free(t->argb);
    t->argb = NULL;
    t->width = width;
    t->height = height;
    if (width == 0 || height == 0) return;
    t->argb = (unsigned int *)malloc((size_t)width * (size_t)height *
                                     sizeof(unsigned int));
    if (t->argb == NULL) {
        ngl_sw.error = GL_OUT_OF_MEMORY;
        return;
    }
    for (py = 0; py < height; ++py) {
        for (px = 0; px < width; ++px) {
            int sx = x + px;
            int sy = y + py;
            unsigned int color = 0;
            if (ngl_sw.pixels != NULL && sx >= 0 && sx < ngl_sw.width &&
                sy >= 0 && sy < ngl_sw.height) {
                color = ngl_sw_rgb565_to_argb(
                    ngl_sw.pixels[(ngl_sw.height - 1 - sy) * ngl_sw.stride + sx]);
            }
            t->argb[py * width + px] = color;
        }
    }
}
static inline void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    int unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    NGLTexture *t;
    (void)target;
    if (level != 0) return;
    if (unit < 0 || type != GL_UNSIGNED_BYTE || pixels == NULL ||
        width < 0 || height < 0 || xoffset < 0 || yoffset < 0) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    t = ngl_sw_texture(ngl_sw.bound_texture[unit]);
    if (t == NULL || t->argb == NULL || xoffset + width > t->width ||
        yoffset + height > t->height ||
        format == GL_PALETTE8_RGB8_OES ||
        format == GL_PALETTE8_RGBA8_OES ||
        format == GL_M3G_PALETTE8_RGB8_32) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    ngl_sw_upload_pixels(t, xoffset, yoffset, width, height, format,
                         (const unsigned char *)pixels);
}
static inline void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    NGLTexture *t;
    int unit;
    int x, y;
    const unsigned char *src;
    (void)target; (void)internalformat; (void)border;
    if (level != 0) return;
    unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (unit < 0 || type != GL_UNSIGNED_BYTE) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    if (ngl_sw.trace_upload_budget > 0) {
        fprintf(stderr, "[M3G NGL tex] unit=%d size=%dx%d format=0x%x\n",
                unit, (int)width, (int)height, (unsigned int)format);
        --ngl_sw.trace_upload_budget;
    }
    t = ngl_sw_texture(ngl_sw.bound_texture[unit]);
    free(t->argb);
    t->argb = NULL;
    t->width = width;
    t->height = height;
    if (pixels == NULL || width <= 0 || height <= 0) return;
    t->argb = (unsigned int *)malloc((size_t)width * (size_t)height * sizeof(unsigned int));
    if (t->argb == NULL) { ngl_sw.error = GL_OUT_OF_MEMORY; return; }
    src = (const unsigned char *)pixels;
    if (format == GL_PALETTE8_RGB8_OES || format == GL_PALETTE8_RGBA8_OES ||
        format == GL_M3G_PALETTE8_RGB8_32) {
        int ps = format == GL_PALETTE8_RGB8_OES ? 3 : 4;
        const unsigned char *pal = src;
        const unsigned char *idx = src + 256 * ps;
        for (y = 0; y < height; ++y) for (x = 0; x < width; ++x) {
            int pi = idx[y * width + x] & 0xff;
            const unsigned char *p = pal + pi * ps;
            t->argb[y * width + x] =
                ngl_sw_pack(p[0], p[1], p[2],
                            format == GL_PALETTE8_RGBA8_OES ? p[3] : 255);
        }
    } else {
        ngl_sw_upload_pixels(t, 0, 0, width, height, format, src);
    }
}
static inline void glDrawArrays(GLenum mode, GLint first, GLsizei count) { ngl_sw_draw_indexed(mode, count, GL_UNSIGNED_SHORT, NULL, first); }
static inline void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) { ngl_sw_draw_indexed(mode, count, type, indices, 0); }
static inline void glClear(GLbitfield mask) {
    int x, y, x0, y0, x1, y1;
    x0 = ngl_sw.scissor_test ? ngl_sw.scissor[0] : 0;
    y0 = ngl_sw.scissor_test ? ngl_sw.scissor[1] : 0;
    x1 = x0 + (ngl_sw.scissor_test ? ngl_sw.scissor[2] : ngl_sw.width);
    y1 = y0 + (ngl_sw.scissor_test ? ngl_sw.scissor[3] : ngl_sw.height);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > ngl_sw.width) x1 = ngl_sw.width;
    if (y1 > ngl_sw.height) y1 = ngl_sw.height;
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            int fy = ngl_sw.height - 1 - y;
            int offset = fy * ngl_sw.stride + x;
            if ((mask & GL_COLOR_BUFFER_BIT) && ngl_sw.pixels != NULL) {
                unsigned int dst = ngl_sw_rgb565_to_argb(ngl_sw.pixels[offset]);
                ngl_sw.pixels[offset] =
                    ngl_sw_rgb565(ngl_sw_apply_color_mask(ngl_sw.clear_color, dst));
            }
            if ((mask & GL_DEPTH_BUFFER_BIT) && ngl_sw.depth != NULL) {
                ngl_sw.depth[offset] = ngl_sw.clear_depth;
            }
        }
    }
}
static inline void glDeleteTextures(GLsizei n, const GLuint *textures) {
    GLsizei i;
    int slot;
    int unit;
    if (textures == NULL) return;
    for (i = 0; i < n; ++i) {
        if (textures[i] == 0) continue;
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            if (ngl_sw.bound_texture[unit] == textures[i]) {
                ngl_sw.bound_texture[unit] = 0;
            }
        }
        for (slot = 1; slot < NGL_SW_MAX_TEXTURES; ++slot) {
            if (ngl_sw.textures[slot].id == textures[i]) {
                free(ngl_sw.textures[slot].argb);
                memset(&ngl_sw.textures[slot], 0, sizeof(ngl_sw.textures[slot]));
                break;
            }
        }
    }
}
static inline void *nglCreateTextureManager(void) { return (void *)1; }
static inline void nglDeleteTextureManager(void *manager) { (void)manager; }
static inline int nglInit(void *memory, unsigned memoryBytes, void *textureManager, int flags) {
    int unit;
    int light;
    (void)memory; (void)memoryBytes; (void)textureManager; (void)flags;
    memset(&ngl_sw, 0, sizeof(ngl_sw));
    ngl_sw.matrix_mode = GL_MODELVIEW;
    ngl_sw.active_texture = GL_TEXTURE0;
    ngl_sw.client_texture = GL_TEXTURE0;
    ngl_sw.depth_mask = GL_TRUE;
    ngl_sw.depth_func = GL_LEQUAL;
    ngl_sw.alpha_func = GL_ALWAYS;
    ngl_sw.blend_src = GL_SRC_ALPHA;
    ngl_sw.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
    ngl_sw.fog_mode = GL_EXP;
    ngl_sw.shade_model = GL_SMOOTH;
    ngl_sw.fog_density = 1.0f;
    ngl_sw.fog_end = 1.0f;
    ngl_sw.clear_depth = 1.0f;
    ngl_sw.depth_near = 0.0f;
    ngl_sw.depth_far = 1.0f;
    ngl_sw.color_mask[0] = GL_TRUE;
    ngl_sw.color_mask[1] = GL_TRUE;
    ngl_sw.color_mask[2] = GL_TRUE;
    ngl_sw.color_mask[3] = GL_TRUE;
    ngl_sw.current_color = 0xffffffffU;
    ngl_sw.cull_mode = GL_BACK;
    ngl_sw.front_face = GL_CCW;
    ngl_sw.color_material_face = GL_FRONT_AND_BACK;
    ngl_sw.color_material_mode = GL_AMBIENT | GL_DIFFUSE;
    ngl_sw.material.ambient[0] = 0.2f;
    ngl_sw.material.ambient[1] = 0.2f;
    ngl_sw.material.ambient[2] = 0.2f;
    ngl_sw.material.ambient[3] = 1.0f;
    ngl_sw.material.diffuse[0] = 0.8f;
    ngl_sw.material.diffuse[1] = 0.8f;
    ngl_sw.material.diffuse[2] = 0.8f;
    ngl_sw.material.diffuse[3] = 1.0f;
    ngl_sw.material.specular[3] = 1.0f;
    ngl_sw.material.emission[3] = 1.0f;
    ngl_sw.light_model_ambient[0] = 0.2f;
    ngl_sw.light_model_ambient[1] = 0.2f;
    ngl_sw.light_model_ambient[2] = 0.2f;
    ngl_sw.light_model_ambient[3] = 1.0f;
    for (light = 0; light < 8; ++light) {
        ngl_sw.lights[light].position[2] = 1.0f;
        ngl_sw.lights[light].spot_direction[2] = -1.0f;
        ngl_sw.lights[light].spot_cutoff = 180.0f;
        ngl_sw.lights[light].constant_attenuation = 1.0f;
        ngl_sw.lights[light].ambient[3] = 1.0f;
        ngl_sw.lights[light].diffuse[3] = 1.0f;
        ngl_sw.lights[light].specular[3] = 1.0f;
    }
    ngl_sw_identity(ngl_sw.modelview);
    ngl_sw_identity(ngl_sw.projection);
    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        ngl_sw.tex_env_mode[unit] = GL_MODULATE;
        ngl_sw.tex_env_color[unit] = 0xff000000U;
        ngl_sw_identity(ngl_sw.texture_matrix[unit]);
    }
    return 1;
}
static inline void nglExit(void) {
    int i;
    for (i = 0; i < NGL_SW_MAX_TEXTURES; ++i) free(ngl_sw.textures[i].argb);
    free(ngl_sw.depth);
    memset(&ngl_sw, 0, sizeof(ngl_sw));
}
static inline void nglInitTextures(GLsizei count, const GLuint *textures) { GLsizei i; for (i = 0; i < count; ++i) ngl_sw_texture(textures[i]); }
static inline void nglBindTextureInternal(GLenum target, GLuint texture) { glBindTexture(target, texture); }

#endif
