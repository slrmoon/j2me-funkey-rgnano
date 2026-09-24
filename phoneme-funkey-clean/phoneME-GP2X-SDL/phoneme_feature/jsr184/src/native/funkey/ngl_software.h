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
#include <sys/stat.h>

#define NGL_SW_MAX_TEXTURES 256
#define NGL_SW_STACK_DEPTH 16
#define NGL_SW_TEXTURE_UNITS 2

typedef struct {
    int valid;
    int index[3];
    float object[3][4];
    float clip[3][4];
    float screen[3][2];
    float uv[3][2];
    float signed_area;
    int front_face;
    GLenum cull_mode;
} NGLImmediateTriangleSample;

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
    int frame;
    int draw_call;
    unsigned long vb, ib, app, texture, image;
    int vertex_count, index_count;
    int input_triangles, raster_calls, clip_reject[7];
    int w_reject, area_zero, culled, depth_reject, alpha_reject;
    int bounds_valid, min_x, max_x, min_y, max_y;
    float w_min, w_max;
    int pixels;
    int cull_override;
    int highlight;
    NGLImmediateTriangleSample first_nonzero;
    NGLImmediateTriangleSample first_culled;
    NGLImmediateTriangleSample first_passed;
} NGLImmediateRecord;

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
    GLenum tex_env_mode[NGL_SW_TEXTURE_UNITS];
    unsigned int tex_env_color[NGL_SW_TEXTURE_UNITS];
    int trace_draw_budget;
    int trace_upload_budget;
    int trace_current_draw;
    int trace_scene_draw;
    int trace_triangles;
    int trace_culled;
    int trace_pixels;
    int trace_scene_input_triangles;
    int trace_scene_clip_reject[7];
    int trace_scene_raster_calls;
    int trace_scene_w_reject;
    int trace_scene_area_zero;
    int trace_scene_matrix_pending;
    int trace_scene_matrix_active;
    int trace_scene_matrix_stage;
    int trace_scene_matrix_done;
    float trace_scene_modelview_identity[16];
    int trace_immediate_draw;
    unsigned long immediate_vb;
    unsigned long immediate_ib;
    unsigned long immediate_app;
    unsigned long immediate_texture;
    unsigned long immediate_image;
    int immediate_vertex_count;
    int immediate_index_count;
    int immediate_frame;
    int immediate_draw_call;
    int immediate_defer;
    int immediate_record_count;
    NGLImmediateRecord immediate_records[32];
    int immediate_min_x;
    int immediate_max_x;
    int immediate_min_y;
    int immediate_max_y;
    int immediate_depth_reject;
    int immediate_alpha_reject;
    int immediate_clip_reject[7];
    int immediate_input_triangles;
    int immediate_raster_calls;
    int immediate_w_reject;
    int immediate_area_zero;
    int immediate_bounds_valid;
    float immediate_w_min;
    float immediate_w_max;
    NGLImmediateTriangleSample immediate_first_nonzero;
    NGLImmediateTriangleSample immediate_first_culled;
    NGLImmediateTriangleSample immediate_first_passed;
    int immediate_disable_cull;
    int immediate_highlight;
    float immediate_transform[16];
    int immediate_transform_valid;
    int rally_geometry_trace_count;
    int trace_frame_matrices_pending;
    int trace_frame_matrix_frame;
    int diag_enabled;
    int diag_inited;
    int diag_frame;
    int diag_capture_frame;
    int diag_capture_active;
    int diag_done;
    int diag_draw_index;
    int diag_draw_limit;
    int diag_skipped_draws;
    int diag_source;
    int diag_mode;
    int diag_depth_reject;
    int diag_alpha_reject;
    int diag_pixels_changed;
    int diag_texture_dumped[NGL_SW_MAX_TEXTURES];
    char diag_dir[192];
    GLenum error;
} NGLContext;

extern NGLContext ngl_sw;

static int ngl_sw_texture_unit(GLenum texture);

static int ngl_sw_trace_enabled(void) {
    const char *value = getenv("M3G_NGL_TRACE");
    if (value != NULL) {
        return value[0] != '\0' && value[0] != '0';
    }
    if (value == NULL) {
        value = getenv("M3G_NGL_PROBE_TRACE");
    }
    if (value != NULL) {
        return value[0] != '\0' && value[0] != '0';
    }
    return 1;
}

static int ngl_sw_verbose_trace_enabled(void) {
	const char *value = getenv("M3G_TRACE_VERBOSE");
    return value != NULL && value[0] != '\0' && value[0] != '0';
}

static int ngl_sw_rally_trace_enabled(void) {
    const char *value = getenv("M3G_RALLY_TRACE");
    return value != NULL && value[0] != '\0' && value[0] != '0';
}

static int ngl_sw_rally_reflect_stage(void) {
    const char *value = getenv("M3G_RALLY_REFLECT_X");
    if (!ngl_sw_rally_trace_enabled() || value == NULL) return 0;
    if (strcmp(value, "model") == 0) return 1;
    if (strcmp(value, "view") == 0) return 2;
    if (strcmp(value, "projection") == 0) return 3;
    return 0;
}

static int ngl_sw_diag_flip_front_enabled(void) {
    const char *value = getenv("M3G_VIS_FLIP_FRONT");
    return value != NULL && value[0] != '\0' && value[0] != '0';
}

