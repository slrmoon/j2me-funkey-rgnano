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
#include <stdlib.h>
#include <string.h>

#define NGL_SW_MAX_TEXTURES 1024
#define NGL_SW_MAX_MIP_LEVELS 12
#define NGL_SW_STACK_DEPTH 16
#define NGL_SW_TEXTURE_UNITS 2

typedef struct {
    GLint size;
    GLenum type;
    GLsizei stride;
    const GLvoid *ptr;
    GLboolean enabled;
} NGLArray;

typedef struct NGLTextureS {
    GLuint id;
    int width;
    int height;
    unsigned int *argb;
    GLint wrap_s;
    GLint wrap_t;
    GLint min_filter;
    GLint mag_filter;
    /* Box-filtered levels 1..mip_count, built on first mipmapped use and
     * dropped whenever level 0 changes. */
    struct NGLTextureS *mips;
    int mip_count;
    /* Base format components; GL texture environments take the missing
     * ones from the fragment (e.g. REPLACE with an RGB texture keeps the
     * fragment alpha). 0 means "has it", so zeroed slots act as RGBA. */
    unsigned char no_color;
    unsigned char no_alpha;
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
    GLfloat spot_cos_cutoff;  /* cos(spot_cutoff), kept in sync by glLightf */
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
    unsigned short *depth;
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
    /* Resolved slot for bound_texture[]; NULL means "look it up again". */
    NGLTexture *bound_ptr[NGL_SW_TEXTURE_UNITS];
    /* Per-draw transformed vertex cache (NGLVertex is declared later). */
    void *vcache;
    unsigned int *vcache_stamp;
    int vcache_cap;
    unsigned int vcache_draw;
    /* projection * modelview, set per draw (see ngl_sw_xform_begin) */
    float mvp[16];
    /* Per-draw lighting setup (see ngl_sw_light_begin). */
    float normal_matrix[9];
    float light_dir[8][3];   /* directional: normalized direction */
    float light_half[8][3];  /* directional: infinite-viewer half vector */
    GLenum tex_env_mode[NGL_SW_TEXTURE_UNITS];
    unsigned int tex_env_color[NGL_SW_TEXTURE_UNITS];
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
    /* one type dispatch per vector instead of per component */
    step = a->size < want ? a->size : want;
    switch (a->type) {
    case GL_SHORT:
        for (i = 0; i < step; ++i) v[i] = (float)((const short *)p)[i];
        break;
    case GL_BYTE:
        for (i = 0; i < step; ++i) v[i] = (float)((const signed char *)p)[i];
        break;
    case GL_UNSIGNED_BYTE:
        for (i = 0; i < step; ++i) v[i] = (float)p[i];
        break;
    case GL_UNSIGNED_SHORT:
        for (i = 0; i < step; ++i) v[i] = (float)((const unsigned short *)p)[i];
        break;
    case GL_FIXED:
        for (i = 0; i < step; ++i) v[i] = (float)((const int *)p)[i] / 65536.0f;
        break;
    default:
        for (i = 0; i < step; ++i) v[i] = ((const float *)p)[i];
        break;
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

/* Per-draw projection * modelview (column-major, like GL). */
static void ngl_sw_xform_begin(void) {
    const float *p = ngl_sw.projection;
    const float *m = ngl_sw.modelview;
    int r, c;
    for (c = 0; c < 4; ++c) {
        for (r = 0; r < 4; ++r) {
            ngl_sw.mvp[c * 4 + r] = p[0 * 4 + r] * m[c * 4 + 0] +
                                    p[1 * 4 + r] * m[c * 4 + 1] +
                                    p[2 * 4 + r] * m[c * 4 + 2] +
                                    p[3 * 4 + r] * m[c * 4 + 3];
        }
    }
}

/*
 * Per-draw lighting setup: the normal matrix (inverse transpose of the
 * modelview 3x3, as GL uses) and the per-light vectors that do not depend
 * on the vertex. GL ES 1.x has no local viewer, so the eye direction is
 * (0, 0, 1) and directional half vectors are constant per light.
 */
static void ngl_sw_light_begin(void) {
    const float *m = ngl_sw.modelview;
    float c00, c01, c02, c10, c11, c12, c20, c21, c22, det, k;
    int i;
    if (!ngl_sw.lighting) return;
    /* M(r,k) = m[k*4+r]; cofactors of the upper 3x3. */
    c00 = m[5] * m[10] - m[9] * m[6];
    c01 = -(m[1] * m[10] - m[9] * m[2]);
    c02 = m[1] * m[6] - m[5] * m[2];
    c10 = -(m[4] * m[10] - m[8] * m[6]);
    c11 = m[0] * m[10] - m[8] * m[2];
    c12 = -(m[0] * m[6] - m[4] * m[2]);
    c20 = m[4] * m[9] - m[8] * m[5];
    c21 = -(m[0] * m[9] - m[8] * m[1]);
    c22 = m[0] * m[5] - m[4] * m[1];
    det = m[0] * c00 + m[4] * c01 + m[8] * c02;
    /* Normals are renormalized later, only the sign of 1/det matters. */
    k = det < 0.0f ? -1.0f : 1.0f;
    ngl_sw.normal_matrix[0] = c00 * k; ngl_sw.normal_matrix[1] = c01 * k;
    ngl_sw.normal_matrix[2] = c02 * k; ngl_sw.normal_matrix[3] = c10 * k;
    ngl_sw.normal_matrix[4] = c11 * k; ngl_sw.normal_matrix[5] = c12 * k;
    ngl_sw.normal_matrix[6] = c20 * k; ngl_sw.normal_matrix[7] = c21 * k;
    ngl_sw.normal_matrix[8] = c22 * k;
    for (i = 0; i < 8; ++i) {
        NGLLight *light = &ngl_sw.lights[i];
        float *l = ngl_sw.light_dir[i];
        float *h = ngl_sw.light_half[i];
        if (!ngl_sw.light_enabled[i] || light->position[3] != 0.0f) continue;
        l[0] = light->position[0];
        l[1] = light->position[1];
        l[2] = light->position[2];
        ngl_sw_normalize3(l);
        h[0] = l[0];
        h[1] = l[1];
        h[2] = l[2] + 1.0f;
        ngl_sw_normalize3(h);
    }
}

static unsigned int ngl_sw_lit_color(int index, const float *eye,
                                     unsigned int base_color) {
    float normal[4];
    float n[3];
    float material_ambient[4];
    float material_diffuse[4];
    float result[4];
    const float *nm = ngl_sw.normal_matrix;
    int i;
    if (!ngl_sw.lighting) {
        return base_color;
    }
    if (ngl_sw.normal.enabled && ngl_sw.normal.ptr != NULL) {
        ngl_sw_vec(&ngl_sw.normal, index, normal, 3);
    } else {
        /* GL's current normal; m3gcore never changes it. */
        normal[0] = 0.0f;
        normal[1] = 0.0f;
        normal[2] = 1.0f;
    }
    n[0] = nm[0] * normal[0] + nm[1] * normal[1] + nm[2] * normal[2];
    n[1] = nm[3] * normal[0] + nm[4] * normal[1] + nm[5] * normal[2];
    n[2] = nm[6] * normal[0] + nm[7] * normal[1] + nm[8] * normal[2];
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
        float lbuf[3];
        float hbuf[3];
        const float *l;
        const float *h;
        float attenuation = 1.0f;
        float ndotl;
        int c;
        if (!ngl_sw.light_enabled[i]) {
            continue;
        }
        if (light->position[3] == 0.0f) {
            l = ngl_sw.light_dir[i];
            h = ngl_sw.light_half[i];
        } else {
            float inv_w = 1.0f / light->position[3];
            float lx = light->position[0] * inv_w - eye[0];
            float ly = light->position[1] * inv_w - eye[1];
            float lz = light->position[2] * inv_w - eye[2];
            float dist2 = lx * lx + ly * ly + lz * lz;
            float dist = sqrtf(dist2);
            if (dist > 0.000001f) {
                float denom = light->constant_attenuation +
                              light->linear_attenuation * dist +
                              light->quadratic_attenuation * dist2;
                float inv = 1.0f / dist;
                if (denom > 0.000001f) attenuation = 1.0f / denom;
                lx *= inv; ly *= inv; lz *= inv;
            }
            lbuf[0] = lx; lbuf[1] = ly; lbuf[2] = lz;
            l = lbuf;
            if (light->spot_cutoff < 180.0f) {
                /* spot_direction is unit length (normalized on set). */
                float spot = -(lx * light->spot_direction[0] +
                               ly * light->spot_direction[1] +
                               lz * light->spot_direction[2]);
                if (spot < light->spot_cos_cutoff) {
                    attenuation = 0.0f;
                } else if (light->spot_exponent > 0.0f) {
                    attenuation *= powf(spot, light->spot_exponent);
                }
            }
            hbuf[0] = lx;
            hbuf[1] = ly;
            hbuf[2] = lz + 1.0f;
            ngl_sw_normalize3(hbuf);
            h = hbuf;
        }
        if (attenuation <= 0.0f) {
            continue;
        }
        /* GL scales the whole per-light term, ambient included, by the
         * attenuation and spot factors. */
        for (c = 0; c < 3; ++c) {
            result[c] += attenuation * material_ambient[c] * light->ambient[c];
        }
        ndotl = ngl_sw_dot3(n, l);
        if (ndotl < 0.0f && ngl_sw.light_model_two_side) {
            ndotl = -ndotl;
        }
        if (ndotl > 0.0f) {
            float spec = ngl_sw_dot3(n, h);
            int has_spec = ngl_sw.material.specular[0] != 0.0f ||
                           ngl_sw.material.specular[1] != 0.0f ||
                           ngl_sw.material.specular[2] != 0.0f;
            if (spec < 0.0f) spec = 0.0f;
            if (has_spec) {
                /* GL: (n.h)^shininess, which is 1 for shininess 0. */
                spec = ngl_sw.material.shininess > 0.0f ?
                       powf(spec, ngl_sw.material.shininess) : 1.0f;
            } else {
                spec = 0.0f;
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
    int hint;
    if (id == 0) {
        return &ngl_sw.textures[0];
    }
    /* Ids come from a counter, so id-modulo usually lands on the slot. */
    hint = NGL_SW_TEXTURE_UNITS +
           (int)(id % (GLuint)(NGL_SW_MAX_TEXTURES - NGL_SW_TEXTURE_UNITS));
    if (ngl_sw.textures[hint].id == id) {
        return &ngl_sw.textures[hint];
    }
    if (ngl_sw.textures[hint].id == 0) {
        for (i = NGL_SW_TEXTURE_UNITS; i < NGL_SW_MAX_TEXTURES; ++i) {
            if (ngl_sw.textures[i].id == id) return &ngl_sw.textures[i];
        }
        ngl_sw.textures[hint].id = id;
        ngl_sw.textures[hint].wrap_s = GL_CLAMP_TO_EDGE;
        ngl_sw.textures[hint].wrap_t = GL_CLAMP_TO_EDGE;
        ngl_sw.textures[hint].min_filter = GL_NEAREST;
        ngl_sw.textures[hint].mag_filter = GL_NEAREST;
        return &ngl_sw.textures[hint];
    }
    for (i = NGL_SW_TEXTURE_UNITS; i < NGL_SW_MAX_TEXTURES; ++i) {
        if (ngl_sw.textures[i].id == id) {
            return &ngl_sw.textures[i];
        }
    }
    for (i = NGL_SW_TEXTURE_UNITS; i < NGL_SW_MAX_TEXTURES; ++i) {
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

static NGLTexture *ngl_sw_bound_texture(int unit) {
    if (unit < 0 || unit >= NGL_SW_TEXTURE_UNITS) {
        return NULL;
    }
    if (ngl_sw.bound_ptr[unit] == NULL) {
        ngl_sw.bound_ptr[unit] = ngl_sw.bound_texture[unit] == 0 ?
                                 &ngl_sw.textures[unit] :
                                 ngl_sw_texture(ngl_sw.bound_texture[unit]);
    }
    return ngl_sw.bound_ptr[unit];
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

static void ngl_sw_texture_drop_mips(NGLTexture *t) {
    int i;
    if (t == NULL || t->mips == NULL) return;
    for (i = 0; i < t->mip_count; ++i) free(t->mips[i].argb);
    free(t->mips);
    t->mips = NULL;
    t->mip_count = 0;
}

static int ngl_sw_min_filter_is_mipmapped(GLint f) {
    return f == GL_NEAREST_MIPMAP_NEAREST || f == GL_LINEAR_MIPMAP_NEAREST ||
           f == GL_NEAREST_MIPMAP_LINEAR || f == GL_LINEAR_MIPMAP_LINEAR;
}

/* Builds levels 1.. by 2x2 box filtering (edge texels repeated for odd
 * sizes). Returns the number of levels beyond the base. */
static int ngl_sw_texture_build_mips(NGLTexture *t) {
    int level;
    const NGLTexture *src = t;
    if (t->mips != NULL) return t->mip_count;
    if (t->argb == NULL || (t->width <= 1 && t->height <= 1)) return 0;
    t->mips = (NGLTexture *)calloc(NGL_SW_MAX_MIP_LEVELS, sizeof(NGLTexture));
    if (t->mips == NULL) return 0;
    for (level = 0; level < NGL_SW_MAX_MIP_LEVELS &&
                    (src->width > 1 || src->height > 1); ++level) {
        NGLTexture *dst = &t->mips[level];
        int w = src->width > 1 ? src->width / 2 : 1;
        int h = src->height > 1 ? src->height / 2 : 1;
        int x, y;
        dst->argb = (unsigned int *)malloc((size_t)w * (size_t)h * sizeof(unsigned int));
        if (dst->argb == NULL) break;
        dst->width = w;
        dst->height = h;
        for (y = 0; y < h; ++y) {
            int sy0 = y * 2 < src->height ? y * 2 : src->height - 1;
            int sy1 = sy0 + 1 < src->height ? sy0 + 1 : sy0;
            const unsigned int *r0 = src->argb + sy0 * src->width;
            const unsigned int *r1 = src->argb + sy1 * src->width;
            for (x = 0; x < w; ++x) {
                int sx0 = x * 2 < src->width ? x * 2 : src->width - 1;
                int sx1 = sx0 + 1 < src->width ? sx0 + 1 : sx0;
                unsigned int p[4];
                unsigned int rb, ag;
                p[0] = r0[sx0]; p[1] = r0[sx1]; p[2] = r1[sx0]; p[3] = r1[sx1];
                rb = (p[0] & 0x00ff00ffU) + (p[1] & 0x00ff00ffU) +
                     (p[2] & 0x00ff00ffU) + (p[3] & 0x00ff00ffU) + 0x00020002U;
                ag = ((p[0] >> 8) & 0x00ff00ffU) + ((p[1] >> 8) & 0x00ff00ffU) +
                     ((p[2] >> 8) & 0x00ff00ffU) + ((p[3] >> 8) & 0x00ff00ffU) +
                     0x00020002U;
                dst->argb[y * w + x] = ((rb >> 2) & 0x00ff00ffU) |
                                       ((ag << 6) & 0xff00ff00U);
            }
        }
        ++t->mip_count;
        src = dst;
    }
    return t->mip_count;
}

/* a*(256-f) + b*f per channel, f in [0,256]; R/B and A/G lanes are 16 bits
 * apart so the weighted sums cannot carry into the next lane. */
static unsigned int ngl_sw_lerp8(unsigned int a, unsigned int b, unsigned int f) {
    unsigned int g = 256U - f;
    unsigned int rb = (((a & 0x00ff00ffU) * g + (b & 0x00ff00ffU) * f) >> 8) & 0x00ff00ffU;
    unsigned int ag = (((a >> 8) & 0x00ff00ffU) * g + ((b >> 8) & 0x00ff00ffU) * f) & 0xff00ff00U;
    return rb | ag;
}

static int ngl_sw_wrap_coord(int c, int extent, GLint wrap) {
    if (wrap == GL_REPEAT) {
        if ((extent & (extent - 1)) == 0) return c & (extent - 1);
        c %= extent;
        return c < 0 ? c + extent : c;
    }
    if (c < 0) return 0;
    return c >= extent ? extent - 1 : c;
}

static unsigned int ngl_sw_texel_bilinear(const NGLTexture *t, float s, float tt) {
    float fx = s * (float)t->width - 0.5f;
    float fy = tt * (float)t->height - 0.5f;
    int x0 = (int)fx;
    int y0 = (int)fy;
    unsigned int wx, wy;
    int x1, y1;
    const unsigned int *r0;
    const unsigned int *r1;
    if (fx < (float)x0) --x0;
    if (fy < (float)y0) --y0;
    wx = (unsigned int)((fx - (float)x0) * 256.0f);
    wy = (unsigned int)((fy - (float)y0) * 256.0f);
    if (wx > 256U) wx = 256U;
    if (wy > 256U) wy = 256U;
    x1 = ngl_sw_wrap_coord(x0 + 1, t->width, t->wrap_s);
    y1 = ngl_sw_wrap_coord(y0 + 1, t->height, t->wrap_t);
    x0 = ngl_sw_wrap_coord(x0, t->width, t->wrap_s);
    y0 = ngl_sw_wrap_coord(y0, t->height, t->wrap_t);
    r0 = t->argb + y0 * t->width;
    r1 = t->argb + y1 * t->width;
    return ngl_sw_lerp8(ngl_sw_lerp8(r0[x0], r0[x1], wx),
                        ngl_sw_lerp8(r1[x0], r1[x1], wx), wy);
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
    if (t->mag_filter == GL_LINEAR ||
        t->min_filter == GL_LINEAR ||
        t->min_filter == GL_LINEAR_MIPMAP_NEAREST ||
        t->min_filter == GL_LINEAR_MIPMAP_LINEAR) {
        return ngl_sw_texel_bilinear(t, s, tt);
    }
    /* GL nearest sampling: texel i covers [i/w, (i+1)/w). */
    x = (int)floorf(s * (float)t->width);
    y = (int)floorf(tt * (float)t->height);
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

static void ngl_sw_texture_set_components(NGLTexture *t, GLenum format) {
    t->no_color = format == GL_ALPHA;
    t->no_alpha = format == GL_LUMINANCE || format == GL_RGB ||
                  format == GL_M3G_RGB565 || format == GL_M3G_RGB8_32 ||
                  format == GL_M3G_BGR8_32 || format == GL_PALETTE8_RGB8_OES ||
                  format == GL_M3G_PALETTE8_RGB8_32;
}

static unsigned int ngl_sw_apply_tex_env(int unit, const NGLTexture *t,
                                         unsigned int color,
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
        unsigned int rgb = t->no_color ? (color & 0x00ffffffU) : (texel & 0x00ffffffU);
        unsigned int a = t->no_alpha ? (color & 0xff000000U) : (texel & 0xff000000U);
        return rgb | a;
    }
    if (t->no_color && ngl_sw.tex_env_mode[unit] == GL_BLEND) {
        /* ALPHA textures: Cv = Cf, Av = Af * At */
        return (color & 0x00ffffffU) | ((ca * ta / 255U) << 24);
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

static int ngl_sw_depth_compare(GLenum func, unsigned short lhs,
                                unsigned short rhs) {
    switch (func) {
    case GL_NEVER: return 0;
    case GL_LESS: return lhs < rhs;
    case GL_EQUAL: return lhs == rhs;
    case GL_LEQUAL: return lhs <= rhs;
    case GL_GREATER: return lhs > rhs;
    case GL_NOTEQUAL: return lhs != rhs;
    case GL_GEQUAL: return lhs >= rhs;
    case GL_ALWAYS:
    default: return 1;
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
    if (ngl_sw.blend_src == GL_SRC_ALPHA &&
            ngl_sw.blend_dst == GL_ONE_MINUS_SRC_ALPHA) {
        unsigned int a = (src >> 24) & 0xffU;
        return ngl_sw_lerp8(dst, src, a + (a >> 7)) | 0xff000000U;
    }
    if (ngl_sw.blend_src == GL_ONE && ngl_sw.blend_dst == GL_ONE) {
        unsigned int r = ((src >> 16) & 0xffU) + ((dst >> 16) & 0xffU);
        unsigned int g = ((src >> 8) & 0xffU) + ((dst >> 8) & 0xffU);
        unsigned int b = (src & 0xffU) + (dst & 0xffU);
        return ngl_sw_pack(r, g, b, 255U);
    }
    if (ngl_sw.blend_src == GL_ONE && ngl_sw.blend_dst == GL_ZERO) {
        return src | 0xff000000U;
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
    unsigned int outcode;   /* clip planes outside (transformed vertices only) */
} NGLVertex;

static unsigned int ngl_sw_outcode(const NGLVertex *v);

static void ngl_sw_transform_vertex(int index, NGLVertex *out) {
    float p[4];
    float mv[4];
    float clip[4];
    float tc[4];
    int unit;
    ngl_sw_vec(&ngl_sw.vertex, index, p, 4);
    if (ngl_sw.lighting) {
        /* lighting needs the eye-space position anyway */
        ngl_sw_transform_vec4(mv, ngl_sw.modelview, p);
        ngl_sw_transform_vec4(clip, ngl_sw.projection, mv);
    } else {
        const float *m = ngl_sw.mvp;
        const float *e = ngl_sw.modelview;
        if (p[3] == 1.0f) {
            clip[0] = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
            clip[1] = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
            clip[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
            clip[3] = m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15];
            mv[2] = e[2] * p[0] + e[6] * p[1] + e[10] * p[2] + e[14];
        } else {
            ngl_sw_transform_vec4(clip, m, p);
            mv[2] = e[2] * p[0] + e[6] * p[1] + e[10] * p[2] + e[14] * p[3];
        }
    }
    out->x = clip[0];
    out->y = clip[1];
    out->z = clip[2];
    out->w = clip[3];
    /* GL ES fog coordinate -z_eye (signed). m3gcore's NGL sprite path draws
     * in screen space with fog start = -finalZ/fogValue, end = 0, which
     * only yields the intended factor with the signed coordinate; fabsf
     * fully fogged every sprite with a positive NDC depth. Identical for
     * ordinary geometry in front of the camera. */
    out->fog = -mv[2];
    out->outcode = ngl_sw_outcode(out);
    out->color = ngl_sw_lit_color(index, mv, ngl_sw_color(index));
    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        out->u[unit] = 0.0f;
        out->v[unit] = 0.0f;
        if (ngl_sw.texcoord[unit].enabled && ngl_sw.texcoord[unit].ptr != NULL) {
            ngl_sw_vec(&ngl_sw.texcoord[unit], index, tc, 2);
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

static int ngl_sw_top_left(float ax, float ay, float bx, float by) {
    float dx = bx - ax;
    float dy = by - ay;
    return dy < 0.0f || (dy == 0.0f && dx > 0.0f);
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
    /* near plane with a little slack: vertices placed exactly on it
     * (full-screen quads at z = -near) must not be lost to rounding */
    case 5: return v->z + v->w * 1.0002f;
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

/* Reference rasterizer: float edge functions. Only used for triangles
 * whose screen coordinates are too large for the fixed-point fast path. */
static void ngl_sw_raster_tri_ref(NGLVertex a, NGLVertex b, NGLVertex c) {
    float ax, ay, bx, by, cx, cy;
    float aw, bw, cw;
    float az, bz, cz;
    float area;
    int minx, maxx, miny, maxy;
    int x, y;
    int scx0, scy0, scx1, scy1;
    float depth_offset = 0.0f;
    NGLTexture *tex_unit[NGL_SW_TEXTURE_UNITS];
    if (ngl_sw.pixels == NULL || ngl_sw.width <= 0 || ngl_sw.height <= 0) {
        return;
    }
    if (a.w == 0.0f || b.w == 0.0f || c.w == 0.0f) {
        return;
    }
    /* Clipping already guarantees a positive w. Keep generated vertices on
     * the w epsilon plane instead of dropping camera-plane intersections. */
    if (a.w <= 0.0f || b.w <= 0.0f || c.w <= 0.0f) {
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
    if (area == 0.0f) {
        return;
    }
    if (ngl_sw.cull_face) {
        /* ngl_sw_edge(a, b, c) is the negated cross product, so a
         * counter-clockwise triangle has a negative area here. */
        int front = (area < 0.0f) == (ngl_sw.front_face == GL_CCW);
        if ((front && ngl_sw.cull_mode == GL_FRONT) ||
            (!front && ngl_sw.cull_mode == GL_BACK)) {
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
    {
        int unit;
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            NGLTexture *t = ngl_sw.texture_2d[unit] ? ngl_sw_bound_texture(unit) : NULL;
            tex_unit[unit] = (t != NULL && t->argb != NULL &&
                              t->width > 0 && t->height > 0) ? t : NULL;
        }
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
            float e0 = ngl_sw_edge(bx, by, cx, cy, px, py);
            float e1 = ngl_sw_edge(cx, cy, ax, ay, px, py);
            float e2 = ngl_sw_edge(ax, ay, bx, by, px, py);
            float w0;
            float w1;
            float w2;
            int fy;
            int offset;
            float z;
            unsigned short depth_value;
            float fog_coord;
            unsigned int color;
            float denom;
            if (e0 < 0.0f || (e0 == 0.0f && !ngl_sw_top_left(bx, by, cx, cy)) ||
                e1 < 0.0f || (e1 == 0.0f && !ngl_sw_top_left(cx, cy, ax, ay)) ||
                e2 < 0.0f || (e2 == 0.0f && !ngl_sw_top_left(ax, ay, bx, by))) {
                continue;
            }
            w0 = e0 / area;
            w1 = e1 / area;
            w2 = e2 / area;
            z = w0 * az + w1 * bz + w2 * cz;
            z = ngl_sw.depth_near + z * (ngl_sw.depth_far - ngl_sw.depth_near);
            z += depth_offset;
            z = ngl_sw_clampf(z, 0.0f, 1.0f);
            depth_value = (unsigned short)(z * 65535.0f + 0.5f);
            fy = ngl_sw.height - 1 - y;
            if (fy < 0 || fy >= ngl_sw.height || x < 0 || x >= ngl_sw.width) continue;
            offset = fy * ngl_sw.stride + x;
            if (ngl_sw.depth_test && ngl_sw.depth != NULL) {
                if (!ngl_sw_depth_compare(ngl_sw.depth_func, depth_value,
                                          ngl_sw.depth[offset])) {
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
                    NGLTexture *tex = tex_unit[unit];
                    if (tex != NULL) {
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
                        color = ngl_sw_apply_tex_env(unit, tex, color, texel);
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
                ngl_sw.depth[offset] = depth_value;
            }
            color = ngl_sw_apply_fog(color, fog_coord);
            {
                unsigned int dst = ngl_sw_rgb565_to_argb(ngl_sw.pixels[offset]);
                color = ngl_sw_blend(color, dst);
                color = ngl_sw_apply_color_mask(color, dst);
            }
            ngl_sw.pixels[offset] = ngl_sw_rgb565(color);
        }
    }
}

/*
 * Fast triangle rasterizer.
 *
 * Screen positions are snapped to a subpixel grid and edge functions are
 * evaluated in exact integer arithmetic, so triangles sharing an edge never
 * leave gaps or touch a pixel twice. Attributes use plane equations driven by
 * the integer barycentrics, which costs at most one division per pixel (none
 * when w is constant across the triangle). Diagnostics keep using the
 * reference rasterizer above, which also serves extreme coordinates.
 */
static int ngl_sw_floor_int(float f) {
    int i = (int)f;
    return (f < (float)i) ? i - 1 : i;
}

/* floorf without the libm call for the usual range; huge or NaN values
 * (degenerate w) keep the exact libm behaviour. */
static float ngl_sw_floorf(float f) {
    if (f > -8388608.0f && f < 8388608.0f) return (float)ngl_sw_floor_int(f);
    return floorf(f);
}

static int ngl_sw_is_pow2(int v) {
    return v > 0 && (v & (v - 1)) == 0;
}

/* Nearest-filter fetch without floorf/branches on the common paths. */
static unsigned int ngl_sw_texel_nearest(const NGLTexture *t, float s, float tt) {
    int x = ngl_sw_floor_int(s * (float)t->width);
    int y = ngl_sw_floor_int(tt * (float)t->height);
    if (t->wrap_s == GL_REPEAT) {
        if (ngl_sw_is_pow2(t->width)) x &= t->width - 1;
        else { x %= t->width; if (x < 0) x += t->width; }
    } else {
        if (x < 0) x = 0;
        if (x >= t->width) x = t->width - 1;
    }
    if (t->wrap_t == GL_REPEAT) {
        if (ngl_sw_is_pow2(t->height)) y &= t->height - 1;
        else { y %= t->height; if (y < 0) y += t->height; }
    } else {
        if (y < 0) y = 0;
        if (y >= t->height) y = t->height - 1;
    }
    return t->argb[y * t->width + x];
}

static int ngl_sw_texture_is_nearest(const NGLTexture *t) {
    return !(t->mag_filter == GL_LINEAR ||
             t->min_filter == GL_LINEAR ||
             t->min_filter == GL_LINEAR_MIPMAP_NEAREST ||
             t->min_filter == GL_LINEAR_MIPMAP_LINEAR);
}

typedef struct {
    float a, db, dc;  /* value = a + l1 * db + l2 * dc */
} NGLPlane;

/* round(log2(sqrt(rho2))) from the float exponent; rho2 > 1. */
static int ngl_sw_mip_level(float rho2) {
    unsigned int bits;
    int e;
    memcpy(&bits, &rho2, sizeof(bits));
    e = (int)((bits >> 23) & 0xffU) - 127;  /* floor(log2(rho2)) */
    return (e + 1) >> 1;
}

static void ngl_sw_plane(NGLPlane *p, float va, float vb, float vc) {
    p->a = va;
    p->db = vb - va;
    p->dc = vc - va;
}

#define NGL_SW_PLANE(p, l1, l2) ((p).a + (l1) * (p).db + (l2) * (p).dc)

/* Narrow [lo, hi) to the steps k where e + k * a >= 0 (one edge of a row).
 * Edge functions are linear in x, so this is exactly the set a pixel scan
 * would find, without walking the empty part of the bounding box. */
static void ngl_sw_edge_range(int e, int a, int *lo, int *hi) {
    if (a > 0) {
        if (e < 0) {
            unsigned int k = ((unsigned int)(-e) + (unsigned int)a - 1U) / (unsigned int)a;
            if (k > (unsigned int)*hi) k = (unsigned int)*hi;
            if ((int)k > *lo) *lo = (int)k;
        }
    } else if (a < 0) {
        if (e < 0) {
            *hi = *lo;
        } else {
            unsigned int k = (unsigned int)e / (unsigned int)(-a) + 1U;
            if (k < (unsigned int)*hi) *hi = (int)k;
        }
    } else if (e < 0) {
        *hi = *lo;
    }
}

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && !defined(NGL_SW_NO_NEON)
#include <arm_neon.h>
#define NGL_SW_HAVE_NEON 1

/* Per-triangle state of the NEON span kernel. */
typedef struct {
    int depth_test;            /* 0: none/ALWAYS, 1: LESS, 2: LEQUAL */
    int depth_write;
    const unsigned int *tex;   /* NULL: untextured */
    int tw, wmask, hmask, repeat_s, repeat_t;
    int replace;               /* GL_REPLACE (else MODULATE) */
    int smooth;
    unsigned int flat_color;
} NGLSpanNeon;

/* same clamp/wrap as the scalar span loop */
static inline int ngl_sw_texel_index(const NGLSpanNeon *s, int u, int v) {
    int tx = u >> 16, ty = v >> 16;
    if (s->repeat_s) tx &= s->wmask;
    else if (tx < 0) tx = 0;
    else if (tx > s->wmask) tx = s->wmask;
    if (s->repeat_t) ty &= s->hmask;
    else if (ty < 0) ty = 0;
    else if (ty > s->hmask) ty = s->hmask;
    return ty * s->tw + tx;
}

static inline uint32x4_t ngl_sw_div255_neon(uint32x4_t x) {
    /* exact x / 255 for x <= 255 * 255 */
    return vshrq_n_u32(vaddq_u32(vaddq_u32(x, vshrq_n_u32(x, 8)), vdupq_n_u32(1)), 8);
}

/*
 * Four pixels per iteration of the span loop in ngl_sw_span_fast, for the
 * states without alpha test or blending. Same arithmetic as the scalar loop
 * (16.16 uv and colour, 20.12 depth, nearest texel, exact /255 modulate),
 * so the output is identical. count is a multiple of 4; the interpolators
 * are advanced by count pixels on return.
 */
static void ngl_sw_span_neon(const NGLSpanNeon *s, unsigned short *dst,
                             unsigned short *depth, int count,
                             int *zfix, int dzfix, int *su, int *sv,
                             int dsu, int dsv, int *ci, const int *dci) {
    static const int32_t lane[4] = {0, 1, 2, 3};
    const int32x4_t vi = vld1q_s32(lane);
    const int32x4_t zero = vdupq_n_s32(0);
    const int32x4_t z4 = vdupq_n_s32(dzfix * 4);
    const uint32x4_t m8 = vdupq_n_u32(0xffU);
    int32x4_t z = vmlaq_n_s32(vdupq_n_s32(*zfix), vi, dzfix);
    /* texture coordinates stay in core registers: NEON -> core moves stall
     * the Cortex-A7 pipeline, core -> NEON moves are cheap */
    int u = *su, v = *sv;
    int32x4_t cr = zero, cg = zero, cb = zero;
    int32x4_t cr4 = zero, cg4 = zero, cb4 = zero;
    uint32x4_t fr, fg, fb;
    int white = !s->smooth && (s->flat_color & 0x00ffffffU) == 0x00ffffffU;
    int k;

    fr = vdupq_n_u32((s->flat_color >> 16) & 0xffU);
    fg = vdupq_n_u32((s->flat_color >> 8) & 0xffU);
    fb = vdupq_n_u32(s->flat_color & 0xffU);
    if (s->smooth) {
        cr = vmlaq_n_s32(vdupq_n_s32(ci[0]), vi, dci[0]);
        cg = vmlaq_n_s32(vdupq_n_s32(ci[1]), vi, dci[1]);
        cb = vmlaq_n_s32(vdupq_n_s32(ci[2]), vi, dci[2]);
        cr4 = vdupq_n_s32(dci[0] * 4);
        cg4 = vdupq_n_s32(dci[1] * 4);
        cb4 = vdupq_n_s32(dci[2] * 4);
    }

    for (k = 0; k < count; k += 4) {
        int32x4_t dz = vminq_s32(vmaxq_s32(vshrq_n_s32(z, 12), zero),
                                 vdupq_n_s32(65535));
        uint16x4_t dv = vmovn_u32(vreinterpretq_u32_s32(dz));
        uint16x4_t pass = vdup_n_u16(0xffffU);
        uint16x4_t old = dv;
        uint32x4_t r, g, b, out;

        if (s->depth_test) {
            old = vld1_u16(depth + k);
            pass = s->depth_test == 1 ? vclt_u16(dv, old) : vcle_u16(dv, old);
        }
        {
            if (s->smooth) {
                r = vreinterpretq_u32_s32(vminq_s32(vmaxq_s32(vshrq_n_s32(cr, 16), zero), vdupq_n_s32(255)));
                g = vreinterpretq_u32_s32(vminq_s32(vmaxq_s32(vshrq_n_s32(cg, 16), zero), vdupq_n_s32(255)));
                b = vreinterpretq_u32_s32(vminq_s32(vmaxq_s32(vshrq_n_s32(cb, 16), zero), vdupq_n_s32(255)));
            } else {
                r = fr; g = fg; b = fb;
            }
            if (s->tex != NULL) {
                unsigned int t0, t1, t2, t3;
                uint32x4_t t;
                t0 = s->tex[ngl_sw_texel_index(s, u, v)];
                t1 = s->tex[ngl_sw_texel_index(s, u + dsu, v + dsv)];
                t2 = s->tex[ngl_sw_texel_index(s, u + 2 * dsu, v + 2 * dsv)];
                t3 = s->tex[ngl_sw_texel_index(s, u + 3 * dsu, v + 3 * dsv)];
                t = vcombine_u32(vcreate_u32(((uint64_t)t1 << 32) | t0),
                                 vcreate_u32(((uint64_t)t3 << 32) | t2));
                if (s->replace || white) {
                    out = vorrq_u32(vorrq_u32(
                              vandq_u32(vshrq_n_u32(t, 8), vdupq_n_u32(0xf800U)),
                              vandq_u32(vshrq_n_u32(t, 5), vdupq_n_u32(0x07e0U))),
                              vandq_u32(vshrq_n_u32(t, 3), vdupq_n_u32(0x001fU)));
                    goto store;
                }
                r = ngl_sw_div255_neon(vmulq_u32(r, vandq_u32(vshrq_n_u32(t, 16), m8)));
                g = ngl_sw_div255_neon(vmulq_u32(g, vandq_u32(vshrq_n_u32(t, 8), m8)));
                b = ngl_sw_div255_neon(vmulq_u32(b, vandq_u32(t, m8)));
            }
            out = vorrq_u32(vorrq_u32(
                      vshlq_n_u32(vandq_u32(r, vdupq_n_u32(0xf8U)), 8),
                      vshlq_n_u32(vandq_u32(g, vdupq_n_u32(0xfcU)), 3)),
                      vshrq_n_u32(b, 3));
        store:
            vst1_u16(dst + k, vbsl_u16(pass, vmovn_u32(out), vld1_u16(dst + k)));
            if (s->depth_write) vst1_u16(depth + k, vbsl_u16(pass, dv, old));
        }
        z = vaddq_s32(z, z4);
        u += 4 * dsu;
        v += 4 * dsv;
        if (s->smooth) {
            cr = vaddq_s32(cr, cr4);
            cg = vaddq_s32(cg, cg4);
            cb = vaddq_s32(cb, cb4);
        }
    }
    *zfix += dzfix * count;
    *su += dsu * count;
    *sv += dsv * count;
    if (s->smooth) {
        ci[0] += dci[0] * count;
        ci[1] += dci[1] * count;
        ci[2] += dci[2] * count;
        ci[3] += dci[3] * count;
    }
}
#endif

/*
 * Span fast path for the common state: at most one texture (unit 0),
 * nearest, no mipmaps, REPLACE or MODULATE; no fog, blending or colour
 * mask; depth test off or LESS/LEQUAL/ALWAYS. Per row the covered span is
 * found with the integer edge functions alone, then attributes are stepped
 * across it instead of being rebuilt from barycentrics at every pixel:
 * depth in 20.12 fixed point, texture coordinates and colours in 16.16,
 * with the perspective division done every NGL_SW_SPAN pixels and linear
 * interpolation in between (subpixel error, invisible at 240 px). Returns
 * 0 when the state needs the general per-pixel loop.
 */
#define NGL_SW_SPAN 16

static int ngl_sw_span_fast(NGLTexture *const *tex_unit, const int *tex_nearest,
                            const int *tex_mips, int perspective, int smooth,
                            int rgb_mask_all, unsigned int flat_color,
                            float inv_area, const NGLPlane *pz,
                            const NGLPlane *pinvw, const NGLPlane *pu,
                            const NGLPlane *pv, const NGLPlane *pcol,
                            int minx, int maxx, int miny, int maxy,
                            int row0, int row1, int row2,
                            int a0, int a1, int a2, int b0, int b1, int b2,
                            int bias1, int bias2) {
    const NGLTexture *tex = tex_unit[0];
    int depth_on = ngl_sw.depth_test && ngl_sw.depth != NULL;
    int depth_less = depth_on && ngl_sw.depth_func == GL_LESS;
    int depth_write = depth_on && ngl_sw.depth_mask;
    int mode = GL_MODULATE;
    int repeat_s = 0, repeat_t = 0;
    int tw = 0, th = 0, wmask = 0, hmask = 0;
    float inv_tw = 0.0f, inv_th = 0.0f;  /* exact: repeat needs pow2 */
    float l1x = (float)a1 * inv_area, l2x = (float)a2 * inv_area;
    float dqx = 0.0f, duqx = 0.0f, dvqx = 0.0f, dzx;
    float dcx[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int y, ch;
    /* 0: plain write, 1: SRC_ALPHA/ONE_MINUS_SRC_ALPHA, 2: other blend */
    int blend_mode = 0;
#ifdef NGL_SW_HAVE_NEON
    NGLSpanNeon ns;
    int use_neon;
#endif

    if (!rgb_mask_all || ngl_sw.fog) return 0;
    if (ngl_sw.blend) {
        blend_mode = (ngl_sw.blend_src == GL_SRC_ALPHA &&
                      ngl_sw.blend_dst == GL_ONE_MINUS_SRC_ALPHA) ? 1 : 2;
    }
    if (depth_on && ngl_sw.depth_func != GL_LEQUAL &&
            ngl_sw.depth_func != GL_LESS && ngl_sw.depth_func != GL_ALWAYS) return 0;
    if (NGL_SW_TEXTURE_UNITS > 1 && tex_unit[1] != NULL) return 0;
    if (tex != NULL) {
        mode = ngl_sw.tex_env_mode[0];
        if (!tex_nearest[0] || tex_mips[0] || tex->no_color) return 0;
        if (mode != GL_MODULATE && mode != GL_REPLACE) return 0;
        tw = tex->width;
        th = tex->height;
        inv_tw = 1.0f / (float)tw;
        inv_th = 1.0f / (float)th;
        repeat_s = tex->wrap_s == GL_REPEAT;
        repeat_t = tex->wrap_t == GL_REPEAT;
        if ((repeat_s && !ngl_sw_is_pow2(tw)) || (repeat_t && !ngl_sw_is_pow2(th))) return 0;
        wmask = tw - 1;
        hmask = th - 1;
    }

#ifdef NGL_SW_HAVE_NEON
    use_neon = !ngl_sw.alpha_test && blend_mode == 0;
    ns.depth_test = (depth_on && ngl_sw.depth_func != GL_ALWAYS) ? (depth_less ? 1 : 2) : 0;
    ns.depth_write = depth_write;
    ns.tex = tex != NULL ? tex->argb : NULL;
    ns.tw = tw;
    ns.wmask = wmask;
    ns.hmask = hmask;
    ns.repeat_s = repeat_s;
    ns.repeat_t = repeat_t;
    ns.replace = mode == GL_REPLACE;
    ns.smooth = smooth;
    ns.flat_color = flat_color;
#endif

    dzx = pz->db * l1x + pz->dc * l2x;
    if (perspective) dqx = pinvw->db * l1x + pinvw->dc * l2x;
    if (tex != NULL) {
        duqx = pu[0].db * l1x + pu[0].dc * l2x;
        dvqx = pv[0].db * l1x + pv[0].dc * l2x;
    }
    if (smooth) {
        for (ch = 0; ch < 4; ++ch) dcx[ch] = pcol[ch].db * l1x + pcol[ch].dc * l2x;
    }

    for (y = miny; y < maxy; ++y, row0 += b0, row1 += b1, row2 += b2) {
        int e0 = row0, e1 = row1, e2 = row2;
        int xs, xe, x;
        unsigned short *dst_row;
        unsigned short *depth_row;
        float l1, l2, q, uq = 0.0f, vq = 0.0f, rw0;
        float cq[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float u0 = 0.0f, v0 = 0.0f, c0[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        int zfix, dzfix;

        /* covered span of this row (triangles are convex) */
        {
            int lo = 0, hi = maxx - minx;
            ngl_sw_edge_range(e0, a0, &lo, &hi);
            ngl_sw_edge_range(e1, a1, &lo, &hi);
            ngl_sw_edge_range(e2, a2, &lo, &hi);
            if (lo >= hi) continue;
            xs = minx + lo;
            xe = minx + hi;
            e1 += lo * a1;
            e2 += lo * a2;
        }
        dst_row = ngl_sw.pixels + (ngl_sw.height - 1 - y) * ngl_sw.stride;
        depth_row = depth_on ? ngl_sw.depth + (ngl_sw.height - 1 - y) * ngl_sw.stride : NULL;

        l1 = (float)(e1 - bias1) * inv_area;
        l2 = (float)(e2 - bias2) * inv_area;
        {
            float zf = NGL_SW_PLANE(*pz, l1, l2);
            zfix = (int)(zf * 4096.0f + 2048.0f);
            dzfix = (int)(dzx * 4096.0f);
        }
        q = perspective ? NGL_SW_PLANE(*pinvw, l1, l2) : 1.0f;
        rw0 = 1.0f / q;
        if (tex != NULL) {
            uq = NGL_SW_PLANE(pu[0], l1, l2);
            vq = NGL_SW_PLANE(pv[0], l1, l2);
            u0 = uq * rw0 * (float)tw;
            v0 = vq * rw0 * (float)th;
        }
        if (smooth) {
            for (ch = 0; ch < 4; ++ch) {
                cq[ch] = NGL_SW_PLANE(pcol[ch], l1, l2);
                c0[ch] = cq[ch] * rw0;
            }
        }

        for (x = xs; x < xe; ) {
            int n = xe - x < NGL_SW_SPAN ? xe - x : NGL_SW_SPAN;
            int i;
            float rw1, u1 = 0.0f, v1 = 0.0f, c1[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            int su = 0, sv = 0, dsu = 0, dsv = 0;
            int ci[4] = {0, 0, 0, 0}, dci[4] = {0, 0, 0, 0};
            float inv_n = 1.0f / (float)n;

            q += dqx * (float)n;
            rw1 = 1.0f / q;
            if (tex != NULL) {
                /* segment ends in texels; s0/s1, t0/t1 are working copies,
                 * u1/v1 stay unshifted as the next segment's start */
                float s0 = u0, t0 = v0, s1, t1;
                uq += duqx * (float)n;
                vq += dvqx * (float)n;
                u1 = uq * rw1 * (float)tw;
                v1 = vq * rw1 * (float)th;
                s1 = u1;
                t1 = v1;
                /* keep 16.16 in range: shift repeating coordinates by whole
                 * texture periods, clamp the others (indices clamp anyway) */
                if (repeat_s) {
                    float k = ngl_sw_floorf(s0 * inv_tw) * (float)tw;
                    s0 -= k;
                    s1 -= k;
                } else {
                    s0 = s0 < -8192.0f ? -8192.0f : (s0 > 8192.0f ? 8192.0f : s0);
                    s1 = s1 < -8192.0f ? -8192.0f : (s1 > 8192.0f ? 8192.0f : s1);
                }
                if (repeat_t) {
                    float k = ngl_sw_floorf(t0 * inv_th) * (float)th;
                    t0 -= k;
                    t1 -= k;
                } else {
                    t0 = t0 < -8192.0f ? -8192.0f : (t0 > 8192.0f ? 8192.0f : t0);
                    t1 = t1 < -8192.0f ? -8192.0f : (t1 > 8192.0f ? 8192.0f : t1);
                }
                if (s1 - s0 > 16384.0f || s0 - s1 > 16384.0f ||
                    t1 - t0 > 16384.0f || t0 - t1 > 16384.0f) {
                    s1 = s0;   /* absurd gradient (degenerate w): flat */
                    t1 = t0;
                }
                su = (int)(s0 * 65536.0f);
                sv = (int)(t0 * 65536.0f);
                dsu = (int)((s1 - s0) * inv_n * 65536.0f);
                dsv = (int)((t1 - t0) * inv_n * 65536.0f);
            }
            if (smooth) {
                for (ch = 0; ch < 4; ++ch) {
                    cq[ch] += dcx[ch] * (float)n;
                    c1[ch] = cq[ch] * rw1;
                    ci[ch] = (int)(c0[ch] * 65536.0f) + 32768;
                    dci[ch] = (int)((c1[ch] - c0[ch]) * inv_n * 65536.0f);
                }
            }

            i = 0;
#ifdef NGL_SW_HAVE_NEON
            if (use_neon && n >= 4) {
                int cnt = n & ~3;
                ngl_sw_span_neon(&ns, dst_row + x,
                                 depth_row != NULL ? depth_row + x : NULL, cnt,
                                 &zfix, dzfix, &su, &sv, dsu, dsv, ci, dci);
                i = cnt;
                x += cnt;
            }
#endif
            for (; i < n; ++i, ++x) {
                unsigned int color;
                int dv = zfix >> 12;
                if (dv < 0) dv = 0;
                if (dv > 65535) dv = 65535;
                zfix += dzfix;
                if (depth_row != NULL && ngl_sw.depth_func != GL_ALWAYS) {
                    unsigned short old = depth_row[x];
                    if (depth_less ? (unsigned int)dv >= old : (unsigned int)dv > old) {
                        su += dsu; sv += dsv;
                        if (smooth) { ci[0] += dci[0]; ci[1] += dci[1]; ci[2] += dci[2]; ci[3] += dci[3]; }
                        continue;
                    }
                }
                if (smooth) {
                    int r = ci[0] >> 16, g = ci[1] >> 16, bl = ci[2] >> 16, al = ci[3] >> 16;
                    ci[0] += dci[0]; ci[1] += dci[1]; ci[2] += dci[2]; ci[3] += dci[3];
                    color = ngl_sw_pack((unsigned int)(r < 0 ? 0 : r), (unsigned int)(g < 0 ? 0 : g),
                                        (unsigned int)(bl < 0 ? 0 : bl), (unsigned int)(al < 0 ? 0 : al));
                } else {
                    color = flat_color;
                }
                if (tex != NULL) {
                    int tx = su >> 16, ty = sv >> 16;
                    unsigned int texel;
                    su += dsu;
                    sv += dsv;
                    if (repeat_s) tx &= wmask;
                    else if (tx < 0) tx = 0;
                    else if (tx > wmask) tx = wmask;
                    if (repeat_t) ty &= hmask;
                    else if (ty < 0) ty = 0;
                    else if (ty > hmask) ty = hmask;
                    texel = tex->argb[ty * tw + tx];
                    if (mode == GL_REPLACE) {
                        color = (texel & 0x00ffffffU) |
                                (tex->no_alpha ? (color & 0xff000000U) : (texel & 0xff000000U));
                    } else if (color != 0xffffffffU) {
                        color = ngl_sw_modulate(color, texel);
                    } else {
                        color = texel;   /* modulate by white */
                    }
                }
                if (ngl_sw.alpha_test &&
                        !ngl_sw_compare(ngl_sw.alpha_func,
                                        (float)((color >> 24) & 0xffU) / 255.0f,
                                        ngl_sw.alpha_ref)) {
                    continue;
                }
                if (depth_write) depth_row[x] = (unsigned short)dv;
                if (blend_mode == 1) {
                    /* same result as ngl_sw_blend(); alpha 0 leaves the
                     * pixel as it is (565 -> 8888 -> 565 round-trips) */
                    unsigned int al = color >> 24;
                    if (al == 0) continue;
                    if (al != 255U) {
                        color = ngl_sw_lerp8(ngl_sw_rgb565_to_argb(dst_row[x]),
                                             color, al + (al >> 7));
                    }
                } else if (blend_mode == 2) {
                    color = ngl_sw_blend(color, ngl_sw_rgb565_to_argb(dst_row[x]));
                }
                dst_row[x] = ngl_sw_rgb565(color);
            }
            u0 = u1;
            v0 = v1;
            if (smooth) {
                for (ch = 0; ch < 4; ++ch) c0[ch] = c1[ch];
            }
        }
    }
    return 1;
}

static void ngl_sw_raster_tri_fast(const NGLVertex *a, const NGLVertex *b,
                                   const NGLVertex *c) {
    float aw, bw, cw;
    float fax, fay, fbx, fby, fcx, fcy;
    float maxabs;
    int shift, sub, half;
    int ax, ay, bx, by, cx, cy;
    int area;
    float inv_area;
    float depth_offset = 0.0f;
    int minx, maxx, miny, maxy;
    int scx0, scy0, scx1, scy1;
    int a0, b0, a1, b1, a2, b2;   /* per-pixel x/y steps for e0, e1, e2 */
    int bias0, bias1, bias2;
    int row0, row1, row2;
    int x, y;
    int unit;
    NGLTexture *tex_unit[NGL_SW_TEXTURE_UNITS];
    int tex_nearest[NGL_SW_TEXTURE_UNITS];
    int tex_mips[NGL_SW_TEXTURE_UNITS];
    int tex_min_nearest[NGL_SW_TEXTURE_UNITS];
    float dux[NGL_SW_TEXTURE_UNITS], duy[NGL_SW_TEXTURE_UNITS];
    float dvx[NGL_SW_TEXTURE_UNITS], dvy[NGL_SW_TEXTURE_UNITS];
    float dqx = 0.0f, dqy = 0.0f;
    int any_tex = 0;
    int perspective;
    int smooth;
    int simple_write;
    int rgb_mask_all;
    float zscale, zbase;
    NGLPlane pz, pinvw, pfog;
    NGLPlane pu[NGL_SW_TEXTURE_UNITS], pv[NGL_SW_TEXTURE_UNITS];
    NGLPlane pcol[4];
    unsigned int flat_color;

    if (ngl_sw.pixels == NULL || ngl_sw.width <= 0 || ngl_sw.height <= 0) {
        return;
    }
    if (!(a->w > 0.0f) || !(b->w > 0.0f) || !(c->w > 0.0f)) {
        return;
    }
    aw = 1.0f / a->w;
    bw = 1.0f / b->w;
    cw = 1.0f / c->w;
    fax = (a->x * aw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    fay = (a->y * aw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];
    fbx = (b->x * bw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    fby = (b->y * bw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];
    fcx = (c->x * cw * 0.5f + 0.5f) * (float)ngl_sw.viewport[2] + (float)ngl_sw.viewport[0];
    fcy = (c->y * cw * 0.5f + 0.5f) * (float)ngl_sw.viewport[3] + (float)ngl_sw.viewport[1];

    /* Pick the finest subpixel grid whose edge values fit in 32 bits:
     * |E| <= 2 * (2 * M * sub)^2 < 2^31  =>  M * sub < 16384. */
    /* inline max (fmaxf is a libm call here); NaN coordinates after the
     * first are ignored, as with fmaxf */
    maxabs = fabsf(fax);
    if (fabsf(fay) > maxabs) maxabs = fabsf(fay);
    if (fabsf(fbx) > maxabs) maxabs = fabsf(fbx);
    if (fabsf(fby) > maxabs) maxabs = fabsf(fby);
    if (fabsf(fcx) > maxabs) maxabs = fabsf(fcx);
    if (fabsf(fcy) > maxabs) maxabs = fabsf(fcy);
    if (maxabs * 16.0f < 16000.0f) shift = 4;
    else if (maxabs * 4.0f < 16000.0f) shift = 2;
    else {
        ngl_sw_raster_tri_ref(*a, *b, *c);
        return;
    }
    sub = 1 << shift;
    half = sub >> 1;
    ax = ngl_sw_floor_int(fax * (float)sub + 0.5f);
    ay = ngl_sw_floor_int(fay * (float)sub + 0.5f);
    bx = ngl_sw_floor_int(fbx * (float)sub + 0.5f);
    by = ngl_sw_floor_int(fby * (float)sub + 0.5f);
    cx = ngl_sw_floor_int(fcx * (float)sub + 0.5f);
    cy = ngl_sw_floor_int(fcy * (float)sub + 0.5f);

    /* Same orientation convention as ngl_sw_edge(a, b, c). */
    area = (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    if (area == 0) {
        return;
    }
    if (ngl_sw.cull_face) {
        /* area is the negated cross product: counter-clockwise < 0. Using
         * > 0 here culled every front face (all Sprite3Ds vanished, meshes
         * showed their back faces). */
        int front = (area < 0) == (ngl_sw.front_face == GL_CCW);
        if ((front && ngl_sw.cull_mode == GL_FRONT) ||
            (!front && ngl_sw.cull_mode == GL_BACK) ||
            ngl_sw.cull_mode == GL_FRONT_AND_BACK) {
            return;
        }
    }
    if (area < 0) {
        const NGLVertex *tv = b; b = c; c = tv;
        { int t = bx; bx = cx; cx = t; }
        { int t = by; by = cy; cy = t; }
        { float t = bw; bw = cw; cw = t; }
        { float t = fbx; fbx = fcx; fcx = t; }
        { float t = fby; fby = fcy; fcy = t; }
        area = -area;
    }
    inv_area = 1.0f / (float)area;

    /* Pixel bounds, then scissor and target: reject off-screen
     * triangles before any attribute or texture setup. */
    minx = (int)(((ax < bx ? (ax < cx ? ax : cx) : (bx < cx ? bx : cx))) >> shift);
    maxx = (int)(((ax > bx ? (ax > cx ? ax : cx) : (bx > cx ? bx : cx))) >> shift) + 1;
    miny = (int)(((ay < by ? (ay < cy ? ay : cy) : (by < cy ? by : cy))) >> shift);
    maxy = (int)(((ay > by ? (ay > cy ? ay : cy) : (by > cy ? by : cy))) >> shift) + 1;
    scx0 = ngl_sw.scissor_test ? ngl_sw.scissor[0] : 0;
    scy0 = ngl_sw.scissor_test ? ngl_sw.scissor[1] : 0;
    scx1 = scx0 + (ngl_sw.scissor_test ? ngl_sw.scissor[2] : ngl_sw.width);
    scy1 = scy0 + (ngl_sw.scissor_test ? ngl_sw.scissor[3] : ngl_sw.height);
    if (scx0 < 0) scx0 = 0;
    if (scy0 < 0) scy0 = 0;
    if (scx1 > ngl_sw.width) scx1 = ngl_sw.width;
    if (scy1 > ngl_sw.height) scy1 = ngl_sw.height;
    if (minx < scx0) minx = scx0;
    if (maxx > scx1) maxx = scx1;
    if (miny < scy0) miny = scy0;
    if (maxy > scy1) maxy = scy1;
    if (minx >= maxx || miny >= maxy) {
        return;
    }

    /* Depth in final 16-bit units, linear in screen space. */
    {
        float az = a->z * aw * 0.5f + 0.5f;
        float bz = b->z * bw * 0.5f + 0.5f;
        float cz = c->z * cw * 0.5f + 0.5f;
        if (ngl_sw.polygon_offset_fill) {
            float span_x = fmaxf(fax, fmaxf(fbx, fcx)) - fminf(fax, fminf(fbx, fcx));
            float span_y = fmaxf(fay, fmaxf(fby, fcy)) - fminf(fay, fminf(fby, fcy));
            float span = fmaxf(1.0f, fmaxf(span_x, span_y));
            float slope = fmaxf(fabsf(bz - az), fabsf(cz - az)) / span;
            depth_offset = ngl_sw.polygon_offset_factor * slope +
                           ngl_sw.polygon_offset_units / 65535.0f;
        }
        zscale = (ngl_sw.depth_far - ngl_sw.depth_near) * 65535.0f;
        zbase = (ngl_sw.depth_near + depth_offset) * 65535.0f;
        ngl_sw_plane(&pz, zbase + az * zscale, zbase + bz * zscale,
                     zbase + cz * zscale);
    }

    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        NGLTexture *t = ngl_sw.texture_2d[unit] ? ngl_sw_bound_texture(unit) : NULL;
        tex_unit[unit] = (t != NULL && t->argb != NULL &&
                          t->width > 0 && t->height > 0) ? t : NULL;
        tex_nearest[unit] = tex_unit[unit] != NULL &&
                            ngl_sw_texture_is_nearest(tex_unit[unit]);
        tex_mips[unit] = 0;
        tex_min_nearest[unit] = 1;
        if (tex_unit[unit] != NULL) {
            NGLTexture *t = tex_unit[unit];
            any_tex = 1;
            if (ngl_sw_min_filter_is_mipmapped(t->min_filter)) {
                int i;
                tex_mips[unit] = ngl_sw_texture_build_mips(t);
                for (i = 0; i < tex_mips[unit]; ++i) {
                    t->mips[i].wrap_s = t->wrap_s;
                    t->mips[i].wrap_t = t->wrap_t;
                }
                tex_min_nearest[unit] = t->min_filter == GL_NEAREST_MIPMAP_NEAREST ||
                                        t->min_filter == GL_NEAREST_MIPMAP_LINEAR;
            }
        }
    }
    smooth = ngl_sw.shade_model == GL_SMOOTH &&
             !(a->color == b->color && b->color == c->color);
    flat_color = c->color;
    /* Constant w (2D overlays, backgrounds, orthographic) needs no
     * per-pixel division at all. */
    perspective = (any_tex || smooth || ngl_sw.fog) &&
                  !(aw == bw && bw == cw);

    if (perspective) {
        ngl_sw_plane(&pinvw, aw, bw, cw);
    }
    {
        float ka = perspective ? aw : 1.0f;
        float kb = perspective ? bw : 1.0f;
        float kc = perspective ? cw : 1.0f;
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            if (tex_unit[unit] == NULL) continue;
            ngl_sw_plane(&pu[unit], a->u[unit] * ka, b->u[unit] * kb, c->u[unit] * kc);
            ngl_sw_plane(&pv[unit], a->v[unit] * ka, b->v[unit] * kb, c->v[unit] * kc);
        }
        if (ngl_sw.fog) {
            ngl_sw_plane(&pfog, a->fog * ka, b->fog * kb, c->fog * kc);
        }
        if (smooth) {
            int ch;
            for (ch = 0; ch < 4; ++ch) {
                int sh = ch == 3 ? 24 : 16 - ch * 8;
                ngl_sw_plane(&pcol[ch],
                             (float)((a->color >> sh) & 0xffU) * ka,
                             (float)((b->color >> sh) & 0xffU) * kb,
                             (float)((c->color >> sh) & 0xffU) * kc);
            }
        }
    }
    rgb_mask_all = ngl_sw.color_mask[0] && ngl_sw.color_mask[1] &&
                   ngl_sw.color_mask[2];
    simple_write = !ngl_sw.blend && !ngl_sw.fog &&
                   ngl_sw.color_mask[0] && ngl_sw.color_mask[1] &&
                   ngl_sw.color_mask[2];


    /* e0 = edge(b, c, p) weights a; e1 = edge(c, a, p) weights b;
     * e2 = edge(a, b, p) weights c. E(p) = (px-Px)(Qy-Py) - (py-Py)(Qx-Px). */
    a0 = (cy - by) << shift;  b0 = -((cx - bx) << shift);
    a1 = (ay - cy) << shift;  b1 = -((ax - cx) << shift);
    a2 = (by - ay) << shift;  b2 = -((bx - ax) << shift);
    bias0 = ngl_sw_top_left((float)bx, (float)by, (float)cx, (float)cy) ? 0 : -1;
    bias1 = ngl_sw_top_left((float)cx, (float)cy, (float)ax, (float)ay) ? 0 : -1;
    bias2 = ngl_sw_top_left((float)ax, (float)ay, (float)bx, (float)by) ? 0 : -1;
    {
        int px = (minx << shift) + half;
        int py = (miny << shift) + half;
        row0 = (px - bx) * (cy - by) - (py - by) * (cx - bx) + bias0;
        row1 = (px - cx) * (ay - cy) - (py - cy) * (ax - cx) + bias1;
        row2 = (px - ax) * (by - ay) - (py - ay) * (bx - ax) + bias2;
    }

    /* Screen-space derivatives of the (perspective-divided) texture
     * planes, for per-pixel mip level selection. */
    {
        float l1x = (float)a1 * inv_area, l2x = (float)a2 * inv_area;
        float l1y = (float)b1 * inv_area, l2y = (float)b2 * inv_area;
        if (perspective) {
            dqx = pinvw.db * l1x + pinvw.dc * l2x;
            dqy = pinvw.db * l1y + pinvw.dc * l2y;
        }
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            if (!tex_mips[unit]) continue;
            dux[unit] = pu[unit].db * l1x + pu[unit].dc * l2x;
            duy[unit] = pu[unit].db * l1y + pu[unit].dc * l2y;
            dvx[unit] = pv[unit].db * l1x + pv[unit].dc * l2x;
            dvy[unit] = pv[unit].db * l1y + pv[unit].dc * l2y;
        }
    }
    if (ngl_sw_span_fast(tex_unit, tex_nearest, tex_mips, perspective, smooth,
                         rgb_mask_all, flat_color, inv_area, &pz, &pinvw,
                         pu, pv, pcol, minx, maxx, miny, maxy,
                         row0, row1, row2, a0, a1, a2, b0, b1, b2,
                         bias1, bias2)) {
        return;
    }
    for (y = miny; y < maxy; ++y, row0 += b0, row1 += b1, row2 += b2) {
        int e0 = row0, e1 = row1, e2 = row2;
        int entered = 0;
        unsigned short *dst_row = ngl_sw.pixels +
                                  (ngl_sw.height - 1 - y) * ngl_sw.stride;
        unsigned short *depth_row = ngl_sw.depth != NULL ?
                                    ngl_sw.depth +
                                    (ngl_sw.height - 1 - y) * ngl_sw.stride : NULL;
        for (x = minx; x < maxx; ++x, e0 += a0, e1 += a1, e2 += a2) {
            float l1, l2, zf, rw;
            unsigned short depth_value;
            unsigned int color;
            if ((e0 | e1 | e2) < 0) {
                if (entered) break;  /* triangles are convex */
                continue;
            }
            entered = 1;
            /* Undo the top-left bias for the barycentric weights. */
            l1 = (float)(e1 - bias1) * inv_area;
            l2 = (float)(e2 - bias2) * inv_area;
            zf = NGL_SW_PLANE(pz, l1, l2);
            if (zf < 0.0f) zf = 0.0f;
            if (zf > 65535.0f) zf = 65535.0f;
            depth_value = (unsigned short)(zf + 0.5f);
            if (ngl_sw.depth_test && depth_row != NULL &&
                    !ngl_sw_depth_compare(ngl_sw.depth_func, depth_value,
                                          depth_row[x])) {
                continue;
            }
            rw = perspective ? 1.0f / NGL_SW_PLANE(pinvw, l1, l2) : 1.0f;
            if (smooth) {
                float r = NGL_SW_PLANE(pcol[0], l1, l2) * rw;
                float g = NGL_SW_PLANE(pcol[1], l1, l2) * rw;
                float bl = NGL_SW_PLANE(pcol[2], l1, l2) * rw;
                float al = NGL_SW_PLANE(pcol[3], l1, l2) * rw;
                color = ngl_sw_pack((unsigned int)(r > 0.0f ? r + 0.5f : 0.0f),
                                    (unsigned int)(g > 0.0f ? g + 0.5f : 0.0f),
                                    (unsigned int)(bl > 0.0f ? bl + 0.5f : 0.0f),
                                    (unsigned int)(al > 0.0f ? al + 0.5f : 0.0f));
            } else {
                color = flat_color;
            }
            if (any_tex) {
                for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
                    NGLTexture *tex = tex_unit[unit];
                    float u, v;
                    unsigned int texel;
                    if (tex == NULL) continue;
                    u = NGL_SW_PLANE(pu[unit], l1, l2) * rw;
                    v = NGL_SW_PLANE(pv[unit], l1, l2) * rw;
                    if (tex_mips[unit]) {
                        /* d(U/Q) = (dU - u dQ) / Q; scaled to texels. */
                        float ux = (dux[unit] - u * dqx) * rw * (float)tex->width;
                        float uy = (duy[unit] - u * dqy) * rw * (float)tex->width;
                        float vx = (dvx[unit] - v * dqx) * rw * (float)tex->height;
                        float vy = (dvy[unit] - v * dqy) * rw * (float)tex->height;
                        float rx = ux * ux + vx * vx;
                        float ry = uy * uy + vy * vy;
                        float rho2 = rx > ry ? rx : ry;
                        if (rho2 > 1.0f) {
                            int level = ngl_sw_mip_level(rho2);
                            const NGLTexture *lt;
                            if (level > tex_mips[unit]) level = tex_mips[unit];
                            lt = level > 0 ? &tex->mips[level - 1] : tex;
                            texel = tex_min_nearest[unit] ?
                                    ngl_sw_texel_nearest(lt, u, v) :
                                    ngl_sw_texel_bilinear(lt, u, v);
                        } else {
                            texel = tex->mag_filter == GL_LINEAR ?
                                    ngl_sw_texel_bilinear(tex, u, v) :
                                    ngl_sw_texel_nearest(tex, u, v);
                        }
                    } else {
                        texel = tex_nearest[unit] ? ngl_sw_texel_nearest(tex, u, v) :
                                                    ngl_sw_texel(tex, u, v);
                    }
                    color = ngl_sw_apply_tex_env(unit, tex, color, texel);
                }
            }
            if (ngl_sw.alpha_test &&
                    !ngl_sw_compare(ngl_sw.alpha_func,
                                    (float)((color >> 24) & 0xffU) / 255.0f,
                                    ngl_sw.alpha_ref)) {
                continue;
            }
            if (ngl_sw.depth_test && depth_row != NULL && ngl_sw.depth_mask) {
                depth_row[x] = depth_value;
            }
            if (simple_write) {
                dst_row[x] = ngl_sw_rgb565(color);
            } else {
                unsigned int dst;
                if (ngl_sw.fog) {
                    color = ngl_sw_apply_fog(color, NGL_SW_PLANE(pfog, l1, l2) * rw);
                }
                dst = ngl_sw_rgb565_to_argb(dst_row[x]);
                color = ngl_sw_blend(color, dst);
                if (!rgb_mask_all) color = ngl_sw_apply_color_mask(color, dst);
                dst_row[x] = ngl_sw_rgb565(color);
            }
        }
    }
}

static void ngl_sw_raster_tri(const NGLVertex *a, const NGLVertex *b,
                              const NGLVertex *c) {
    ngl_sw_raster_tri_fast(a, b, c);
}

/* Bit n set when the vertex is outside clip plane n (same planes and
 * epsilon as ngl_sw_clip_distance). */
static unsigned int ngl_sw_outcode(const NGLVertex *v) {
    unsigned int code = 0;
    if (v->w - 0.0001f < 0.0f) code |= 1U;
    if (v->x + v->w < 0.0f) code |= 2U;
    if (v->w - v->x < 0.0f) code |= 4U;
    if (v->y + v->w < 0.0f) code |= 8U;
    if (v->w - v->y < 0.0f) code |= 16U;
    if (v->z + v->w * 1.0002f < 0.0f) code |= 32U;
    if (v->w - v->z < 0.0f) code |= 64U;
    return code;
}

static void ngl_sw_draw_tri(const NGLVertex *a, const NGLVertex *b,
                            const NGLVertex *c) {
    NGLVertex first[12];
    NGLVertex second[12];
    NGLVertex *input = first;
    NGLVertex *output = second;
    NGLVertex *swap;
    int count = 3;
    int plane;
    int i;
    unsigned int clip_mask = 0x7fU;
    {
        unsigned int oa = a->outcode;
        unsigned int ob = b->outcode;
        unsigned int oc = c->outcode;
        if ((oa & ob & oc) != 0) {
            return;  /* entirely outside one plane */
        }
        if ((oa | ob | oc) == 0) {
            /* entirely inside: no clipping */
            if (ngl_sw.shade_model != GL_SMOOTH &&
                    (a->color != c->color || b->color != c->color)) {
                NGLVertex fa = *a, fb = *b;
                fa.color = fb.color = c->color;
                ngl_sw_raster_tri(&fa, &fb, c);
            } else {
                ngl_sw_raster_tri(a, b, c);
            }
            return;
        }
        clip_mask = oa | ob | oc;
    }
    first[0] = *a;
    first[1] = *b;
    first[2] = *c;
    for (plane = 0; plane < 7 && count >= 3; ++plane) {
        if ((clip_mask & (1U << plane)) == 0) {
            continue;  /* no vertex crosses this plane */
        }
        count = ngl_sw_clip_plane(input, count, output, plane);
        swap = input;
        input = output;
        output = swap;
    }
    if (ngl_sw.shade_model != GL_SMOOTH) {
        /* Flat shading uses the provoking (last) vertex of the original
         * triangle for every piece produced by clipping. */
        for (i = 0; i < count; ++i) input[i].color = c->color;
    }
    for (i = 1; i + 1 < count; ++i) {
        ngl_sw_raster_tri(&input[0], &input[i], &input[i + 1]);
    }
}

/*
 * Strips reference every vertex ~3 times; transforming and lighting it once
 * per draw instead of once per triangle is the main geometry saving. The
 * transform depends only on state that is fixed for the duration of a draw.
 */
static void ngl_sw_vcache_begin(void) {
    if (++ngl_sw.vcache_draw == 0) {
        if (ngl_sw.vcache_stamp != NULL) {
            memset(ngl_sw.vcache_stamp, 0,
                   (size_t)ngl_sw.vcache_cap * sizeof(unsigned int));
        }
        ngl_sw.vcache_draw = 1;
    }
}

static int ngl_sw_vcache_reserve(int index) {
    int cap;
    void *verts;
    unsigned int *stamps;
    if (index < ngl_sw.vcache_cap) return 1;
    cap = ngl_sw.vcache_cap > 0 ? ngl_sw.vcache_cap : 256;
    while (cap <= index) cap *= 2;
    verts = realloc(ngl_sw.vcache, (size_t)cap * sizeof(NGLVertex));
    if (verts == NULL) return 0;
    ngl_sw.vcache = verts;
    stamps = (unsigned int *)realloc(ngl_sw.vcache_stamp,
                                     (size_t)cap * sizeof(unsigned int));
    if (stamps == NULL) return 0;
    memset(stamps + ngl_sw.vcache_cap, 0,
           (size_t)(cap - ngl_sw.vcache_cap) * sizeof(unsigned int));
    ngl_sw.vcache_stamp = stamps;
    ngl_sw.vcache_cap = cap;
    return 1;
}

/* The cache is reserved for the whole draw up front (see draw_indexed), so
 * the returned pointer stays valid until the draw ends. */
static const NGLVertex *ngl_sw_fetch_vertex(int index, int cached,
                                            NGLVertex *scratch) {
    NGLVertex *cache;
    if (!cached || index < 0) {
        ngl_sw_transform_vertex(index, scratch);
        return scratch;
    }
    cache = (NGLVertex *)ngl_sw.vcache;
    if (ngl_sw.vcache_stamp[index] != ngl_sw.vcache_draw) {
        ngl_sw_transform_vertex(index, &cache[index]);
        ngl_sw.vcache_stamp[index] = ngl_sw.vcache_draw;
    }
    return &cache[index];
}

static int ngl_sw_index(GLenum type, const GLvoid *indices, int i) {
    if (type == GL_UNSIGNED_BYTE) return ((const unsigned char *)indices)[i];
    if (type == GL_UNSIGNED_SHORT) return ((const unsigned short *)indices)[i];
    return ((const unsigned int *)indices)[i];
}

static void ngl_sw_draw_indexed(GLenum mode, GLsizei count, GLenum type,
                                const GLvoid *indices, GLint first) {
    int i;
    int max_index;
    int cached;
    NGLVertex sa, sb, sc;
    (void)mode;
    if (!ngl_sw.vertex.enabled || ngl_sw.vertex.ptr == NULL || count < 3) return;
    if (indices != NULL) {
        max_index = 0;
        for (i = 0; i < count; ++i) {
            int k = ngl_sw_index(type, indices, i);
            if (k > max_index) max_index = k;
        }
    } else {
        max_index = first + count - 1;
    }
    cached = max_index >= 0 && ngl_sw_vcache_reserve(max_index);
    ngl_sw_vcache_begin();
    ngl_sw_xform_begin();
    ngl_sw_light_begin();
    for (i = 0; i + 2 < count; ++i) {
        int ia = indices != NULL ? ngl_sw_index(type, indices, i) : first + i;
        int ib = indices != NULL ? ngl_sw_index(type, indices, i + 1) : first + i + 1;
        int ic = indices != NULL ? ngl_sw_index(type, indices, i + 2) : first + i + 2;
        if ((i & 1) != 0) {
            int t = ia; ia = ib; ib = t;
        }
        /* strip stitching produces degenerate triangles: nothing to draw */
        if (ia == ib || ib == ic || ia == ic) {
            continue;
        }
        ngl_sw_draw_tri(ngl_sw_fetch_vertex(ia, cached, &sa),
                        ngl_sw_fetch_vertex(ib, cached, &sb),
                        ngl_sw_fetch_vertex(ic, cached, &sc));
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
        ngl_sw.depth = (unsigned short *)malloc((size_t)need * sizeof(unsigned short));
        ngl_sw.depth_count = ngl_sw.depth != NULL ? need : 0;
    }
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
    else if (pname == GL_SPOT_CUTOFF) {
        l->spot_cutoff = param;
        l->spot_cos_cutoff = cosf(param * 3.14159265358979323846f / 180.0f);
    }
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
static inline void glLoadIdentity(void) {
    ngl_sw_identity(ngl_sw_matrix());
}
static inline void glLoadMatrixf(const GLfloat *m) { memcpy(ngl_sw_matrix(), m, 16 * sizeof(float)); }
static inline void glMultMatrixf(const GLfloat *m) {
    ngl_sw_mul(ngl_sw_matrix(), ngl_sw_matrix(), m);
}
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
    float m[16];
    ngl_sw_identity(m); m[0] = x; m[5] = y; m[10] = z; glMultMatrixf(m);
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
    t = ngl_sw_bound_texture(unit);
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
static inline void glBindTexture(GLenum target, GLuint texture) { int unit = ngl_sw_texture_unit(ngl_sw.active_texture); (void)target; if (unit >= 0) { ngl_sw.bound_texture[unit] = texture; ngl_sw.bound_ptr[unit] = NULL; ngl_sw_bound_texture(unit); } }
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
    ngl_sw_texture_set_components(ngl_sw_bound_texture(unit), internalformat);
    if (imageSize < palette_bytes + width * height) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    t = ngl_sw_bound_texture(unit);
    ngl_sw_texture_drop_mips(t);
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
    (void)border;
    if (unit < 0 || width < 0 || height < 0) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    t = ngl_sw_bound_texture(unit);
    ngl_sw_texture_drop_mips(t);
    free(t->argb);
    t->argb = NULL;
    t->width = width;
    t->height = height;
    ngl_sw_texture_set_components(t, internalformat);
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
    t = ngl_sw_bound_texture(unit);
    if (t == NULL || t->argb == NULL || xoffset + width > t->width ||
        yoffset + height > t->height ||
        format == GL_PALETTE8_RGB8_OES ||
        format == GL_PALETTE8_RGBA8_OES ||
        format == GL_M3G_PALETTE8_RGB8_32) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    ngl_sw_texture_drop_mips(t);
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
    t = ngl_sw_bound_texture(unit);
    ngl_sw_texture_drop_mips(t);
    free(t->argb);
    t->argb = NULL;
    t->width = width;
    t->height = height;
    ngl_sw_texture_set_components(t, format);
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
    if (ngl_sw.color_mask[0] && ngl_sw.color_mask[1] && ngl_sw.color_mask[2]) {
        unsigned short c565 = ngl_sw_rgb565(ngl_sw.clear_color);
        unsigned short d16 = (unsigned short)(
            ngl_sw_clampf(ngl_sw.clear_depth, 0.0f, 1.0f) * 65535.0f + 0.5f);
        int do_color = (mask & GL_COLOR_BUFFER_BIT) && ngl_sw.pixels != NULL;
        int do_depth = (mask & GL_DEPTH_BUFFER_BIT) && ngl_sw.depth != NULL;
        for (y = y0; y < y1; ++y) {
            int row = (ngl_sw.height - 1 - y) * ngl_sw.stride;
            if (do_color) {
                unsigned short *p = ngl_sw.pixels + row;
                for (x = x0; x < x1; ++x) p[x] = c565;
            }
            if (do_depth) {
                unsigned short *p = ngl_sw.depth + row;
                for (x = x0; x < x1; ++x) p[x] = d16;
            }
        }
        return;
    }
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
                ngl_sw.depth[offset] = (unsigned short)(
                    ngl_sw_clampf(ngl_sw.clear_depth, 0.0f, 1.0f) * 65535.0f + 0.5f);
            }
        }
    }
}
static inline void glDeleteTextures(GLsizei n, const GLuint *textures) {
    GLsizei i;
    int slot;
    int unit;
    if (textures == NULL) return;
    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        ngl_sw.bound_ptr[unit] = NULL;
    }
    for (i = 0; i < n; ++i) {
        if (textures[i] == 0) continue;
        for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
            if (ngl_sw.bound_texture[unit] == textures[i]) {
                ngl_sw.bound_texture[unit] = 0;
            }
        }
        for (slot = 1; slot < NGL_SW_MAX_TEXTURES; ++slot) {
            if (ngl_sw.textures[slot].id == textures[i]) {
                ngl_sw_texture_drop_mips(&ngl_sw.textures[slot]);
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
        ngl_sw.lights[light].spot_cos_cutoff = -1.0f;
        ngl_sw.lights[light].constant_attenuation = 1.0f;
        ngl_sw.lights[light].ambient[3] = 1.0f;
        ngl_sw.lights[light].diffuse[3] = 1.0f;
        ngl_sw.lights[light].specular[3] = 1.0f;
    }
    ngl_sw_identity(ngl_sw.modelview);
    ngl_sw_identity(ngl_sw.projection);
    for (unit = 0; unit < NGL_SW_TEXTURE_UNITS; ++unit) {
        ngl_sw.textures[unit].wrap_s = GL_CLAMP_TO_EDGE;
        ngl_sw.textures[unit].wrap_t = GL_CLAMP_TO_EDGE;
        ngl_sw.textures[unit].min_filter = GL_NEAREST;
        ngl_sw.textures[unit].mag_filter = GL_NEAREST;
        ngl_sw.tex_env_mode[unit] = GL_MODULATE;
        ngl_sw.tex_env_color[unit] = 0xff000000U;
        ngl_sw_identity(ngl_sw.texture_matrix[unit]);
    }
    return 1;
}
static inline void nglExit(void) {
    int i;
    for (i = 0; i < NGL_SW_MAX_TEXTURES; ++i) {
        ngl_sw_texture_drop_mips(&ngl_sw.textures[i]);
        free(ngl_sw.textures[i].argb);
    }
    free(ngl_sw.depth);
    free(ngl_sw.vcache);
    free(ngl_sw.vcache_stamp);
    memset(&ngl_sw, 0, sizeof(ngl_sw));
}
static inline void nglInitTextures(GLsizei count, const GLuint *textures) { GLsizei i; for (i = 0; i < count; ++i) ngl_sw_texture(textures[i]); }
static inline void nglBindTextureInternal(GLenum target, GLuint texture) { glBindTexture(target, texture); }

#endif