static void ngl_sw_identity(float *m) {
    int i;
    for (i = 0; i < 16; ++i) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void ngl_sw_trace_matrix(const char *stage, const float *m) {
    fprintf(stderr,
            "[M3G SCENE MATRIX %s] "
            "%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
            stage,
            m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
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

static int ngl_sw_diag_env_enabled(void) {
    const char *value = getenv("M3G_VIS_CAPTURE");
    return value != NULL && value[0] != '\0' && value[0] != '0';
}

static void ngl_sw_diag_init(void) {
    const char *dir;
    const char *frame;
    const char *limit;
    const char *mode;
    if (ngl_sw.diag_inited) return;
    ngl_sw.diag_inited = 1;
    ngl_sw.diag_enabled = ngl_sw_diag_env_enabled();
    if (!ngl_sw.diag_enabled) return;
    dir = getenv("M3G_VIS_DIR");
    if (dir == NULL || dir[0] == '\0') dir = "/tmp/opencode/m3g-vis";
    snprintf(ngl_sw.diag_dir, sizeof(ngl_sw.diag_dir), "%s", dir);
    mkdir(ngl_sw.diag_dir, 0777);
    frame = getenv("M3G_VIS_FRAME");
    ngl_sw.diag_capture_frame = frame != NULL && frame[0] != '\0' ? atoi(frame) : -1;
    limit = getenv("M3G_VIS_DRAW_LIMIT");
    ngl_sw.diag_draw_limit = limit != NULL && limit[0] != '\0' ? atoi(limit) : 160;
    if (ngl_sw.diag_draw_limit <= 0) ngl_sw.diag_draw_limit = 160;
    mode = getenv("M3G_VIS_MODE");
    ngl_sw.diag_mode = 0;
    if (mode != NULL && strcmp(mode, "solid") == 0) ngl_sw.diag_mode = 1;
    else if (mode != NULL && strcmp(mode, "nocull") == 0) ngl_sw.diag_mode = 2;
    else if (mode != NULL && strcmp(mode, "nodepth") == 0) ngl_sw.diag_mode = 3;
    else if (mode != NULL && strcmp(mode, "flipfront") == 0) ngl_sw.diag_mode = 4;
    if (ngl_sw_diag_flip_front_enabled()) ngl_sw.diag_mode = 4;
    fprintf(stderr,
            "[NGLDIAG] event=init dir=%s frame=%d drawLimit=%d mode=%d modeText=%s flipFront=%d\n",
            ngl_sw.diag_dir, ngl_sw.diag_capture_frame,
            ngl_sw.diag_draw_limit, ngl_sw.diag_mode,
            mode != NULL ? mode : "", ngl_sw_diag_flip_front_enabled());
}

static const char *ngl_sw_diag_source_name(void) {
    if (ngl_sw.diag_source == 1) return "direct";
    if (ngl_sw.diag_source == 2) return "renderNode";
    if (ngl_sw.diag_source == 3) return "renderWorld";
    return "unknown";
}

static unsigned int ngl_sw_diag_color(int index) {
    unsigned int x = (unsigned int)index * 2654435761U;
    unsigned int r = 64U + ((x >> 16) & 191U);
    unsigned int g = 64U + ((x >> 8) & 191U);
    unsigned int b = 64U + (x & 191U);
    return ngl_sw_pack(r, g, b, 255U);
}

static void ngl_sw_diag_write_rgb565(const char *kind, int draw_index) {
    char path[320];
    FILE *file;
    int y;
    if (!ngl_sw.diag_enabled || ngl_sw.pixels == NULL ||
            ngl_sw.width <= 0 || ngl_sw.height <= 0) return;
    if (draw_index >= 0) {
        snprintf(path, sizeof(path), "%s/%s-f%04d-d%04d.rgb565",
                 ngl_sw.diag_dir, kind, ngl_sw.diag_frame, draw_index);
    } else {
        snprintf(path, sizeof(path), "%s/%s-f%04d.rgb565",
                 ngl_sw.diag_dir, kind, ngl_sw.diag_frame);
    }
    file = fopen(path, "wb");
    if (file == NULL) return;
    for (y = 0; y < ngl_sw.height; ++y) {
        fwrite(ngl_sw.pixels + y * ngl_sw.stride, sizeof(unsigned short),
               (size_t)ngl_sw.width, file);
    }
    fclose(file);
    fprintf(stderr,
            "[NGLDIAG] event=image kind=%s frame=%d draw=%d path=%s width=%d height=%d stride=%d format=rgb565le\n",
            kind, ngl_sw.diag_frame, draw_index, path,
            ngl_sw.width, ngl_sw.height, ngl_sw.stride);
}

static int ngl_sw_diag_count_changed(const unsigned short *before) {
    int changed = 0;
    int x;
    int y;
    if (before == NULL || ngl_sw.pixels == NULL) return -1;
    for (y = 0; y < ngl_sw.height; ++y) {
        const unsigned short *src = before + y * ngl_sw.width;
        const unsigned short *dst = ngl_sw.pixels + y * ngl_sw.stride;
        for (x = 0; x < ngl_sw.width; ++x) {
            if (src[x] != dst[x]) ++changed;
        }
    }
    return changed;
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
    if (ngl_sw.bound_texture[unit] == 0) {
        return &ngl_sw.textures[unit];
    }
    return ngl_sw_texture(ngl_sw.bound_texture[unit]);
}

static void ngl_sw_diag_dump_texture(GLuint id, const NGLTexture *t) {
    char path[320];
    FILE *file;
    unsigned int slot = ((unsigned int)id) % NGL_SW_MAX_TEXTURES;
    if (!ngl_sw.diag_capture_active || t == NULL || t->argb == NULL ||
            t->width <= 0 || t->height <= 0 || ngl_sw.diag_texture_dumped[slot]) return;
    snprintf(path, sizeof(path), "%s/texture-%04u.argb",
             ngl_sw.diag_dir, (unsigned int)id);
    file = fopen(path, "wb");
    if (file == NULL) return;
    fwrite(t->argb, sizeof(unsigned int), (size_t)t->width * (size_t)t->height,
           file);
    fclose(file);
    ngl_sw.diag_texture_dumped[slot] = 1;
    fprintf(stderr,
            "[NGLDIAG] event=texture frame=%d texture=%u path=%s width=%d height=%d format=argb32-source\n",
            ngl_sw.diag_frame, (unsigned int)id, path, t->width, t->height);
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

static unsigned int ngl_sw_texture_pixel(const NGLTexture *t, int x, int y) {
    if (t->wrap_s == GL_REPEAT) {
        x %= t->width;
        if (x < 0) x += t->width;
    } else {
        if (x < 0) x = 0;
        if (x >= t->width) x = t->width - 1;
    }
    if (t->wrap_t == GL_REPEAT) {
        y %= t->height;
        if (y < 0) y += t->height;
    } else {
        if (y < 0) y = 0;
        if (y >= t->height) y = t->height - 1;
    }
    return t->argb[y * t->width + x];
}

static unsigned int ngl_sw_lerp_texel(unsigned int a, unsigned int b, float f) {
    unsigned int r = (unsigned int)((float)((a >> 16) & 0xff) +
                                    ((float)((b >> 16) & 0xff) - (float)((a >> 16) & 0xff)) * f + 0.5f);
    unsigned int g = (unsigned int)((float)((a >> 8) & 0xff) +
                                    ((float)((b >> 8) & 0xff) - (float)((a >> 8) & 0xff)) * f + 0.5f);
    unsigned int bl = (unsigned int)((float)(a & 0xff) +
                                     ((float)(b & 0xff) - (float)(a & 0xff)) * f + 0.5f);
    unsigned int al = (unsigned int)((float)((a >> 24) & 0xff) +
                                     ((float)((b >> 24) & 0xff) - (float)((a >> 24) & 0xff)) * f + 0.5f);
    return ngl_sw_pack(r, g, bl, al);
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
        float fx = s * (float)t->width - 0.5f;
        float fy = tt * (float)t->height - 0.5f;
        int x0 = (int)floorf(fx);
        int y0 = (int)floorf(fy);
        unsigned int a = ngl_sw_lerp_texel(ngl_sw_texture_pixel(t, x0, y0),
                                            ngl_sw_texture_pixel(t, x0 + 1, y0),
                                            fx - (float)x0);
        unsigned int b = ngl_sw_lerp_texel(ngl_sw_texture_pixel(t, x0, y0 + 1),
                                            ngl_sw_texture_pixel(t, x0 + 1, y0 + 1),
                                            fx - (float)x0);
        return ngl_sw_lerp_texel(a, b, fy - (float)y0);
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
    float object[4];
    float view[4];
    int source_index;
    float fog;
    float u[NGL_SW_TEXTURE_UNITS], v[NGL_SW_TEXTURE_UNITS];
    unsigned int color;
} NGLVertex;

static void ngl_sw_trace_screen_vertex(const NGLVertex *v) {
    float sx;
    float sy;
    if (v->w == 0.0f) return;
    sx = (v->x / v->w * 0.5f + 0.5f) * (float) ngl_sw.viewport[2] +
         (float) ngl_sw.viewport[0];
    sy = (v->y / v->w * 0.5f + 0.5f) * (float) ngl_sw.viewport[3] +
         (float) ngl_sw.viewport[1];
    if (sx < (float) ngl_sw.immediate_min_x) ngl_sw.immediate_min_x = (int) sx;
    if (sx > (float) ngl_sw.immediate_max_x) ngl_sw.immediate_max_x = (int) sx;
    if (sy < (float) ngl_sw.immediate_min_y) ngl_sw.immediate_min_y = (int) sy;
    if (sy > (float) ngl_sw.immediate_max_y) ngl_sw.immediate_max_y = (int) sy;
}

static inline void nglTraceFrameMatricesRequest(int frame) {
    ngl_sw.trace_frame_matrices_pending = 1;
    ngl_sw.trace_frame_matrix_frame = frame;
}

static void ngl_sw_trace_frame_matrices(void) {
    if (!ngl_sw.trace_frame_matrices_pending) return;
    fprintf(stderr,
            "[M3G FRAME MATRICES] frame=%d modelview=%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g "
            "projection=%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
            ngl_sw.trace_frame_matrix_frame,
            ngl_sw.modelview[0], ngl_sw.modelview[1], ngl_sw.modelview[2], ngl_sw.modelview[3],
            ngl_sw.modelview[4], ngl_sw.modelview[5], ngl_sw.modelview[6], ngl_sw.modelview[7],
            ngl_sw.modelview[8], ngl_sw.modelview[9], ngl_sw.modelview[10], ngl_sw.modelview[11],
            ngl_sw.modelview[12], ngl_sw.modelview[13], ngl_sw.modelview[14], ngl_sw.modelview[15],
            ngl_sw.projection[0], ngl_sw.projection[1], ngl_sw.projection[2], ngl_sw.projection[3],
            ngl_sw.projection[4], ngl_sw.projection[5], ngl_sw.projection[6], ngl_sw.projection[7],
            ngl_sw.projection[8], ngl_sw.projection[9], ngl_sw.projection[10], ngl_sw.projection[11],
            ngl_sw.projection[12], ngl_sw.projection[13], ngl_sw.projection[14], ngl_sw.projection[15]);
    ngl_sw.trace_frame_matrices_pending = 0;
}

static int ngl_sw_env_draw_selected(const char *name,
                                    unsigned long vb, int draw_call) {
    const char *text = getenv(name);
    char *end;
    unsigned long selected_vb;
    long selected_draw;
    if (text == NULL || text[0] == '\0') return 0;
    while (*text != '\0') {
        while (*text == ',' || *text == ' ' || *text == '\t') ++text;
        selected_vb = strtoul(text, &end, 10);
        if (end == text) {
            while (*text != '\0' && *text != ',') ++text;
            continue;
        }
        text = end;
        if (*text == ':') {
            ++text;
            selected_draw = strtol(text, &end, 10);
            if (end != text && selected_vb == vb && selected_draw == draw_call) {
                return 1;
            }
            text = end;
        } else if (selected_vb == vb) {
            return 1;
        }
    }
    return 0;
}

static void ngl_sw_capture_triangle(NGLImmediateTriangleSample *sample,
                                    NGLVertex a, NGLVertex b, NGLVertex c,
                                    float ax, float ay, float bx, float by,
                                    float cx, float cy, float area) {
    NGLVertex vertices[3];
    int i;
    if (sample->valid) return;
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    memset(sample, 0, sizeof(*sample));
    sample->valid = 1;
    sample->signed_area = area;
    sample->front_face = (area > 0.0f) == (ngl_sw.front_face == GL_CCW);
    sample->cull_mode = ngl_sw.cull_mode;
    for (i = 0; i < 3; ++i) {
        sample->index[i] = vertices[i].source_index;
        memcpy(sample->object[i], vertices[i].object, sizeof(sample->object[i]));
        sample->clip[i][0] = vertices[i].x;
        sample->clip[i][1] = vertices[i].y;
        sample->clip[i][2] = vertices[i].z;
        sample->clip[i][3] = vertices[i].w;
        sample->uv[i][0] = vertices[i].u[0];
        sample->uv[i][1] = vertices[i].v[0];
    }
    sample->screen[0][0] = ax;
    sample->screen[0][1] = ay;
    sample->screen[1][0] = bx;
    sample->screen[1][1] = by;
    sample->screen[2][0] = cx;
    sample->screen[2][1] = cy;
}

static void ngl_sw_print_triangle_sample(const char *kind,
                                         const NGLImmediateTriangleSample *s,
                                         const NGLImmediateRecord *r) {
    int i;
    if (!s->valid) {
        fprintf(stderr,
                "[M3G IMMEDIATE TRI] kind=%s frame=%d drawCall=%d vb=%lu none\n",
                kind, r->frame, r->draw_call, r->vb);
        return;
    }
    fprintf(stderr,
            "[M3G IMMEDIATE TRI] kind=%s frame=%d drawCall=%d vb=%lu "
            "idx=%d,%d,%d area=%g frontFace=%d cullMode=%u",
            kind, r->frame, r->draw_call, r->vb,
            s->index[0], s->index[1], s->index[2], s->signed_area,
            s->front_face, (unsigned int)s->cull_mode);
    for (i = 0; i < 3; ++i) {
        fprintf(stderr,
                " v%d=obj(%g,%g,%g,%g) clip(%g,%g,%g,%g) screen(%g,%g) uv(%g,%g)",
                i,
                s->object[i][0], s->object[i][1], s->object[i][2], s->object[i][3],
                s->clip[i][0], s->clip[i][1], s->clip[i][2], s->clip[i][3],
                s->screen[i][0], s->screen[i][1],
                s->uv[i][0], s->uv[i][1]);
    }
    fprintf(stderr, "\n");
}

static void ngl_sw_print_immediate_record(const NGLImmediateRecord *r) {
    fprintf(stderr,
            "[M3G IMMEDIATE DRAW] frame=%d drawCall=%d vb=%lu ib=%lu app=%lu "
            "tex=%lu image=%lu vertices=%d indices=%d "
            "inputTri=%d raster=%d clip=%d,%d,%d,%d,%d,%d,%d "
            "wClip=%d wReject=%d areaZero=%d culled=%d depthReject=%d "
            "alphaReject=%d boundsValid=%d wRange=%g,%g "
             "bounds=%d,%d,%d,%d pixels=%d cullOverride=%d highlight=%d\n",
            r->frame, r->draw_call, r->vb, r->ib, r->app, r->texture, r->image,
            r->vertex_count, r->index_count, r->input_triangles,
            r->raster_calls, r->clip_reject[0], r->clip_reject[1],
            r->clip_reject[2], r->clip_reject[3], r->clip_reject[4],
            r->clip_reject[5], r->clip_reject[6], r->clip_reject[0],
            r->w_reject, r->area_zero, r->culled, r->depth_reject,
            r->alpha_reject, r->bounds_valid, r->w_min, r->w_max,
             r->min_x, r->max_x, r->min_y, r->max_y, r->pixels,
             r->cull_override, r->highlight);
    ngl_sw_print_triangle_sample("nonzero", &r->first_nonzero, r);
    ngl_sw_print_triangle_sample("culled", &r->first_culled, r);
    ngl_sw_print_triangle_sample("passed", &r->first_passed, r);
}

static inline void nglTraceImmediateFrameBegin(void) {
    ngl_sw.immediate_record_count = 0;
}

static inline void nglTraceImmediateFrameFlush(void) {
    int i;
    for (i = 0; i < ngl_sw.immediate_record_count; ++i) {
        ngl_sw_print_immediate_record(&ngl_sw.immediate_records[i]);
    }
    ngl_sw.immediate_record_count = 0;
}

static void ngl_sw_transform_vertex(int index, NGLVertex *out) {
    float p[4];
    float mv[4];
    float clip[4];
    float tc[4];
    int unit;
    ngl_sw_vec(&ngl_sw.vertex, index, p, 4);
    memcpy(out->object, p, sizeof(out->object));
    out->source_index = index;
    ngl_sw_transform_vec4(mv, ngl_sw.modelview, p);
    ngl_sw_transform_vec4(clip, ngl_sw.projection, mv);
    out->x = clip[0];
    out->y = clip[1];
    out->z = clip[2];
    out->w = clip[3];
    memcpy(out->view, mv, sizeof(out->view));
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

static void ngl_sw_rally_trace_geometry(const NGLVertex *a,
                                        const NGLVertex *b,
                                        const NGLVertex *c) {
    const NGLVertex *vertices[3];
    int i;
    if (!ngl_sw_rally_trace_enabled() ||
            !ngl_sw.immediate_transform_valid ||
            ngl_sw.rally_geometry_trace_count >= 8) {
        return;
    }
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    fprintf(stderr,
            "[M3G RALLY NGL MATRICES] frame=%d drawCall=%d "
            "sourceLayout=row-major modelviewLayout=column-major "
            "projectionLayout=column-major sourceTransform=",
            ngl_sw.immediate_frame, ngl_sw.immediate_draw_call);
    for (i = 0; i < 16; ++i) {
        fprintf(stderr, "%s%g", i == 0 ? "" : ",",
                ngl_sw.immediate_transform[i]);
    }
    fprintf(stderr, " modelview=");
    for (i = 0; i < 16; ++i) {
        fprintf(stderr, "%s%g", i == 0 ? "" : ",", ngl_sw.modelview[i]);
    }
    fprintf(stderr, " projection=");
    for (i = 0; i < 16; ++i) {
        fprintf(stderr, "%s%g", i == 0 ? "" : ",", ngl_sw.projection[i]);
    }
    fprintf(stderr, " order=clip=projection*modelview*object\n");
    for (i = 0; i < 3; ++i) {
        const NGLVertex *v = vertices[i];
        if (v->w != 0.0f) {
            fprintf(stderr,
                    "[M3G RALLY NGL VERTEX] frame=%d drawCall=%d v=%d "
                    "index=%d object=%g,%g,%g,%g view=%g,%g,%g,%g "
                    "clip=%g,%g,%g,%g ndc=%g,%g,%g\n",
                    ngl_sw.immediate_frame, ngl_sw.immediate_draw_call, i,
                    v->source_index,
                    v->object[0], v->object[1], v->object[2], v->object[3],
                    v->view[0], v->view[1], v->view[2], v->view[3],
                    v->x, v->y, v->z, v->w,
                    v->x / v->w, v->y / v->w, v->z / v->w);
        } else {
            fprintf(stderr,
                    "[M3G RALLY NGL VERTEX] frame=%d drawCall=%d v=%d "
                    "index=%d object=%g,%g,%g,%g view=%g,%g,%g,%g "
                    "clip=%g,%g,%g,%g ndc=undefined\n",
                    ngl_sw.immediate_frame, ngl_sw.immediate_draw_call, i,
                    v->source_index,
                    v->object[0], v->object[1], v->object[2], v->object[3],
                    v->view[0], v->view[1], v->view[2], v->view[3],
                    v->x, v->y, v->z, v->w);
        }
    }
    ++ngl_sw.rally_geometry_trace_count;
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
    out.object[0] = a.object[0] + (b.object[0] - a.object[0]) * t;
    out.object[1] = a.object[1] + (b.object[1] - a.object[1]) * t;
    out.object[2] = a.object[2] + (b.object[2] - a.object[2]) * t;
    out.object[3] = a.object[3] + (b.object[3] - a.object[3]) * t;
    out.view[0] = a.view[0] + (b.view[0] - a.view[0]) * t;
    out.view[1] = a.view[1] + (b.view[1] - a.view[1]) * t;
    out.view[2] = a.view[2] + (b.view[2] - a.view[2]) * t;
    out.view[3] = a.view[3] + (b.view[3] - a.view[3]) * t;
    out.source_index = -1;
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
        if (ngl_sw.trace_scene_draw) {
            ++ngl_sw.trace_scene_w_reject;
        }
        if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_w_reject;
        return;
    }
    /* Clipping already guarantees a positive w. Keep generated vertices on
     * the w epsilon plane instead of dropping camera-plane intersections. */
    if (a.w <= 0.0f || b.w <= 0.0f || c.w <= 0.0f) {
        if (ngl_sw.trace_scene_draw) {
            ++ngl_sw.trace_scene_w_reject;
        }
        if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_w_reject;
        return;
    }
    if (ngl_sw.trace_scene_draw) {
        ++ngl_sw.trace_scene_raster_calls;
    }
    if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_raster_calls;
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
        if (ngl_sw.trace_scene_draw) {
            ++ngl_sw.trace_scene_area_zero;
        }
        if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_area_zero;
        return;
    }
    if (ngl_sw.trace_immediate_draw) {
        ngl_sw_capture_triangle(&ngl_sw.immediate_first_nonzero,
                                a, b, c, ax, ay, bx, by, cx, cy, area);
    }
    if (ngl_sw.trace_current_draw) {
        ++ngl_sw.trace_triangles;
    }
    if (ngl_sw.cull_face &&
            !(ngl_sw.trace_immediate_draw && ngl_sw.immediate_disable_cull)) {
        int front = ((ngl_sw.diag_capture_active &&
                      (ngl_sw.diag_mode == 4 || ngl_sw_diag_flip_front_enabled())) ?
                     (area < 0.0f) : (area > 0.0f)) ==
                    (ngl_sw.front_face == GL_CCW);
        if ((front && ngl_sw.cull_mode == GL_FRONT) ||
            (!front && ngl_sw.cull_mode == GL_BACK)) {
            if (ngl_sw.trace_immediate_draw) {
                ngl_sw_capture_triangle(&ngl_sw.immediate_first_culled,
                                        a, b, c, ax, ay, bx, by, cx, cy, area);
            }
            if (ngl_sw.trace_current_draw) {
                ++ngl_sw.trace_culled;
            }
            return;
        }
    }
    if (ngl_sw.trace_immediate_draw) {
        ngl_sw_capture_triangle(&ngl_sw.immediate_first_passed,
                                a, b, c, ax, ay, bx, by, cx, cy, area);
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
                    if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_depth_reject;
                    if (ngl_sw.diag_capture_active) ++ngl_sw.diag_depth_reject;
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
                                      ngl_sw_bound_texture(unit) :
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
                if (ngl_sw.trace_immediate_draw) ++ngl_sw.immediate_alpha_reject;
                if (ngl_sw.diag_capture_active) ++ngl_sw.diag_alpha_reject;
                continue;
            }
            if (ngl_sw.diag_capture_active && ngl_sw.diag_mode == 1) {
                color = ngl_sw_diag_color(ngl_sw.diag_draw_index);
            }
            if (ngl_sw.trace_immediate_draw && ngl_sw.immediate_highlight) {
                color = 0xffff0000U;
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
    if (ngl_sw.trace_immediate_draw) {
        ++ngl_sw.immediate_input_triangles;
        if (ngl_sw.immediate_input_triangles == 1) {
            ngl_sw.immediate_w_min = a.w;
            ngl_sw.immediate_w_max = a.w;
        }
        if (a.w < ngl_sw.immediate_w_min) ngl_sw.immediate_w_min = a.w;
        if (b.w < ngl_sw.immediate_w_min) ngl_sw.immediate_w_min = b.w;
        if (c.w < ngl_sw.immediate_w_min) ngl_sw.immediate_w_min = c.w;
        if (a.w > ngl_sw.immediate_w_max) ngl_sw.immediate_w_max = a.w;
        if (b.w > ngl_sw.immediate_w_max) ngl_sw.immediate_w_max = b.w;
        if (c.w > ngl_sw.immediate_w_max) ngl_sw.immediate_w_max = c.w;
        if (a.w <= 0.0001f || b.w <= 0.0001f || c.w <= 0.0001f) {
            ngl_sw.immediate_bounds_valid = 0;
        }
        ngl_sw_trace_screen_vertex(&a);
        ngl_sw_trace_screen_vertex(&b);
        ngl_sw_trace_screen_vertex(&c);
    }
    if (ngl_sw.trace_scene_draw) {
        ++ngl_sw.trace_scene_input_triangles;
    }
    for (plane = 0; plane < 7 && count >= 3; ++plane) {
        count = ngl_sw_clip_plane(input, count, output, plane);
        if (count < 3 && ngl_sw.trace_scene_draw) {
            ++ngl_sw.trace_scene_clip_reject[plane];
        }
        if (count < 3 && ngl_sw.trace_immediate_draw) {
            ++ngl_sw.immediate_clip_reject[plane];
        }
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
    int reflect_stage = 0;
    float saved_modelview[16];
    float saved_projection[16];
    unsigned short *diag_before = NULL;
    GLboolean saved_texture_2d[NGL_SW_TEXTURE_UNITS];
    GLboolean saved_cull_face = GL_FALSE;
    GLboolean saved_depth_test = GL_FALSE;
    unsigned int saved_current_color = 0xffffffffU;
    int diag_this_draw = 0;
    int trace = ngl_sw.trace_draw_budget > 0;
    int scene_trace = ngl_sw.trace_scene_draw;
    int immediate_trace = ngl_sw.trace_immediate_draw;
    int collect_trace = trace || scene_trace || immediate_trace;
    NGLTexture *t0 = NULL;
    NGLTexture *t1 = NULL;
    (void)mode;
    if (!ngl_sw.vertex.enabled || ngl_sw.vertex.ptr == NULL || count < 3) return;
    ngl_sw_diag_init();
    if (ngl_sw.diag_capture_active) {
        if (ngl_sw.diag_draw_index < ngl_sw.diag_draw_limit) {
            size_t bytes = (size_t)ngl_sw.width * (size_t)ngl_sw.height *
                           sizeof(unsigned short);
            int y;
            diag_this_draw = ++ngl_sw.diag_draw_index;
            if (ngl_sw.pixels != NULL && ngl_sw.width > 0 && ngl_sw.height > 0) {
                diag_before = (unsigned short *)malloc(bytes);
                if (diag_before != NULL) {
                    for (y = 0; y < ngl_sw.height; ++y) {
                        memcpy(diag_before + y * ngl_sw.width,
                               ngl_sw.pixels + y * ngl_sw.stride,
                               (size_t)ngl_sw.width * sizeof(unsigned short));
                    }
                }
            }
            memcpy(saved_texture_2d, ngl_sw.texture_2d, sizeof(saved_texture_2d));
            saved_cull_face = ngl_sw.cull_face;
            saved_depth_test = ngl_sw.depth_test;
            saved_current_color = ngl_sw.current_color;
            if (ngl_sw.diag_mode == 1) {
                memset(ngl_sw.texture_2d, 0, sizeof(ngl_sw.texture_2d));
                ngl_sw.current_color = ngl_sw_diag_color(diag_this_draw);
            } else if (ngl_sw.diag_mode == 2) {
                ngl_sw.cull_face = GL_FALSE;
            } else if (ngl_sw.diag_mode == 3) {
                ngl_sw.depth_test = GL_FALSE;
            }
            ngl_sw.trace_current_draw = 1;
            ngl_sw.trace_triangles = 0;
            ngl_sw.trace_culled = 0;
            ngl_sw.trace_pixels = 0;
        } else {
            ++ngl_sw.diag_skipped_draws;
        }
    }
    if (collect_trace) {
        int plane;
        ngl_sw.trace_current_draw = 1;
        ngl_sw.trace_triangles = 0;
        ngl_sw.trace_culled = 0;
        ngl_sw.trace_pixels = 0;
        ngl_sw.trace_scene_input_triangles = 0;
        ngl_sw.trace_scene_raster_calls = 0;
        ngl_sw.trace_scene_w_reject = 0;
        ngl_sw.trace_scene_area_zero = 0;
        ngl_sw.immediate_input_triangles = 0;
        ngl_sw.immediate_raster_calls = 0;
        ngl_sw.immediate_w_reject = 0;
        ngl_sw.immediate_area_zero = 0;
        ngl_sw.immediate_depth_reject = 0;
        ngl_sw.immediate_alpha_reject = 0;
        ngl_sw.immediate_min_x = 2147483647;
        ngl_sw.immediate_max_x = -2147483647;
        ngl_sw.immediate_min_y = 2147483647;
        ngl_sw.immediate_max_y = -2147483647;
        ngl_sw.immediate_bounds_valid = 1;
        ngl_sw.immediate_w_min = 0.0f;
        ngl_sw.immediate_w_max = 0.0f;
        memset(&ngl_sw.immediate_first_nonzero, 0,
               sizeof(ngl_sw.immediate_first_nonzero));
        memset(&ngl_sw.immediate_first_culled, 0,
               sizeof(ngl_sw.immediate_first_culled));
        memset(&ngl_sw.immediate_first_passed, 0,
               sizeof(ngl_sw.immediate_first_passed));
        ngl_sw.immediate_disable_cull = immediate_trace &&
            ngl_sw_env_draw_selected("M3G_IMMEDIATE_DISABLE_CULL",
                                     ngl_sw.immediate_vb,
                                     ngl_sw.immediate_draw_call);
        ngl_sw.immediate_highlight = immediate_trace &&
            ngl_sw_env_draw_selected("M3G_IMMEDIATE_HIGHLIGHT",
                                     ngl_sw.immediate_vb,
                                     ngl_sw.immediate_draw_call);
        for (plane = 0; plane < 7; ++plane) {
            ngl_sw.trace_scene_clip_reject[plane] = 0;
            ngl_sw.immediate_clip_reject[plane] = 0;
        }
        if (ngl_sw.texture_2d[0]) {
            t0 = ngl_sw_bound_texture(0);
        }
        if (NGL_SW_TEXTURE_UNITS > 1 && ngl_sw.texture_2d[1]) {
            t1 = ngl_sw_bound_texture(1);
        }
    }
    if (ngl_sw.diag_capture_active && diag_this_draw > 0) {
        ngl_sw.diag_depth_reject = 0;
        ngl_sw.diag_alpha_reject = 0;
    }
    if (ngl_sw.diag_capture_active && diag_this_draw > 0) {
        if (ngl_sw.texture_2d[0]) t0 = ngl_sw_bound_texture(0);
        if (NGL_SW_TEXTURE_UNITS > 1 && ngl_sw.texture_2d[1]) {
            t1 = ngl_sw_bound_texture(1);
        }
        if (t0 != NULL) ngl_sw_diag_dump_texture(ngl_sw.bound_texture[0], t0);
        if (t1 != NULL) ngl_sw_diag_dump_texture(ngl_sw.bound_texture[1], t1);
    }
    if (immediate_trace) {
        reflect_stage = ngl_sw_rally_reflect_stage();
        if (reflect_stage != 0) {
            memcpy(saved_modelview, ngl_sw.modelview, sizeof(saved_modelview));
            memcpy(saved_projection, ngl_sw.projection, sizeof(saved_projection));
            if (reflect_stage == 1) {
                ngl_sw.modelview[0] = -ngl_sw.modelview[0];
                ngl_sw.modelview[1] = -ngl_sw.modelview[1];
                ngl_sw.modelview[2] = -ngl_sw.modelview[2];
                ngl_sw.modelview[3] = -ngl_sw.modelview[3];
            } else if (reflect_stage == 2) {
                ngl_sw.modelview[0] = -ngl_sw.modelview[0];
                ngl_sw.modelview[4] = -ngl_sw.modelview[4];
                ngl_sw.modelview[8] = -ngl_sw.modelview[8];
                ngl_sw.modelview[12] = -ngl_sw.modelview[12];
            } else {
                ngl_sw.projection[0] = -ngl_sw.projection[0];
                ngl_sw.projection[4] = -ngl_sw.projection[4];
                ngl_sw.projection[8] = -ngl_sw.projection[8];
                ngl_sw.projection[12] = -ngl_sw.projection[12];
            }
            fprintf(stderr,
                    "[M3G RALLY REFLECT] frame=%d drawCall=%d stage=%s "
                    "axis=X temporary=1\n",
                    ngl_sw.immediate_frame, ngl_sw.immediate_draw_call,
                    reflect_stage == 1 ? "model" :
                    (reflect_stage == 2 ? "view" : "projection"));
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
        if (scene_trace && i == 0) {
            float p0[4], p1[4], p2[4];
            float mv0[4], mv1[4], mv2[4];
            float cp0[4], cp1[4], cp2[4];
            ngl_sw_vec(&ngl_sw.vertex, ia, p0, 4);
            ngl_sw_vec(&ngl_sw.vertex, ib, p1, 4);
            ngl_sw_vec(&ngl_sw.vertex, ic, p2, 4);
            ngl_sw_transform_vec4(mv0, ngl_sw.modelview, p0);
            ngl_sw_transform_vec4(mv1, ngl_sw.modelview, p1);
            ngl_sw_transform_vec4(mv2, ngl_sw.modelview, p2);
            ngl_sw_transform_vec4(cp0, ngl_sw.projection, mv0);
            ngl_sw_transform_vec4(cp1, ngl_sw.projection, mv1);
            ngl_sw_transform_vec4(cp2, ngl_sw.projection, mv2);
            fprintf(stderr,
                    "[M3G NGL scene firsttri] idx=%d,%d,%d "
                    "p0=%g,%g,%g,%g mv0=%g,%g,%g,%g clip0=%g,%g,%g,%g z-w=%g "
                    "p1=%g,%g,%g,%g mv1=%g,%g,%g,%g clip1=%g,%g,%g,%g z-w=%g "
                    "p2=%g,%g,%g,%g mv2=%g,%g,%g,%g clip2=%g,%g,%g,%g z-w=%g\n",
                    ia, ib, ic,
                    p0[0], p0[1], p0[2], p0[3],
                    mv0[0], mv0[1], mv0[2], mv0[3],
                    cp0[0], cp0[1], cp0[2], cp0[3], cp0[2] - cp0[3],
                    p1[0], p1[1], p1[2], p1[3],
                    mv1[0], mv1[1], mv1[2], mv1[3],
                    cp1[0], cp1[1], cp1[2], cp1[3], cp1[2] - cp1[3],
                    p2[0], p2[1], p2[2], p2[3],
                    mv2[0], mv2[1], mv2[2], mv2[3],
                    cp2[0], cp2[1], cp2[2], cp2[3], cp2[2] - cp2[3]);
            fprintf(stderr,
                    "[M3G NGL scene matrices] "
                    "mv=%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g "
                    "proj=%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
                    ngl_sw.modelview[0], ngl_sw.modelview[1],
                    ngl_sw.modelview[2], ngl_sw.modelview[3],
                    ngl_sw.modelview[4], ngl_sw.modelview[5],
                    ngl_sw.modelview[6], ngl_sw.modelview[7],
                    ngl_sw.modelview[8], ngl_sw.modelview[9],
                    ngl_sw.modelview[10], ngl_sw.modelview[11],
                    ngl_sw.modelview[12], ngl_sw.modelview[13],
                    ngl_sw.modelview[14], ngl_sw.modelview[15],
                    ngl_sw.projection[0], ngl_sw.projection[1],
                    ngl_sw.projection[2], ngl_sw.projection[3],
                    ngl_sw.projection[4], ngl_sw.projection[5],
                    ngl_sw.projection[6], ngl_sw.projection[7],
                    ngl_sw.projection[8], ngl_sw.projection[9],
                    ngl_sw.projection[10], ngl_sw.projection[11],
                    ngl_sw.projection[12], ngl_sw.projection[13],
                    ngl_sw.projection[14], ngl_sw.projection[15]);
        }
        ngl_sw_transform_vertex(ia, &a);
        ngl_sw_transform_vertex(ib, &b);
        ngl_sw_transform_vertex(ic, &c);
        if (immediate_trace && i == 0) {
            ngl_sw_rally_trace_geometry(&a, &b, &c);
        }
        ngl_sw_draw_tri(a, b, c);
    }
    if (reflect_stage != 0) {
        memcpy(ngl_sw.modelview, saved_modelview, sizeof(saved_modelview));
        memcpy(ngl_sw.projection, saved_projection, sizeof(saved_projection));
    }
    if (ngl_sw.diag_capture_active && diag_this_draw > 0) {
        int changed = ngl_sw_diag_count_changed(diag_before);
        ngl_sw.diag_pixels_changed = changed;
        if (ngl_sw.diag_mode != 0) {
            memcpy(ngl_sw.texture_2d, saved_texture_2d, sizeof(saved_texture_2d));
            ngl_sw.cull_face = saved_cull_face;
            ngl_sw.depth_test = saved_depth_test;
            ngl_sw.current_color = saved_current_color;
        }
        ngl_sw_diag_write_rgb565(ngl_sw.diag_mode == 1 ? "solid-draw" : "draw",
                                 diag_this_draw);
        fprintf(stderr,
                "[NGLDIAG] event=draw frame=%d draw=%d source=%s mode=%d count=%d inputTri=%d rasterTri=%d culled=%d pixels=%d pixelsChanged=%d depthReject=%d alphaReject=%d tex0=%u tex0w=%d tex0h=%d tex1=%u tex1w=%d tex1h=%d depth=%d depthFunc=0x%x depthMask=%d cull=%d cullMode=0x%x frontFace=0x%x alpha=%d blend=%d bbox=%d,%d,%d,%d modelview=",
                ngl_sw.diag_frame, diag_this_draw, ngl_sw_diag_source_name(),
                ngl_sw.diag_mode, (int)count, count >= 3 ? (int)count - 2 : 0,
                ngl_sw.trace_triangles, ngl_sw.trace_culled, ngl_sw.trace_pixels,
                changed, ngl_sw.diag_depth_reject, ngl_sw.diag_alpha_reject,
                (unsigned int)ngl_sw.bound_texture[0],
                t0 != NULL ? t0->width : 0, t0 != NULL ? t0->height : 0,
                (unsigned int)ngl_sw.bound_texture[1],
                t1 != NULL ? t1->width : 0, t1 != NULL ? t1->height : 0,
                (int)saved_depth_test, (unsigned int)ngl_sw.depth_func,
                (int)ngl_sw.depth_mask, (int)saved_cull_face,
                (unsigned int)ngl_sw.cull_mode, (unsigned int)ngl_sw.front_face,
                (int)ngl_sw.alpha_test, (int)ngl_sw.blend,
                0, ngl_sw.width - 1, 0, ngl_sw.height - 1);
        for (i = 0; i < 16; ++i) fprintf(stderr, "%s%g", i == 0 ? "" : ",", ngl_sw.modelview[i]);
        fprintf(stderr, " projection=");
        for (i = 0; i < 16; ++i) fprintf(stderr, "%s%g", i == 0 ? "" : ",", ngl_sw.projection[i]);
        fprintf(stderr, "\n");
        ngl_sw.trace_current_draw = 0;
    }
    free(diag_before);
    if (trace) {
        fprintf(stderr,
                "[M3G NGL draw] idx=%d count=%d tex0=%dx%d tex1=%dx%d "
                "uv0=%d/%p uv1=%d/%p "
                "depth=%d/%x/%d cull=%d/%x/%x alpha=%d blend=%d "
                "offset=%d tri=%d culled=%d pixels=%d\n",
                ngl_sw.trace_draw_budget, (int)count,
                t0 != NULL ? t0->width : 0, t0 != NULL ? t0->height : 0,
                t1 != NULL ? t1->width : 0, t1 != NULL ? t1->height : 0,
                ngl_sw.texcoord[0].enabled, ngl_sw.texcoord[0].ptr,
                ngl_sw.texcoord[1].enabled, ngl_sw.texcoord[1].ptr,
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
    if (scene_trace) {
        fprintf(stderr,
                "[M3G NGL scene draw] count=%d tex0=%dx%d tex1=%dx%d "
                "uv0=%d/%p uv1=%d/%p "
                "depth=%d/%x/%d cull=%d/%x/%x alpha=%d blend=%d "
                "offset=%d inputTri=%d raster=%d clip=%d,%d,%d,%d,%d,%d,%d "
                "wReject=%d areaZero=%d tri=%d culled=%d pixels=%d\n",
                (int)count,
                t0 != NULL ? t0->width : 0, t0 != NULL ? t0->height : 0,
                t1 != NULL ? t1->width : 0, t1 != NULL ? t1->height : 0,
                ngl_sw.texcoord[0].enabled, ngl_sw.texcoord[0].ptr,
                ngl_sw.texcoord[1].enabled, ngl_sw.texcoord[1].ptr,
                ngl_sw.depth_test, (unsigned int)ngl_sw.depth_func,
                ngl_sw.depth_mask, ngl_sw.cull_face,
                (unsigned int)ngl_sw.cull_mode,
                (unsigned int)ngl_sw.front_face,
                ngl_sw.alpha_test, ngl_sw.blend, ngl_sw.polygon_offset_fill,
                ngl_sw.trace_scene_input_triangles,
                ngl_sw.trace_scene_raster_calls,
                ngl_sw.trace_scene_clip_reject[0],
                ngl_sw.trace_scene_clip_reject[1],
                ngl_sw.trace_scene_clip_reject[2],
                ngl_sw.trace_scene_clip_reject[3],
                ngl_sw.trace_scene_clip_reject[4],
                ngl_sw.trace_scene_clip_reject[5],
                ngl_sw.trace_scene_clip_reject[6],
                ngl_sw.trace_scene_w_reject,
                ngl_sw.trace_scene_area_zero,
                ngl_sw.trace_triangles, ngl_sw.trace_culled,
                ngl_sw.trace_pixels);
        ngl_sw.trace_current_draw = 0;
        ngl_sw.trace_scene_draw = 0;
    }
    if (immediate_trace) {
        NGLImmediateRecord record;
        ngl_sw_trace_frame_matrices();
        record.frame = ngl_sw.immediate_frame;
        record.draw_call = ngl_sw.immediate_draw_call;
        record.vb = ngl_sw.immediate_vb;
        record.ib = ngl_sw.immediate_ib;
        record.app = ngl_sw.immediate_app;
        record.texture = ngl_sw.immediate_texture;
        record.image = ngl_sw.immediate_image;
        record.vertex_count = ngl_sw.immediate_vertex_count;
        record.index_count = ngl_sw.immediate_index_count;
        record.input_triangles = ngl_sw.immediate_input_triangles;
        record.raster_calls = ngl_sw.immediate_raster_calls;
        for (i = 0; i < 7; ++i) record.clip_reject[i] = ngl_sw.immediate_clip_reject[i];
        record.w_reject = ngl_sw.immediate_w_reject;
        record.area_zero = ngl_sw.immediate_area_zero;
        record.culled = ngl_sw.trace_culled;
        record.depth_reject = ngl_sw.immediate_depth_reject;
        record.alpha_reject = ngl_sw.immediate_alpha_reject;
        record.bounds_valid = ngl_sw.immediate_bounds_valid;
        record.w_min = ngl_sw.immediate_w_min;
        record.w_max = ngl_sw.immediate_w_max;
        record.cull_override = ngl_sw.immediate_disable_cull;
        record.highlight = ngl_sw.immediate_highlight;
        memcpy(&record.first_nonzero, &ngl_sw.immediate_first_nonzero,
               sizeof(record.first_nonzero));
        memcpy(&record.first_culled, &ngl_sw.immediate_first_culled,
               sizeof(record.first_culled));
        memcpy(&record.first_passed, &ngl_sw.immediate_first_passed,
               sizeof(record.first_passed));
        record.min_x = ngl_sw.immediate_min_x == 2147483647 ? 0 : ngl_sw.immediate_min_x;
        record.max_x = ngl_sw.immediate_max_x == -2147483647 ? 0 : ngl_sw.immediate_max_x;
        record.min_y = ngl_sw.immediate_min_y == 2147483647 ? 0 : ngl_sw.immediate_min_y;
        record.max_y = ngl_sw.immediate_max_y == -2147483647 ? 0 : ngl_sw.immediate_max_y;
        record.pixels = ngl_sw.trace_pixels;
        if (ngl_sw.immediate_defer && ngl_sw.immediate_record_count < 32) {
            ngl_sw.immediate_records[ngl_sw.immediate_record_count++] = record;
        } else {
            ngl_sw_print_immediate_record(&record);
        }
        ngl_sw.trace_current_draw = 0;
        ngl_sw.trace_immediate_draw = 0;
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
    if (ngl_sw_verbose_trace_enabled()) {
        fprintf(stderr, "[M3G NGL target] pixels=%p size=%dx%d stride=%d depth=%d\n",
                pixels, width, height, ngl_sw.stride, ngl_sw.depth_count);
    }
}

static inline void nglTraceFrame(int draw_budget, int upload_budget) {
    ngl_sw.trace_draw_budget = draw_budget;
    ngl_sw.trace_upload_budget = upload_budget;
    ngl_sw.trace_current_draw = 0;
    ngl_sw.trace_scene_draw = 0;
}

static inline void nglDiagFrameBegin(int frame, int auto_capture) {
    ngl_sw_diag_init();
    if (!ngl_sw.diag_enabled) return;
    if (ngl_sw.diag_capture_active && !ngl_sw.diag_done) {
        ngl_sw_diag_write_rgb565("frame-final", -1);
        fprintf(stderr,
                "[NGLDIAG] event=frameEnd frame=%d draws=%d skipped=%d state=DONE\n",
                ngl_sw.diag_frame, ngl_sw.diag_draw_index,
                ngl_sw.diag_skipped_draws);
        ngl_sw.diag_done = 1;
        ngl_sw.diag_capture_active = 0;
    }
    ngl_sw.diag_frame = frame;
    ngl_sw.diag_draw_index = 0;
    ngl_sw.diag_skipped_draws = 0;
    memset(ngl_sw.diag_texture_dumped, 0, sizeof(ngl_sw.diag_texture_dumped));
    if (!ngl_sw.diag_done &&
            ((ngl_sw.diag_capture_frame >= 0 && frame == ngl_sw.diag_capture_frame) ||
             (ngl_sw.diag_capture_frame < 0 && auto_capture))) {
        ngl_sw.diag_capture_active = 1;
        fprintf(stderr,
                "[NGLDIAG] event=frameBegin frame=%d mode=%d sourceAuto=%d width=%d height=%d stride=%d\n",
                frame, ngl_sw.diag_mode, auto_capture, ngl_sw.width,
                ngl_sw.height, ngl_sw.stride);
        ngl_sw_diag_write_rgb565("frame-begin", -1);
    }
}

static inline void nglDiagForceCapture(void) {
    ngl_sw_diag_init();
    if (!ngl_sw.diag_enabled || ngl_sw.diag_done || ngl_sw.diag_capture_active) {
        return;
    }
    if (ngl_sw.diag_capture_frame >= 0 && ngl_sw.diag_frame != ngl_sw.diag_capture_frame) {
        return;
    }
    ngl_sw.diag_draw_index = 0;
    ngl_sw.diag_skipped_draws = 0;
    ngl_sw.diag_capture_active = 1;
    fprintf(stderr,
            "[NGLDIAG] event=frameBegin frame=%d mode=%d sourceAuto=1 width=%d height=%d stride=%d\n",
            ngl_sw.diag_frame, ngl_sw.diag_mode, ngl_sw.width,
            ngl_sw.height, ngl_sw.stride);
    ngl_sw_diag_write_rgb565("frame-begin", -1);
}

static inline void nglDiagFinish(void) {
    ngl_sw_diag_init();
    if (!ngl_sw.diag_enabled || !ngl_sw.diag_capture_active || ngl_sw.diag_done) return;
    ngl_sw_diag_write_rgb565("frame-final", -1);
    fprintf(stderr,
            "[NGLDIAG] event=frameEnd frame=%d draws=%d skipped=%d state=DONE\n",
            ngl_sw.diag_frame, ngl_sw.diag_draw_index,
            ngl_sw.diag_skipped_draws);
    ngl_sw.diag_done = 1;
    ngl_sw.diag_capture_active = 0;
}

static inline void nglDiagSetSource(int source) {
    ngl_sw_diag_init();
    ngl_sw.diag_source = source;
}

static inline void nglTraceSceneDraw(void) {
    ngl_sw.trace_scene_draw = 1;
}

static inline void nglTraceImmediateTransform(const float *matrix) {
    if (matrix != NULL) {
        memcpy(ngl_sw.immediate_transform, matrix,
               sizeof(ngl_sw.immediate_transform));
    } else {
        ngl_sw_identity(ngl_sw.immediate_transform);
    }
    ngl_sw.immediate_transform_valid = 1;
}

static inline void nglTraceImmediateMesh(unsigned long vb,
                                         unsigned long ib,
                                         unsigned long app,
                                         unsigned long texture,
                                         unsigned long image,
                                         int vertex_count,
                                         int index_count,
                                         int frame,
                                         int draw_call,
                                         int defer) {
    ngl_sw.trace_immediate_draw = 1;
    ngl_sw.immediate_vb = vb;
    ngl_sw.immediate_ib = ib;
    ngl_sw.immediate_app = app;
    ngl_sw.immediate_texture = texture;
    ngl_sw.immediate_image = image;
    ngl_sw.immediate_vertex_count = vertex_count;
    ngl_sw.immediate_index_count = index_count;
    ngl_sw.immediate_frame = frame;
    ngl_sw.immediate_draw_call = draw_call;
    ngl_sw.immediate_defer = defer;
}

static inline void nglTraceSceneModelviewReset(void) {
    if (!ngl_sw_verbose_trace_enabled()) return;
    if (!ngl_sw.trace_scene_matrix_done) {
        memcpy(ngl_sw.trace_scene_modelview_identity, ngl_sw.modelview,
               sizeof(ngl_sw.modelview));
        ngl_sw.trace_scene_matrix_pending = 1;
    }
}

static inline int nglTraceSceneModelviewBegin(void) {
    if (!ngl_sw.trace_scene_matrix_pending ||
            ngl_sw.trace_scene_matrix_done) {
        return 0;
    }
    ngl_sw.trace_scene_matrix_pending = 0;
    ngl_sw.trace_scene_matrix_active = 1;
    ngl_sw.trace_scene_matrix_stage = 0;
    ngl_sw_trace_matrix("AFTER IDENTITY", ngl_sw.trace_scene_modelview_identity);
    return 1;
}

static inline void nglTraceSceneModelviewExpectModel(void) {
    if (ngl_sw.trace_scene_matrix_active) {
        ngl_sw.trace_scene_matrix_stage = 1;
    }
}

static inline void nglTraceSceneVertexScale(float scale, const float *bias) {
    if (ngl_sw.trace_scene_matrix_active) {
        fprintf(stderr,
                "[M3G SCENE VERTEX SCALE] scale=%g bias=%g,%g,%g\n",
                scale, bias[0], bias[1], bias[2]);
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
static inline void glLoadIdentity(void) {
    ngl_sw_identity(ngl_sw_matrix());
    if (ngl_sw.trace_scene_matrix_pending &&
            ngl_sw.matrix_mode == GL_MODELVIEW &&
            !ngl_sw.trace_scene_matrix_done) {
        memcpy(ngl_sw.trace_scene_modelview_identity, ngl_sw.modelview,
               sizeof(ngl_sw.modelview));
    }
}
static inline void glLoadMatrixf(const GLfloat *m) { memcpy(ngl_sw_matrix(), m, 16 * sizeof(float)); }
static inline void glMultMatrixf(const GLfloat *m) {
    ngl_sw_mul(ngl_sw_matrix(), ngl_sw_matrix(), m);
    if (ngl_sw.trace_scene_matrix_active &&
            ngl_sw.trace_scene_matrix_stage == 1 &&
            ngl_sw.matrix_mode == GL_MODELVIEW) {
        ngl_sw_trace_matrix("AFTER MODEL", ngl_sw.modelview);
        ngl_sw.trace_scene_matrix_stage = 2;
    }
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
    if (ngl_sw.trace_scene_matrix_active &&
            ngl_sw.trace_scene_matrix_stage == 3 &&
            ngl_sw.matrix_mode == GL_MODELVIEW) {
        ngl_sw_trace_matrix("AFTER SCALE", ngl_sw.modelview);
        ngl_sw.trace_scene_matrix_active = 0;
        ngl_sw.trace_scene_matrix_done = 1;
    }
}
static inline void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    float m[16]; ngl_sw_identity(m); m[12] = x; m[13] = y; m[14] = z; glMultMatrixf(m);
    if (ngl_sw.trace_scene_matrix_active &&
            ngl_sw.trace_scene_matrix_stage == 2 &&
            ngl_sw.matrix_mode == GL_MODELVIEW) {
        ngl_sw_trace_matrix("AFTER BIAS", ngl_sw.modelview);
        ngl_sw.trace_scene_matrix_stage = 3;
    }
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
static inline void glBindTexture(GLenum target, GLuint texture) { int unit = ngl_sw_texture_unit(ngl_sw.active_texture); (void)target; if (unit >= 0) { ngl_sw.bound_texture[unit] = texture; ngl_sw_bound_texture(unit); } }
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
    t = ngl_sw_bound_texture(unit);
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
    t = ngl_sw_bound_texture(unit);
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
    t = ngl_sw_bound_texture(unit);
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
    unsigned int alpha_min = 255U;
    unsigned int alpha_max = 0U;
    int alpha_zero = 0;
    const unsigned char *src;
    (void)target; (void)internalformat; (void)border;
    if (level != 0) return;
    unit = ngl_sw_texture_unit(ngl_sw.active_texture);
    if (unit < 0 || type != GL_UNSIGNED_BYTE) {
        ngl_sw.error = GL_INVALID_OPERATION;
        return;
    }
    if (ngl_sw.trace_upload_budget > 0) {
        fprintf(stderr, "[NGL TEX] texture=%u unit=%d data=%p width=%d height=%d format=0x%x type=0x%x\n",
                (unsigned int)ngl_sw.bound_texture[unit], unit, pixels,
                (int)width, (int)height, (unsigned int)format,
                (unsigned int)type);
        --ngl_sw.trace_upload_budget;
    }
    t = ngl_sw_bound_texture(unit);
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
    if (ngl_sw.trace_upload_budget > 0 && t->argb != NULL) {
        int pixel_count = width * height;
        int i;
        for (i = 0; i < pixel_count; ++i) {
            unsigned int alpha = (t->argb[i] >> 24) & 0xffU;
            if (alpha < alpha_min) alpha_min = alpha;
            if (alpha > alpha_max) alpha_max = alpha;
            if (alpha == 0U) ++alpha_zero;
        }
        fprintf(stderr,
                "[NGL TEX DATA] texture=%u format=0x%x first=0x%08x "
                "alpha=%u..%u zero=%d/%d\n",
                (unsigned int)ngl_sw.bound_texture[unit],
                (unsigned int)format, t->argb[0], alpha_min, alpha_max,
                alpha_zero, pixel_count);
    }
}
static inline void glDrawArrays(GLenum mode, GLint first, GLsizei count) { ngl_sw_draw_indexed(mode, count, GL_UNSIGNED_SHORT, NULL, first); }
static inline void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) { ngl_sw_draw_indexed(mode, count, type, indices, 0); }
static inline void glClear(GLbitfield mask) {
    int x, y, x0, y0, x1, y1;
    if (ngl_sw_verbose_trace_enabled()) {
        fprintf(stderr,
                "[M3G NGL clear] mask=0x%x color=0x%08x depth=%g clip=%d,%d %dx%d\n",
                (unsigned int)mask, ngl_sw.clear_color, ngl_sw.clear_depth,
                ngl_sw.scissor[0], ngl_sw.scissor[1],
                ngl_sw.scissor[2], ngl_sw.scissor[3]);
    }
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
    for (i = 0; i < NGL_SW_MAX_TEXTURES; ++i) free(ngl_sw.textures[i].argb);
    free(ngl_sw.depth);
    memset(&ngl_sw, 0, sizeof(ngl_sw));
}
static inline void nglInitTextures(GLsizei count, const GLuint *textures) { GLsizei i; for (i = 0; i < count; ++i) ngl_sw_texture(textures[i]); }
static inline void nglBindTextureInternal(GLenum target, GLuint texture) { glBindTexture(target, texture); }

#endif
