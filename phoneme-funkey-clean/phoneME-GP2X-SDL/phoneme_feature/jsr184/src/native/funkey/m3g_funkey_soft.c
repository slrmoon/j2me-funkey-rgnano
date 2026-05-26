#include "m3g_funkey_soft.h"
#include "m3g_core.h"
#include "ngl.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

NGLContext ngl_sw;

#define FUNKEY_M3G_MAX_OBJECTS 4096
#define FUNKEY_M3G_MAX_CHILDREN 64
#define FUNKEY_M3G_MAX_SUBMESHES 64
#define FUNKEY_M3G_NUM_TEXTURE_UNITS 2
#define FUNKEY_M3G_MAX_ANIMATION_TRACKS 32
#define FUNKEY_M3G_MAX_LOADER_REFS 512

typedef struct FunKeyM3GObject {
    int alive;
    int class_id;
    int ref_count;
    M3GObject core;
    M3GInterface core_interface;
    M3GObject loader_external_refs[FUNKEY_M3G_MAX_LOADER_REFS];
    int loader_external_ref_count;
    int user_id;
    long parent;
    long z_ref;
    long y_ref;
    int scope;
    float alpha;
    int rendering_enabled;
    int picking_enabled;
    float translation[3];
    float scale[3];
    float orientation[4];
    float transform[16];
    long children[FUNKEY_M3G_MAX_CHILDREN];
    int child_count;
    long active_camera;
    long background;
    int projection_mode;
    float projection_params[4];
    float projection_matrix[16];
    unsigned int background_color;
    long background_image;
    int image_mode_x;
    int image_mode_y;
    int color_clear_enabled;
    int depth_clear_enabled;
    int crop_x;
    int crop_y;
    int crop_w;
    int crop_h;
    int vertex_count;
    int component_count;
    int component_type;
    int *components;
    long vb_positions;
    long vb_normals;
    long vb_colors;
    long vb_texcoords[FUNKEY_M3G_NUM_TEXTURE_UNITS];
    float vb_position_scale_bias[4];
    float vb_texcoord_scale_bias[FUNKEY_M3G_NUM_TEXTURE_UNITS][4];
    unsigned int default_color;
    int *indices;
    int index_count;
    long mesh_vertices;
    long mesh_indices[FUNKEY_M3G_MAX_SUBMESHES];
    long mesh_appearances[FUNKEY_M3G_MAX_SUBMESHES];
    int mesh_submesh_count;
    long morph_targets[FUNKEY_M3G_MAX_SUBMESHES];
    float morph_weights[FUNKEY_M3G_MAX_SUBMESHES];
    int morph_target_count;
    long skin_skeleton;
    int sprite_scaled;
    long sprite_image;
    long sprite_appearance;
    int image_format;
    int image_width;
    int image_height;
    int image_mutable;
    unsigned char *image_pixels;
    int image_pixel_count;
    unsigned char *image_palette;
    int image_palette_count;
    long texture_image;
    int texture_level_filter;
    int texture_image_filter;
    int texture_wrap_s;
    int texture_wrap_t;
    int texture_blending;
    unsigned int texture_blend_color;
    long appearance_compositing;
    long appearance_fog;
    long appearance_material;
    long appearance_polygon;
    long appearance_textures[FUNKEY_M3G_NUM_TEXTURE_UNITS];
    int appearance_layer;
    unsigned int material_ambient;
    unsigned int material_diffuse;
    unsigned int material_emissive;
    unsigned int material_specular;
    float material_shininess;
    int material_vertex_color_tracking;
    int compositing_blending;
    float compositing_alpha_threshold;
    int compositing_alpha_write;
    int compositing_depth_test;
    int compositing_depth_write;
    int compositing_color_write;
    float compositing_depth_offset_factor;
    float compositing_depth_offset_units;
    int anim_active_start;
    int anim_active_end;
    int anim_ref_world_time;
    float anim_position;
    float anim_speed;
    float anim_weight;
    long animation_tracks[FUNKEY_M3G_MAX_ANIMATION_TRACKS];
    int animation_track_count;
    long track_sequence;
    long track_controller;
    int track_property;
    int keyframe_count;
    int keyframe_components;
    int keyframe_interpolation;
    int keyframe_repeat;
    int keyframe_duration;
    int keyframe_valid_first;
    int keyframe_valid_last;
    int *keyframe_times;
    float *keyframe_values;
    int polygon_culling;
    int polygon_winding;
    int polygon_shading;
    int polygon_two_sided_lighting;
    int polygon_local_camera_lighting;
    int polygon_perspective_correction;
    int fog_mode;
    float fog_near;
    float fog_far;
    float fog_density;
    unsigned int fog_color;
    float light_intensity;
    unsigned int light_color;
    int light_mode;
    float light_spot_angle;
    float light_spot_exponent;
    float light_attenuation[3];
} FunKeyM3GObject;

static FunKeyM3GObject g_objects[FUNKEY_M3G_MAX_OBJECTS];
static M3GInterface g_core_interface;
static int g_mesh_trace_count;
static long g_mesh_trace_world;
static int g_texture_set_trace_count;
static long g_ngl_trace_world;
static int g_ngl_trace_frame;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

M3Gsizei
m3gSymbianInflateBlock(M3Gsizei srcLength, const M3Gubyte *src,
                       M3Gsizei dstLength, M3Gubyte *dst) {
    uLongf len = (uLongf) dstLength;
    if (src == 0 || dst == 0 || srcLength < 0 || dstLength < 0) {
        return 0;
    }
    if (uncompress((Bytef *) dst, &len, (const Bytef *) src,
                   (uLong) srcLength) != Z_OK) {
        return 0;
    }
    return (M3Gsizei) len;
}

static void
funkey_m3g_free_object(FunKeyM3GObject *obj) {
    if (obj == 0) {
        return;
    }
    if (obj->components != 0) {
        free(obj->components);
    }
    if (obj->indices != 0) {
        free(obj->indices);
    }
    if (obj->image_pixels != 0) {
        free(obj->image_pixels);
    }
    if (obj->image_palette != 0) {
        free(obj->image_palette);
    }
    if (obj->keyframe_times != 0) {
        free(obj->keyframe_times);
    }
    if (obj->keyframe_values != 0) {
        free(obj->keyframe_values);
    }
    if (obj->core != 0) {
        m3gDeleteRef(obj->core);
    }
    memset(obj, 0, sizeof(*obj));
}

static FunKeyM3GObject *
funkey_m3g_object(long handle) {
    if (handle <= 0 || handle >= FUNKEY_M3G_MAX_OBJECTS) {
        return 0;
    }
    if (!g_objects[handle].alive) {
        return 0;
    }
    return &g_objects[handle];
}

static unsigned int funkey_m3g_image_argb(FunKeyM3GObject *image, int x, int y);
static void funkey_m3g_matrix_from_float(M3GMatrix *dst, const float *src);
static void funkey_m3g_matrix_identity(float *m);

static long
funkey_m3g_find_core_handle(M3GObject core) {
    long i;
    if (core == 0) {
        return 0;
    }
    for (i = 1; i < FUNKEY_M3G_MAX_OBJECTS; ++i) {
        if (g_objects[i].alive && g_objects[i].core == core) {
            return i;
        }
    }
    return 0;
}

static long
funkey_m3g_alloc_handle(void) {
    long i;
    int j;
    for (i = 1; i < FUNKEY_M3G_MAX_OBJECTS; ++i) {
        if (!g_objects[i].alive) {
            memset(&g_objects[i], 0, sizeof(g_objects[i]));
            g_objects[i].alive = 1;
            g_objects[i].ref_count = 1;
            g_objects[i].scope = -1;
            g_objects[i].alpha = 1.0f;
            g_objects[i].rendering_enabled = 1;
            g_objects[i].picking_enabled = 1;
            g_objects[i].scale[0] = 1.0f;
            g_objects[i].scale[1] = 1.0f;
            g_objects[i].scale[2] = 1.0f;
            g_objects[i].orientation[3] = 1.0f;
            for (j = 0; j < 16; ++j) {
                g_objects[i].transform[j] = 0.0f;
            }
            g_objects[i].transform[0] = 1.0f;
            g_objects[i].transform[5] = 1.0f;
            g_objects[i].transform[10] = 1.0f;
            g_objects[i].transform[15] = 1.0f;
            g_objects[i].background_color = 0xff000000U;
            g_objects[i].projection_mode = 50;
            g_objects[i].projection_params[0] = 45.0f;
            g_objects[i].projection_params[1] = 1.0f;
            g_objects[i].projection_params[2] = 1.0f;
            g_objects[i].projection_params[3] = 100.0f;
            funkey_m3g_matrix_identity(g_objects[i].projection_matrix);
            g_objects[i].image_mode_x = 32;
            g_objects[i].image_mode_y = 32;
            g_objects[i].color_clear_enabled = 1;
            g_objects[i].depth_clear_enabled = 1;
            g_objects[i].default_color = 0xffffffffU;
            g_objects[i].vb_position_scale_bias[0] = 1.0f;
            g_objects[i].vb_texcoord_scale_bias[0][0] = 1.0f;
            g_objects[i].vb_texcoord_scale_bias[1][0] = 1.0f;
            g_objects[i].texture_level_filter = 208;
            g_objects[i].texture_image_filter = 210;
            g_objects[i].texture_wrap_s = 241;
            g_objects[i].texture_wrap_t = 241;
            g_objects[i].texture_blending = 227;
            g_objects[i].material_ambient = 0xff333333U;
            g_objects[i].material_diffuse = 0xffccccccU;
            g_objects[i].material_emissive = 0xff000000U;
            g_objects[i].material_specular = 0xff000000U;
            g_objects[i].compositing_blending = 64;
            g_objects[i].compositing_alpha_write = 1;
            g_objects[i].compositing_depth_test = 1;
            g_objects[i].compositing_depth_write = 1;
            g_objects[i].compositing_color_write = 1;
            g_objects[i].anim_active_end = 0x7fffffff;
            g_objects[i].anim_speed = 1.0f;
            g_objects[i].anim_weight = 1.0f;
            g_objects[i].keyframe_repeat = 193;
            g_objects[i].polygon_culling = 160;
            g_objects[i].polygon_winding = 168;
            g_objects[i].polygon_shading = 165;
            g_objects[i].polygon_perspective_correction = 1;
            g_objects[i].fog_mode = 80;
            g_objects[i].fog_density = 1.0f;
            g_objects[i].fog_color = 0x000000U;
            g_objects[i].light_intensity = 1.0f;
            g_objects[i].light_color = 0x00ffffffU;
            g_objects[i].light_mode = 130;
            g_objects[i].light_spot_angle = 45.0f;
            g_objects[i].light_attenuation[0] = 1.0f;
            return i;
        }
    }
    return 0;
}

static void *
funkey_m3g_core_malloc(M3Gpointer bytes) {
    return malloc((size_t) bytes);
}

static void
funkey_m3g_core_free(void *ptr) {
    free(ptr);
}

static void
funkey_m3g_core_error(M3Genum error_code, M3GInterface interface) {
    (void) interface;
    if (error_code != M3G_NO_ERROR) {
        printf("[M3G core] error=%d\n", (int) error_code);
    }
}

static M3GInterface
funkey_m3g_ensure_core_interface(void) {
    M3Gparams params;
    if (g_core_interface != 0) {
        return g_core_interface;
    }
    memset(&params, 0, sizeof(params));
    params.mallocFunc = funkey_m3g_core_malloc;
    params.freeFunc = funkey_m3g_core_free;
    params.errorFunc = funkey_m3g_core_error;
    g_core_interface = m3gCreateInterface(&params);
    if (g_core_interface == 0) {
        printf("[M3G core] create interface failed\n");
    }
    return g_core_interface;
}

static M3GInterface
funkey_m3g_core_interface_from_handle(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core_interface != 0) {
        return obj->core_interface;
    }
    return funkey_m3g_ensure_core_interface();
}

static long
funkey_m3g_wrap_core_object(M3GObject core) {
    long handle;
    FunKeyM3GObject *obj;
    if (core == 0) {
        return 0;
    }
    handle = funkey_m3g_find_core_handle(core);
    if (handle != 0) {
        return handle;
    }
    handle = funkey_m3g_alloc_handle();
    obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->core = core;
        obj->core_interface = m3gGetObjectInterface(core);
        obj->class_id = (int) m3gGetClass(core);
        obj->user_id = m3gGetUserID(core);
        m3gAddRef(core);
    }
    return handle;
}

static M3GObject
funkey_m3g_core_object(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    return obj != 0 ? obj->core : 0;
}

static int
funkey_m3g_sync_core_image(FunKeyM3GObject *obj) {
    M3GInterface m3g;
    M3GImage image;
    M3Guint *scanline;
    int x;
    int y;
    if (obj == 0 || obj->class_id != FUNKEY_M3G_CLASS_IMAGE_2D ||
            obj->image_width <= 0 || obj->image_height <= 0) {
        return 0;
    }
    if (obj->image_pixels == 0 && obj->image_pixel_count > 0) {
        return 0;
    }
    m3g = obj->core_interface != 0 ? obj->core_interface :
          funkey_m3g_ensure_core_interface();
    if (m3g == 0) {
        return 0;
    }
    image = m3gCreateImage(m3g, (M3GImageFormat) obj->image_format,
                           obj->image_width, obj->image_height,
                           obj->image_mutable ? M3G_TRUE : M3G_FALSE);
    if (image == 0) {
        return 0;
    }
    if (obj->image_pixel_count > 0 && obj->image_pixels != 0) {
        scanline = (M3Guint *) malloc((size_t) obj->image_width *
                                      sizeof(M3Guint));
        if (scanline == 0) {
            m3gDeleteRef((M3GObject) image);
            return 0;
        }
        for (y = 0; y < obj->image_height; ++y) {
            for (x = 0; x < obj->image_width; ++x) {
                scanline[x] = funkey_m3g_image_argb(obj, x, y);
            }
            m3gSetImageScanline(image, y, M3G_TRUE, scanline);
        }
        free(scanline);
    }
    if (obj->core != 0) {
        m3gDeleteRef(obj->core);
    }
    obj->core = (M3GObject) image;
    obj->core_interface = m3g;
    return 1;
}

long
funkey_m3g_create_interface(void) {
    long handle = funkey_m3g_alloc_handle();
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->class_id = FUNKEY_M3G_CLASS_RENDER_CONTEXT;
        obj->core_interface = funkey_m3g_ensure_core_interface();
    }
    return handle;
}

void
funkey_m3g_finalize_interface(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        funkey_m3g_free_object(obj);
    }
}

long
funkey_m3g_create_loader(long interface_handle) {
    long handle = funkey_m3g_alloc_handle();
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GInterface m3g = funkey_m3g_core_interface_from_handle(interface_handle);
    if (obj != 0) {
        obj->class_id = FUNKEY_M3G_CLASS_LOADER;
        obj->core_interface = m3g;
        if (m3g != 0) {
            obj->core = (M3GObject) m3gCreateLoader(m3g);
        }
    }
    return handle;
}

long
funkey_m3g_create_object(int class_id) {
    long handle = funkey_m3g_alloc_handle();
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        M3GInterface m3g;
        obj->class_id = class_id;
        m3g = funkey_m3g_ensure_core_interface();
        obj->core_interface = m3g;
        if (m3g != 0) {
            switch (class_id) {
            case FUNKEY_M3G_CLASS_ANIMATION_CONTROLLER:
                obj->core = (M3GObject) m3gCreateAnimationController(m3g);
                break;
            case FUNKEY_M3G_CLASS_APPEARANCE:
                obj->core = (M3GObject) m3gCreateAppearance(m3g);
                break;
            case FUNKEY_M3G_CLASS_BACKGROUND:
                obj->core = (M3GObject) m3gCreateBackground(m3g);
                break;
            case FUNKEY_M3G_CLASS_CAMERA:
                obj->core = (M3GObject) m3gCreateCamera(m3g);
                break;
            case FUNKEY_M3G_CLASS_COMPOSITING_MODE:
                obj->core = (M3GObject) m3gCreateCompositingMode(m3g);
                break;
            case FUNKEY_M3G_CLASS_FOG:
                obj->core = (M3GObject) m3gCreateFog(m3g);
                break;
            case FUNKEY_M3G_CLASS_GROUP:
                obj->core = (M3GObject) m3gCreateGroup(m3g);
                break;
            case FUNKEY_M3G_CLASS_LIGHT:
                obj->core = (M3GObject) m3gCreateLight(m3g);
                break;
            case FUNKEY_M3G_CLASS_MATERIAL:
                obj->core = (M3GObject) m3gCreateMaterial(m3g);
                break;
            case FUNKEY_M3G_CLASS_POLYGON_MODE:
                obj->core = (M3GObject) m3gCreatePolygonMode(m3g);
                break;
            case FUNKEY_M3G_CLASS_RENDER_CONTEXT:
                obj->core = (M3GObject) m3gCreateContext(m3g);
                break;
            case FUNKEY_M3G_CLASS_VERTEX_BUFFER:
                obj->core = (M3GObject) m3gCreateVertexBuffer(m3g);
                break;
            case FUNKEY_M3G_CLASS_WORLD:
                obj->core = (M3GObject) m3gCreateWorld(m3g);
                break;
            default:
                break;
            }
        }
    }
    return handle;
}

void
funkey_m3g_add_ref(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->ref_count < 0x7fffffff) {
        ++obj->ref_count;
    }
}

void
funkey_m3g_release(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->ref_count > 0) {
        --obj->ref_count;
    }
    if (obj->ref_count == 0) {
        funkey_m3g_free_object(obj);
    }
}

int
funkey_m3g_get_class_id(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return (int) m3gGetClass(obj->core);
    }
    return obj->class_id;
}

void
funkey_m3g_set_user_id(long handle, int user_id) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->user_id = user_id;
        if (obj->core != 0) {
            m3gSetUserID(obj->core, user_id);
        }
    }
}

int
funkey_m3g_get_user_id(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gGetUserID(obj->core);
    }
    return obj->user_id;
}

long
funkey_m3g_find(long handle, int user_id) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    long found;
    int i;

    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return funkey_m3g_wrap_core_object(m3gFind(obj->core, user_id));
    }
    if (obj->user_id == user_id) {
        return handle;
    }
    if (obj->active_camera != 0) {
        found = funkey_m3g_find(obj->active_camera, user_id);
        if (found != 0) {
            return found;
        }
    }
    if (obj->background != 0) {
        found = funkey_m3g_find(obj->background, user_id);
        if (found != 0) {
            return found;
        }
    }
    if (obj->mesh_vertices != 0) {
        found = funkey_m3g_find(obj->mesh_vertices, user_id);
        if (found != 0) {
            return found;
        }
    }
    for (i = 0; i < obj->mesh_submesh_count; ++i) {
        found = funkey_m3g_find(obj->mesh_indices[i], user_id);
        if (found != 0) {
            return found;
        }
        found = funkey_m3g_find(obj->mesh_appearances[i], user_id);
        if (found != 0) {
            return found;
        }
    }
    if (obj->texture_image != 0) {
        found = funkey_m3g_find(obj->texture_image, user_id);
        if (found != 0) {
            return found;
        }
    }
    for (i = 0; i < FUNKEY_M3G_NUM_TEXTURE_UNITS; ++i) {
        found = funkey_m3g_find(obj->appearance_textures[i], user_id);
        if (found != 0) {
            return found;
        }
    }
    for (i = 0; i < obj->child_count; ++i) {
        found = funkey_m3g_find(obj->children[i], user_id);
        if (found != 0) {
            return found;
        }
    }
    /*
     * Loader fallback objects currently expose referenced controllers and
     * textures without a complete AnimationTrack/reference graph. Preserve
     * M3G find behavior for these referenced objects until native decoding
     * builds every edge directly.
     */
    for (i = 1; i < FUNKEY_M3G_MAX_OBJECTS; ++i) {
        if (g_objects[i].alive && g_objects[i].user_id == user_id) {
            return i;
        }
    }
    return 0;
}

int
funkey_m3g_loader_decode(long loader, int bytes, const unsigned char *data) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    if (obj == 0 || obj->core == 0 || data == 0 || bytes < 0) {
        return 0;
    }
    return m3gDecodeData((M3GLoader) obj->core, (M3Gsizei) bytes,
                         (const M3Gubyte *) data);
}

int
funkey_m3g_loader_get_loaded_objects(long loader, long *objects, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    M3Gulong core_objects[512];
    int total;
    int i;
    int out_count = 0;
    if (obj == 0 || obj->core == 0) {
        return 0;
    }
    total = m3gGetLoadedObjects((M3GLoader) obj->core, 0);
    if (total > (int) (sizeof(core_objects) / sizeof(core_objects[0]))) {
        total = (int) (sizeof(core_objects) / sizeof(core_objects[0]));
    }
    memset(core_objects, 0, sizeof(core_objects));
    m3gGetLoadedObjects((M3GLoader) obj->core, core_objects);
    for (i = 0; i < total; ++i) {
        M3GObject core_obj = (M3GObject) (uintptr_t) core_objects[i];
        int is_external = 0;
        int j;
        for (j = 0; j < obj->loader_external_ref_count; ++j) {
            if (obj->loader_external_refs[j] == core_obj) {
                is_external = 1;
                break;
            }
        }
        if (!is_external) {
            if (objects != 0 && count > 0 && out_count < count) {
                objects[out_count] = funkey_m3g_wrap_core_object(core_obj);
            }
            out_count++;
        }
    }
    return out_count;
}

void
funkey_m3g_loader_set_external_refs(long loader, const long *refs, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    M3Gulong core_refs[512];
    int i;
    if (obj == 0 || obj->core == 0 || refs == 0 || count <= 0) {
        return;
    }
    if (count > (int) (sizeof(core_refs) / sizeof(core_refs[0]))) {
        count = (int) (sizeof(core_refs) / sizeof(core_refs[0]));
    }
    obj->loader_external_ref_count = 0;
    for (i = 0; i < count; ++i) {
        M3GObject core_ref = funkey_m3g_core_object(refs[i]);
        if (core_ref == 0) {
            /*
             * External references came from the Java fallback parser. Mixing
             * those handles into m3gcore makes the upstream loader dereference
             * null refs, so keep this whole load on the Java fallback path.
             */
            fprintf(stderr, "[M3G Loader] external refs are fallback-only, disabling native loader\n");
            m3gDeleteRef(obj->core);
            obj->core = 0;
            return;
        }
        core_refs[i] = (M3Gulong) (uintptr_t) core_ref;
        obj->loader_external_refs[obj->loader_external_ref_count++] = core_ref;
    }
    m3gImportObjects((M3GLoader) obj->core, count, core_refs);
}

int
funkey_m3g_loader_get_objects_with_user_params(long loader, long *objects,
                                               int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    M3Gulong core_objects[512];
    int total;
    int i;
    if (obj == 0 || obj->core == 0) {
        return 0;
    }
    total = m3gGetObjectsWithUserParameters((M3GLoader) obj->core, 0);
    if (objects == 0 || count <= 0) {
        return total;
    }
    if (total > (int) (sizeof(core_objects) / sizeof(core_objects[0]))) {
        total = (int) (sizeof(core_objects) / sizeof(core_objects[0]));
    }
    if (total > count) {
        total = count;
    }
    memset(core_objects, 0, sizeof(core_objects));
    m3gGetObjectsWithUserParameters((M3GLoader) obj->core, core_objects);
    for (i = 0; i < total; ++i) {
        objects[i] = funkey_m3g_wrap_core_object((M3GObject) (uintptr_t) core_objects[i]);
    }
    return total;
}

int
funkey_m3g_loader_get_num_user_params(long loader, int object_index) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    if (obj == 0 || obj->core == 0) {
        return 0;
    }
    return m3gGetNumUserParameters((M3GLoader) obj->core, object_index);
}

int
funkey_m3g_loader_get_user_param(long loader, int object_index, int index,
                                 signed char *data, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(loader);
    (void) count;
    if (obj == 0 || obj->core == 0) {
        return 0;
    }
    return m3gGetUserParameter((M3GLoader) obj->core, object_index, index,
                               (M3Gbyte *) data);
}

static int
funkey_m3g_is_node_class(int class_id) {
    switch (class_id) {
    case FUNKEY_M3G_CLASS_CAMERA:
    case FUNKEY_M3G_CLASS_GROUP:
    case FUNKEY_M3G_CLASS_LIGHT:
    case FUNKEY_M3G_CLASS_MESH:
    case FUNKEY_M3G_CLASS_MORPHING_MESH:
    case FUNKEY_M3G_CLASS_SKINNED_MESH:
    case FUNKEY_M3G_CLASS_SPRITE_3D:
    case FUNKEY_M3G_CLASS_WORLD:
        return 1;
    default:
        return 0;
    }
}

long
funkey_m3g_duplicate(long handle, long *pairs, int max_pairs,
                     int *pair_count) {
    FunKeyM3GObject *src = funkey_m3g_object(handle);
    FunKeyM3GObject *dst;
    long clone;
    int i;

    if (pair_count != 0 && *pair_count < 0) {
        *pair_count = 0;
    }
    if (src == 0) {
        return 0;
    }
    if (src->core != 0) {
        int native_pair_count = 1;
        int copy_count;
        M3Gulong *refs;
        M3GObject cloned;

        if (funkey_m3g_is_node_class(src->class_id)) {
            native_pair_count = m3gGetSubtreeSize((M3GNode) src->core);
            if (native_pair_count < 1) {
                native_pair_count = 1;
            }
        }
        refs = (M3Gulong *) calloc((size_t) native_pair_count * 2,
                                   sizeof(M3Gulong));
        if (refs == 0) {
            return 0;
        }
        cloned = m3gDuplicate(src->core, refs);
        clone = funkey_m3g_wrap_core_object(cloned);
        if (cloned != 0 && pairs != 0 && pair_count != 0 &&
                *pair_count < max_pairs) {
            copy_count = native_pair_count;
            if (copy_count > max_pairs - *pair_count) {
                copy_count = max_pairs - *pair_count;
            }
            for (i = 0; i < copy_count; ++i) {
                pairs[(*pair_count + i) * 2] =
                    funkey_m3g_wrap_core_object((M3GObject)
                                                 (uintptr_t) refs[i * 2]);
                pairs[(*pair_count + i) * 2 + 1] =
                    funkey_m3g_wrap_core_object((M3GObject)
                                                 (uintptr_t) refs[i * 2 + 1]);
            }
            *pair_count += copy_count;
        }
        free(refs);
        return clone;
    }

    clone = funkey_m3g_alloc_handle();
    dst = funkey_m3g_object(clone);
    if (dst == 0) {
        return 0;
    }

    *dst = *src;
    dst->alive = 1;
    dst->ref_count = 1;
    dst->parent = 0;
    dst->components = 0;
    dst->indices = 0;
    dst->image_pixels = 0;
    dst->image_palette = 0;
    dst->keyframe_times = 0;
    dst->keyframe_values = 0;

    if (src->components != 0 && src->vertex_count > 0 && src->component_count > 0) {
        int count = src->vertex_count * src->component_count;
        dst->components = (int *) malloc((size_t) count * sizeof(int));
        if (dst->components != 0) {
            memcpy(dst->components, src->components, (size_t) count * sizeof(int));
        }
    }
    if (src->indices != 0 && src->index_count > 0) {
        dst->indices = (int *) malloc((size_t) src->index_count * sizeof(int));
        if (dst->indices != 0) {
            memcpy(dst->indices, src->indices, (size_t) src->index_count * sizeof(int));
        }
    }
    if (src->image_pixels != 0 && src->image_pixel_count > 0) {
        dst->image_pixels = (unsigned char *) malloc((size_t) src->image_pixel_count);
        if (dst->image_pixels != 0) {
            memcpy(dst->image_pixels, src->image_pixels, (size_t) src->image_pixel_count);
        }
    }
    if (src->image_palette != 0 && src->image_palette_count > 0) {
        dst->image_palette = (unsigned char *) malloc((size_t) src->image_palette_count);
        if (dst->image_palette != 0) {
            memcpy(dst->image_palette, src->image_palette,
                   (size_t) src->image_palette_count);
        }
    }
    if (src->keyframe_times != 0 && src->keyframe_count > 0) {
        dst->keyframe_times = (int *) malloc((size_t) src->keyframe_count * sizeof(int));
        if (dst->keyframe_times != 0) {
            memcpy(dst->keyframe_times, src->keyframe_times,
                   (size_t) src->keyframe_count * sizeof(int));
        }
    }
    if (src->keyframe_values != 0 && src->keyframe_count > 0 &&
            src->keyframe_components > 0) {
        int count = src->keyframe_count * src->keyframe_components;
        dst->keyframe_values = (float *) malloc((size_t) count * sizeof(float));
        if (dst->keyframe_values != 0) {
            memcpy(dst->keyframe_values, src->keyframe_values,
                   (size_t) count * sizeof(float));
        }
    }

    if (pairs != 0 && pair_count != 0 && *pair_count < max_pairs) {
        pairs[*pair_count * 2] = handle;
        pairs[*pair_count * 2 + 1] = clone;
    }
    if (pair_count != 0) {
        ++(*pair_count);
    }

    dst->child_count = 0;
    for (i = 0; i < src->child_count && i < FUNKEY_M3G_MAX_CHILDREN; ++i) {
        long child_clone = funkey_m3g_duplicate(src->children[i], pairs,
                                                max_pairs, pair_count);
        if (child_clone != 0) {
            funkey_m3g_group_add_child(clone, child_clone);
        }
    }
    return clone;
}

static void
funkey_m3g_reference_append(long ref, long *refs, int count, int *total) {
    if (ref == 0 || total == 0) {
        return;
    }
    if (refs != 0 && *total < count) {
        refs[*total] = ref;
    }
    ++(*total);
}

int
funkey_m3g_object_get_references(long handle, long *refs, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int total = 0;
    int i;
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        int core_count = m3gGetReferences(obj->core, 0, 0);
        M3Gulong *core_refs;
        if (refs == 0 || count <= 0) {
            return core_count;
        }
        if (count < core_count) {
            return core_count;
        }
        core_refs = core_count > 0 ?
            (M3Gulong *) calloc((size_t) core_count, sizeof(M3Gulong)) : 0;
        if (core_count > 0 && core_refs == 0) {
            return 0;
        }
        m3gGetReferences(obj->core, core_refs, core_count);
        for (i = 0; i < core_count; ++i) {
            refs[i] = funkey_m3g_wrap_core_object((M3GObject)(uintptr_t)core_refs[i]);
        }
        if (core_refs != 0) {
            free(core_refs);
        }
        return core_count;
    }
    funkey_m3g_reference_append(obj->active_camera, refs, count, &total);
    funkey_m3g_reference_append(obj->background, refs, count, &total);
    funkey_m3g_reference_append(obj->background_image, refs, count, &total);
    funkey_m3g_reference_append(obj->vb_positions, refs, count, &total);
    funkey_m3g_reference_append(obj->vb_normals, refs, count, &total);
    funkey_m3g_reference_append(obj->vb_colors, refs, count, &total);
    for (i = 0; i < FUNKEY_M3G_NUM_TEXTURE_UNITS; ++i) {
        funkey_m3g_reference_append(obj->vb_texcoords[i], refs, count, &total);
        funkey_m3g_reference_append(obj->appearance_textures[i], refs, count, &total);
    }
    funkey_m3g_reference_append(obj->mesh_vertices, refs, count, &total);
    for (i = 0; i < obj->mesh_submesh_count; ++i) {
        funkey_m3g_reference_append(obj->mesh_indices[i], refs, count, &total);
        funkey_m3g_reference_append(obj->mesh_appearances[i], refs, count, &total);
    }
    for (i = 0; i < obj->morph_target_count; ++i) {
        funkey_m3g_reference_append(obj->morph_targets[i], refs, count, &total);
    }
    funkey_m3g_reference_append(obj->skin_skeleton, refs, count, &total);
    funkey_m3g_reference_append(obj->sprite_image, refs, count, &total);
    funkey_m3g_reference_append(obj->sprite_appearance, refs, count, &total);
    funkey_m3g_reference_append(obj->texture_image, refs, count, &total);
    funkey_m3g_reference_append(obj->appearance_compositing, refs, count, &total);
    funkey_m3g_reference_append(obj->appearance_fog, refs, count, &total);
    funkey_m3g_reference_append(obj->appearance_material, refs, count, &total);
    funkey_m3g_reference_append(obj->appearance_polygon, refs, count, &total);
    funkey_m3g_reference_append(obj->track_sequence, refs, count, &total);
    funkey_m3g_reference_append(obj->track_controller, refs, count, &total);
    for (i = 0; i < obj->animation_track_count; ++i) {
        funkey_m3g_reference_append(obj->animation_tracks[i], refs, count, &total);
    }
    for (i = 0; i < obj->child_count; ++i) {
        funkey_m3g_reference_append(obj->children[i], refs, count, &total);
    }
    return total;
}

void
funkey_m3g_group_add_child(long group, long child) {
    FunKeyM3GObject *g = funkey_m3g_object(group);
    FunKeyM3GObject *c = funkey_m3g_object(child);
    M3GNode core_parent = 0;
    int i;
    if (g == 0 || c == 0) {
        return;
    }
    if (g->core != 0 && c->core != 0) {
        core_parent = m3gGetParent((M3GNode) c->core);
        if (core_parent != 0 && core_parent != (M3GNode) g->core) {
            fprintf(stderr, "[M3G graph] repair reparent child=%ld old=%p new=%p\n",
                    child, (void *) core_parent, (void *) g->core);
            m3gRemoveChild((M3GGroup) core_parent, (M3GNode) c->core);
        }
        if (m3gGetParent((M3GNode) c->core) != (M3GNode) g->core) {
            m3gAddChild((M3GGroup) g->core, (M3GNode) c->core);
        }
        if (m3gGetParent((M3GNode) c->core) != (M3GNode) g->core) {
            fprintf(stderr, "[M3G graph] rejected add child=%ld group=%ld\n",
                    child, group);
            return;
        }
    } else if (g->child_count >= FUNKEY_M3G_MAX_CHILDREN) {
        return;
    }
    for (i = 0; i < g->child_count; ++i) {
        if (g->children[i] == child) {
            c->parent = group;
            return;
        }
    }
    if (g->child_count < FUNKEY_M3G_MAX_CHILDREN) {
        g->children[g->child_count++] = child;
    }
    c->parent = group;
}

void
funkey_m3g_group_remove_child(long group, long child) {
    FunKeyM3GObject *g = funkey_m3g_object(group);
    FunKeyM3GObject *c = funkey_m3g_object(child);
    int i;
    if (g == 0) {
        return;
    }
    if (g->core != 0 && c != 0 && c->core != 0) {
        m3gRemoveChild((M3GGroup) g->core, (M3GNode) c->core);
    }
    for (i = 0; i < g->child_count; ++i) {
        if (g->children[i] == child) {
            memmove(&g->children[i], &g->children[i + 1],
                    (g->child_count - i - 1) * sizeof(g->children[0]));
            --g->child_count;
            if (c != 0 && c->parent == group) {
                c->parent = 0;
            }
            return;
        }
    }
}

int
funkey_m3g_group_get_child_count(long group) {
    FunKeyM3GObject *g = funkey_m3g_object(group);
    if (g != 0 && g->core != 0) {
        return m3gGetChildCount((M3GGroup) g->core);
    }
    return g != 0 ? g->child_count : 0;
}

long
funkey_m3g_group_get_child(long group, int index) {
    FunKeyM3GObject *g = funkey_m3g_object(group);
    if (g != 0 && g->core != 0) {
        if (index < 0 || index >= m3gGetChildCount((M3GGroup) g->core)) {
            return 0;
        }
        return funkey_m3g_wrap_core_object((M3GObject) m3gGetChild((M3GGroup) g->core, index));
    }
    if (g == 0 || index < 0 || index >= g->child_count) {
        return 0;
    }
    return g->children[index];
}

long
funkey_m3g_group_pick3d(long group, int mask, float *ray, float *result) {
    FunKeyM3GObject *obj = funkey_m3g_object(group);
    M3GNode picked;
    if (obj == 0 || obj->core == 0 || ray == 0 || result == 0) {
        return 0;
    }
    picked = m3gPick3D((M3GGroup) obj->core, mask, ray, result);
    return funkey_m3g_wrap_core_object((M3GObject) picked);
}

long
funkey_m3g_group_pick2d(long group, int mask, float x, float y,
                        long camera, float *result) {
    FunKeyM3GObject *obj = funkey_m3g_object(group);
    M3GObject core_camera = funkey_m3g_core_object(camera);
    M3GNode picked;
    if (obj == 0 || obj->core == 0 || core_camera == 0 || result == 0) {
        return 0;
    }
    picked = m3gPick2D((M3GGroup) obj->core, mask, x, y,
                       (M3GCamera) core_camera, result);
    return funkey_m3g_wrap_core_object((M3GObject) picked);
}

long
funkey_m3g_node_get_parent(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject) m3gGetParent((M3GNode) obj->core));
    }
    return obj != 0 ? obj->parent : 0;
}

int
funkey_m3g_node_get_transform_to(long handle, long target, float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GObject core_target = funkey_m3g_core_object(target);
    M3GMatrix core_matrix;
    if (obj == 0 || obj->core == 0 || core_target == 0) {
        return 0;
    }
    if (!m3gGetTransformTo((M3GNode) obj->core, (M3GNode) core_target,
                           &core_matrix)) {
        return 0;
    }
    if (matrix != 0) {
        m3gGetMatrixRows(&core_matrix, matrix);
    }
    return 1;
}

void
funkey_m3g_node_align(long handle, long reference) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GObject core_ref = funkey_m3g_core_object(reference);
    if (obj != 0 && obj->core != 0) {
        m3gAlignNode((M3GNode) obj->core, (M3GNode) core_ref);
    }
}

void
funkey_m3g_node_set_alignment(long handle, long z_ref, int z_target,
                              long y_ref, int y_target) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GObject core_z = funkey_m3g_core_object(z_ref);
    M3GObject core_y = funkey_m3g_core_object(y_ref);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        m3gSetAlignment((M3GNode) obj->core, (M3GNode) core_z,
                        z_target, (M3GNode) core_y, y_target);
    }
    obj->z_ref = z_ref;
    obj->y_ref = y_ref;
}

void
funkey_m3g_node_set_parent(long handle, long parent) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->parent = parent;
    }
}

void
funkey_m3g_node_set_scope(long handle, int scope) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->scope = scope;
        if (obj->core != 0) {
            m3gSetScope((M3GNode) obj->core, scope);
        }
    }
}

int
funkey_m3g_node_get_scope(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetScope((M3GNode) obj->core);
    }
    return obj != 0 ? obj->scope : 0;
}

void
funkey_m3g_node_set_alpha(long handle, float alpha) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        obj->alpha = alpha;
        if (obj->core != 0) {
            m3gSetAlphaFactor((M3GNode) obj->core, alpha);
        }
    }
}

float
funkey_m3g_node_get_alpha(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetAlphaFactor((M3GNode) obj->core);
    }
    return obj != 0 ? obj->alpha : 1.0f;
}

void
funkey_m3g_node_enable(long handle, int which, int enable) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        m3gEnable((M3GNode) obj->core, which, enable ? M3G_TRUE : M3G_FALSE);
    }
    if (which == 0) {
        obj->rendering_enabled = enable != 0;
    } else if (which == 1) {
        obj->picking_enabled = enable != 0;
    }
}

int
funkey_m3g_node_is_enabled(long handle, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gIsEnabled((M3GNode) obj->core, which) != 0;
    }
    if (which == 0) {
        return obj->rendering_enabled;
    }
    if (which == 1) {
        return obj->picking_enabled;
    }
    return 0;
}

long
funkey_m3g_node_get_z_ref(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetZRef((M3GNode) obj->core));
    }
    return obj != 0 ? obj->z_ref : 0;
}

long
funkey_m3g_node_get_y_ref(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetYRef((M3GNode) obj->core));
    }
    return obj != 0 ? obj->y_ref : 0;
}

int
funkey_m3g_node_get_subtree_size(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int total = 1;
    int i;
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gGetSubtreeSize((M3GNode) obj->core);
    }
    for (i = 0; i < obj->child_count; ++i) {
        total += funkey_m3g_node_get_subtree_size(obj->children[i]);
    }
    return total;
}

int
funkey_m3g_node_get_alignment_target(long handle, int axis) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetAlignmentTarget((M3GNode) obj->core, axis);
    }
    return 144;
}

void
funkey_m3g_transform_set_orientation(long handle, float angle,
                                     float ax, float ay, float az,
                                     int absolute) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (absolute) {
            m3gSetOrientation((M3GTransformable) obj->core,
                              angle, ax, ay, az);
        } else {
            m3gPostRotate((M3GTransformable) obj->core,
                          angle, ax, ay, az);
        }
    }
    if (absolute) {
        obj->orientation[0] = angle;
        obj->orientation[1] = ax;
        obj->orientation[2] = ay;
        obj->orientation[3] = az;
    } else {
        obj->orientation[0] += angle;
        obj->orientation[1] = ax;
        obj->orientation[2] = ay;
        obj->orientation[3] = az;
    }
}

void
funkey_m3g_transform_pre_rotate(long handle, float angle,
                                float ax, float ay, float az) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        m3gPreRotate((M3GTransformable) obj->core, angle, ax, ay, az);
    }
    obj->orientation[0] += angle;
    obj->orientation[1] = ax;
    obj->orientation[2] = ay;
    obj->orientation[3] = az;
}

void
funkey_m3g_transform_get_orientation(long handle, float *values, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int i;
    if (values == 0 || count <= 0) {
        return;
    }
    if (obj != 0 && obj->core != 0) {
        float orientation[4];
        m3gGetOrientation((M3GTransformable) obj->core, orientation);
        for (i = 0; i < count && i < 4; ++i) {
            values[i] = orientation[i];
        }
        return;
    }
    for (i = 0; i < count && i < 4; ++i) {
        values[i] = obj != 0 ? obj->orientation[i] : (i == 3 ? 1.0f : 0.0f);
    }
}

void
funkey_m3g_transform_set_scale(long handle, float sx, float sy, float sz,
                               int absolute) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (absolute) {
            m3gSetScale((M3GTransformable) obj->core, sx, sy, sz);
        } else {
            m3gScale((M3GTransformable) obj->core, sx, sy, sz);
        }
    }
    if (absolute) {
        obj->scale[0] = sx;
        obj->scale[1] = sy;
        obj->scale[2] = sz;
    } else {
        obj->scale[0] *= sx;
        obj->scale[1] *= sy;
        obj->scale[2] *= sz;
    }
}

void
funkey_m3g_transform_get_scale(long handle, float *values, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int i;
    if (values == 0 || count <= 0) {
        return;
    }
    if (obj != 0 && obj->core != 0) {
        float scale[3];
        m3gGetScale((M3GTransformable) obj->core, scale);
        for (i = 0; i < count && i < 3; ++i) {
            values[i] = scale[i];
        }
        return;
    }
    for (i = 0; i < count && i < 3; ++i) {
        values[i] = obj != 0 ? obj->scale[i] : 1.0f;
    }
}

void
funkey_m3g_transform_set_translation(long handle,
                                     float tx, float ty, float tz,
                                     int absolute) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (absolute) {
            m3gSetTranslation((M3GTransformable) obj->core, tx, ty, tz);
        } else {
            m3gTranslate((M3GTransformable) obj->core, tx, ty, tz);
        }
    }
    if (absolute) {
        obj->translation[0] = tx;
        obj->translation[1] = ty;
        obj->translation[2] = tz;
    } else {
        obj->translation[0] += tx;
        obj->translation[1] += ty;
        obj->translation[2] += tz;
    }
}

void
funkey_m3g_transform_get_translation(long handle, float *values, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int i;
    if (values == 0 || count <= 0) {
        return;
    }
    if (obj != 0 && obj->core != 0) {
        float translation[3];
        m3gGetTranslation((M3GTransformable) obj->core, translation);
        for (i = 0; i < count && i < 3; ++i) {
            values[i] = translation[i];
        }
        return;
    }
    for (i = 0; i < count && i < 3; ++i) {
        values[i] = obj != 0 ? obj->translation[i] : 0.0f;
    }
}

void
funkey_m3g_transform_set_matrix(long handle, const float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && matrix != 0) {
        if (obj->core != 0) {
            M3GMatrix core_matrix;
            m3gSetMatrixRows(&core_matrix, matrix);
            m3gSetTransform((M3GTransformable) obj->core, &core_matrix);
        }
        memcpy(obj->transform, matrix, 16 * sizeof(float));
    }
}

void
funkey_m3g_transform_get_matrix(long handle, float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int i;
    if (matrix == 0) {
        return;
    }
    if (obj != 0 && obj->core != 0) {
        M3GMatrix core_matrix;
        m3gGetTransform((M3GTransformable) obj->core, &core_matrix);
        m3gGetMatrixRows(&core_matrix, matrix);
        return;
    }
    if (obj != 0) {
        memcpy(matrix, obj->transform, 16 * sizeof(float));
        matrix[12] += obj->translation[0];
        matrix[13] += obj->translation[1];
        matrix[14] += obj->translation[2];
    } else {
        for (i = 0; i < 16; ++i) {
            matrix[i] = 0.0f;
        }
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    }
}

void
funkey_m3g_transform_get_composite(long handle, float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (matrix == 0) {
        return;
    }
    if (obj != 0 && obj->core != 0) {
        M3GMatrix core_matrix;
        m3gGetCompositeTransform((M3GTransformable) obj->core, &core_matrix);
        m3gGetMatrixRows(&core_matrix, matrix);
        return;
    }
    funkey_m3g_transform_get_matrix(handle, matrix);
}

void
funkey_m3g_anim_set_active_interval(long handle, int start, int end) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetActiveInterval((M3GAnimationController) obj->core,
                                 start, end);
        }
        obj->anim_active_start = start;
        obj->anim_active_end = end;
    }
}

int
funkey_m3g_anim_get_active_interval(long handle, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0) {
        return which == 0 ? 0 : 0x7fffffff;
    }
    if (obj->core != 0) {
        return which == 0 ?
               m3gGetActiveIntervalStart((M3GAnimationController) obj->core) :
               m3gGetActiveIntervalEnd((M3GAnimationController) obj->core);
    }
    return which == 0 ? obj->anim_active_start : obj->anim_active_end;
}

void
funkey_m3g_anim_set_position(long handle, float position, int world_time) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetPosition((M3GAnimationController) obj->core,
                           position, world_time);
        }
        obj->anim_position = position;
        obj->anim_ref_world_time = world_time;
    }
}

float
funkey_m3g_anim_get_position(long handle, int world_time) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetPosition((M3GAnimationController) obj->core, world_time);
    }
    return obj != 0 ? obj->anim_position : 0.0f;
}

int
funkey_m3g_anim_get_ref_world_time(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetRefWorldTime((M3GAnimationController) obj->core);
    }
    return obj != 0 ? obj->anim_ref_world_time : 0;
}

void
funkey_m3g_anim_set_speed(long handle, float speed, int world_time) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetSpeed((M3GAnimationController) obj->core, speed, world_time);
        }
        obj->anim_speed = speed;
        obj->anim_ref_world_time = world_time;
    }
}

float
funkey_m3g_anim_get_speed(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetSpeed((M3GAnimationController) obj->core);
    }
    return obj != 0 ? obj->anim_speed : 1.0f;
}

void
funkey_m3g_anim_set_weight(long handle, float weight) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetWeight((M3GAnimationController) obj->core, weight);
        }
        obj->anim_weight = weight;
    }
}

float
funkey_m3g_anim_get_weight(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        return m3gGetWeight((M3GAnimationController) obj->core);
    }
    return obj != 0 ? obj->anim_weight : 1.0f;
}

int
funkey_m3g_object_add_animation_track(long object, long track) {
    FunKeyM3GObject *obj = funkey_m3g_object(object);
    M3GObject core_track = funkey_m3g_core_object(track);
    int i;
    if (obj == 0 || funkey_m3g_object(track) == 0) {
        return 0;
    }
    if (obj->core != 0 && core_track != 0) {
        return m3gAddAnimationTrack(obj->core, (M3GAnimationTrack) core_track);
    }
    for (i = 0; i < obj->animation_track_count; ++i) {
        if (obj->animation_tracks[i] == track) {
            return i;
        }
    }
    if (obj->animation_track_count >= FUNKEY_M3G_MAX_ANIMATION_TRACKS) {
        return -1;
    }
    obj->animation_tracks[obj->animation_track_count] = track;
    return obj->animation_track_count++;
}

void
funkey_m3g_object_remove_animation_track(long object, long track) {
    FunKeyM3GObject *obj = funkey_m3g_object(object);
    M3GObject core_track = funkey_m3g_core_object(track);
    int i;
    if (obj == 0) {
        return;
    }
    if (obj->core != 0 && core_track != 0) {
        m3gRemoveAnimationTrack(obj->core, (M3GAnimationTrack) core_track);
        return;
    }
    for (i = 0; i < obj->animation_track_count; ++i) {
        if (obj->animation_tracks[i] == track) {
            memmove(&obj->animation_tracks[i], &obj->animation_tracks[i + 1],
                    (size_t) (obj->animation_track_count - i - 1) *
                    sizeof(obj->animation_tracks[0]));
            --obj->animation_track_count;
            return;
        }
    }
}

int
funkey_m3g_object_get_animation_track_count(long object) {
    FunKeyM3GObject *obj = funkey_m3g_object(object);
    if (obj != 0 && obj->core != 0) {
        return m3gGetAnimationTrackCount(obj->core);
    }
    return obj != 0 ? obj->animation_track_count : 0;
}

long
funkey_m3g_object_get_animation_track(long object, int index) {
    FunKeyM3GObject *obj = funkey_m3g_object(object);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetAnimationTrack(obj->core, index));
    }
    if (obj == 0 || index < 0 || index >= obj->animation_track_count) {
        return 0;
    }
    return obj->animation_tracks[index];
}

void
funkey_m3g_animation_track_init(long track, long sequence, int property) {
    FunKeyM3GObject *obj = funkey_m3g_object(track);
    if (obj != 0) {
        M3GObject core_sequence = funkey_m3g_core_object(sequence);
        if (obj->core == 0 && obj->core_interface != 0 &&
                core_sequence != 0) {
            obj->core = (M3GObject)
                m3gCreateAnimationTrack(obj->core_interface,
                                        (M3GKeyframeSequence) core_sequence,
                                        property);
        }
        obj->track_sequence = sequence;
        obj->track_property = property;
    }
}

void
funkey_m3g_animation_track_set_controller(long track, long controller) {
    FunKeyM3GObject *obj = funkey_m3g_object(track);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetController((M3GAnimationTrack) obj->core,
                             (M3GAnimationController)
                             funkey_m3g_core_object(controller));
        }
        obj->track_controller = controller;
    }
}

long
funkey_m3g_animation_track_get_controller(long track) {
    FunKeyM3GObject *obj = funkey_m3g_object(track);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetController((M3GAnimationTrack)
                                                            obj->core));
    }
    return obj != 0 ? obj->track_controller : 0;
}

long
funkey_m3g_animation_track_get_sequence(long track) {
    FunKeyM3GObject *obj = funkey_m3g_object(track);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetSequence((M3GAnimationTrack)
                                                          obj->core));
    }
    return obj != 0 ? obj->track_sequence : 0;
}

int
funkey_m3g_animation_track_get_property(long track) {
    FunKeyM3GObject *obj = funkey_m3g_object(track);
    if (obj != 0 && obj->core != 0) {
        return m3gGetTargetProperty((M3GAnimationTrack) obj->core);
    }
    return obj != 0 ? obj->track_property : 0;
}

int
funkey_m3g_keyframe_init(long sequence, int keyframes, int components,
                         int interpolation) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj == 0 || keyframes <= 0 || components <= 0) {
        return 0;
    }
    if (obj->core == 0 && obj->core_interface != 0) {
        obj->core = (M3GObject)
            m3gCreateKeyframeSequence(obj->core_interface, keyframes,
                                      components, interpolation);
    }
    if (obj->keyframe_times != 0) {
        free(obj->keyframe_times);
    }
    if (obj->keyframe_values != 0) {
        free(obj->keyframe_values);
    }
    obj->keyframe_times = (int *) calloc((size_t) keyframes, sizeof(int));
    obj->keyframe_values = (float *) calloc((size_t) keyframes * components,
                                            sizeof(float));
    if (obj->keyframe_times == 0 || obj->keyframe_values == 0) {
        if (obj->keyframe_times != 0) {
            free(obj->keyframe_times);
        }
        if (obj->keyframe_values != 0) {
            free(obj->keyframe_values);
        }
        obj->keyframe_times = 0;
        obj->keyframe_values = 0;
        return 0;
    }
    obj->keyframe_count = keyframes;
    obj->keyframe_components = components;
    obj->keyframe_interpolation = interpolation;
    obj->keyframe_valid_first = 0;
    obj->keyframe_valid_last = keyframes - 1;
    return 1;
}

void
funkey_m3g_keyframe_set(long sequence, int index, int time,
                        const float *values, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj == 0 || index < 0 || index >= obj->keyframe_count ||
            values == 0 || obj->keyframe_values == 0) {
        return;
    }
    if (count > obj->keyframe_components) {
        count = obj->keyframe_components;
    }
    if (obj->core != 0 && count >= obj->keyframe_components) {
        m3gSetKeyframe((M3GKeyframeSequence) obj->core, index, time,
                       obj->keyframe_components, values);
    }
    obj->keyframe_times[index] = time;
    memcpy(&obj->keyframe_values[index * obj->keyframe_components], values,
           (size_t) count * sizeof(float));
}

int
funkey_m3g_keyframe_get(long sequence, int index, float *values, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj == 0 || index < 0 || index >= obj->keyframe_count) {
        return 0;
    }
    if (obj->core != 0) {
        if (values != 0 && count >= obj->keyframe_components) {
            return m3gGetKeyframe((M3GKeyframeSequence) obj->core,
                                  index, values);
        }
        return m3gGetKeyframe((M3GKeyframeSequence) obj->core, index, 0);
    }
    if (values != 0) {
        if (count > obj->keyframe_components) {
            count = obj->keyframe_components;
        }
        memcpy(values, &obj->keyframe_values[index * obj->keyframe_components],
               (size_t) count * sizeof(float));
    }
    return obj->keyframe_times[index];
}

void
funkey_m3g_keyframe_set_duration(long sequence, int duration) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetDuration((M3GKeyframeSequence) obj->core, duration);
        }
        obj->keyframe_duration = duration;
    }
}

void
funkey_m3g_keyframe_set_repeat(long sequence, int repeat) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetRepeatMode((M3GKeyframeSequence) obj->core, repeat);
        }
        obj->keyframe_repeat = repeat;
    }
}

void
funkey_m3g_keyframe_set_valid_range(long sequence, int first, int last) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetValidRange((M3GKeyframeSequence) obj->core, first, last);
        }
        obj->keyframe_valid_first = first;
        obj->keyframe_valid_last = last;
    }
}

int
funkey_m3g_keyframe_get_int(long sequence, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(sequence);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        M3Gint first;
        M3Gint last;
        if (which == 0) return m3gGetComponentCount((M3GKeyframeSequence) obj->core);
        if (which == 1) return m3gGetDuration((M3GKeyframeSequence) obj->core);
        if (which == 2) return m3gGetInterpolationType((M3GKeyframeSequence) obj->core);
        if (which == 3) return m3gGetKeyframeCount((M3GKeyframeSequence) obj->core);
        if (which == 4) return m3gGetRepeatMode((M3GKeyframeSequence) obj->core);
        m3gGetValidRange((M3GKeyframeSequence) obj->core, &first, &last);
        return which == 5 ? first : last;
    }
    if (which == 0) return obj->keyframe_components;
    if (which == 1) return obj->keyframe_duration;
    if (which == 2) return obj->keyframe_interpolation;
    if (which == 3) return obj->keyframe_count;
    if (which == 4) return obj->keyframe_repeat;
    if (which == 5) return obj->keyframe_valid_first;
    return obj->keyframe_valid_last;
}

static int
funkey_m3g_keyframe_sample(FunKeyM3GObject *sequence, float sequence_time,
                           float *values, int max_values) {
    int first;
    int last;
    int i;
    int components;
    float t;
    if (sequence == 0 || sequence->keyframe_values == 0 ||
            sequence->keyframe_count <= 0 || values == 0) {
        return 0;
    }
    first = sequence->keyframe_valid_first;
    last = sequence->keyframe_valid_last;
    if (first < 0) first = 0;
    if (last >= sequence->keyframe_count) last = sequence->keyframe_count - 1;
    if (last < first) last = first;
    if (sequence->keyframe_repeat == 193 && sequence->keyframe_duration > 0) {
        sequence_time = fmodf(sequence_time, (float) sequence->keyframe_duration);
        if (sequence_time < 0.0f) {
            sequence_time += (float) sequence->keyframe_duration;
        }
    }
    if (sequence_time <= sequence->keyframe_times[first]) {
        i = first;
        t = 0.0f;
    } else {
        i = first;
        while (i < last && sequence_time > sequence->keyframe_times[i + 1]) {
            ++i;
        }
        if (i >= last) {
            i = last;
            t = 0.0f;
        } else if (sequence->keyframe_interpolation == 180) {
            t = 0.0f;
        } else {
            int span = sequence->keyframe_times[i + 1] - sequence->keyframe_times[i];
            t = span > 0 ?
                (sequence_time - sequence->keyframe_times[i]) / (float) span :
                0.0f;
        }
    }
    components = sequence->keyframe_components;
    if (components > max_values) components = max_values;
    if (i == last || t == 0.0f) {
        memcpy(values, &sequence->keyframe_values[i * sequence->keyframe_components],
               (size_t) components * sizeof(float));
    } else {
        int c;
        for (c = 0; c < components; ++c) {
            float a = sequence->keyframe_values[i * sequence->keyframe_components + c];
            float b = sequence->keyframe_values[(i + 1) * sequence->keyframe_components + c];
            values[c] = a + (b - a) * t;
        }
    }
    return components;
}

static void
funkey_m3g_apply_animation_track(FunKeyM3GObject *target, long track_handle,
                                 int world_time) {
    FunKeyM3GObject *track = funkey_m3g_object(track_handle);
    FunKeyM3GObject *controller;
    FunKeyM3GObject *sequence;
    float values[16];
    float time;
    int components;
    if (target == 0 || track == 0) {
        return;
    }
    controller = funkey_m3g_object(track->track_controller);
    sequence = funkey_m3g_object(track->track_sequence);
    if (controller == 0 || sequence == 0 ||
            world_time < controller->anim_active_start ||
            world_time > controller->anim_active_end ||
            controller->anim_weight <= 0.0f) {
        return;
    }
    time = controller->anim_position +
           controller->anim_speed * (float) (world_time - controller->anim_ref_world_time);
    components = funkey_m3g_keyframe_sample(sequence, time, values, 16);
    if (track->track_property == 275 && components >= 3) {
        target->translation[0] = values[0];
        target->translation[1] = values[1];
        target->translation[2] = values[2];
    } else if (track->track_property == 270 && components >= 3) {
        target->scale[0] = values[0];
        target->scale[1] = values[1];
        target->scale[2] = values[2];
    } else if (track->track_property == 268 && components >= 4) {
        target->orientation[0] = values[0];
        target->orientation[1] = values[1];
        target->orientation[2] = values[2];
        target->orientation[3] = values[3];
    } else if (track->track_property == 276 && components >= 1) {
        target->rendering_enabled = values[0] >= 0.5f;
    } else if (track->track_property == 256 && components >= 1) {
        target->alpha = values[0];
    }
}

int
funkey_m3g_object_animate(long object, int world_time) {
    FunKeyM3GObject *obj = funkey_m3g_object(object);
    int i;
    if (obj == 0) {
        return 0x7fffffff;
    }
    if (obj->core != 0) {
        return m3gAnimate(obj->core, world_time);
    }
    for (i = 0; i < obj->animation_track_count; ++i) {
        funkey_m3g_apply_animation_track(obj, obj->animation_tracks[i], world_time);
    }
    for (i = 0; i < obj->child_count; ++i) {
        funkey_m3g_object_animate(obj->children[i], world_time);
    }
    return 0x7fffffff;
}

void
funkey_m3g_world_set_active_camera(long world, long camera) {
    FunKeyM3GObject *obj = funkey_m3g_object(world);
    if (obj != 0) {
        obj->active_camera = camera;
        if (obj->core != 0) {
            m3gSetActiveCamera((M3GWorld) obj->core,
                               (M3GCamera) funkey_m3g_core_object(camera));
        }
    }
}

long
funkey_m3g_world_get_active_camera(long world) {
    FunKeyM3GObject *obj = funkey_m3g_object(world);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject) m3gGetActiveCamera((M3GWorld) obj->core));
    }
    return obj != 0 ? obj->active_camera : 0;
}

void
funkey_m3g_world_set_background(long world, long background) {
    FunKeyM3GObject *obj = funkey_m3g_object(world);
    if (obj != 0) {
        obj->background = background;
        if (obj->core != 0) {
            m3gSetBackground((M3GWorld) obj->core,
                             (M3GBackground) funkey_m3g_core_object(background));
        }
    }
}

long
funkey_m3g_world_get_background(long world) {
    FunKeyM3GObject *obj = funkey_m3g_object(world);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject) m3gGetBackground((M3GWorld) obj->core));
    }
    return obj != 0 ? obj->background : 0;
}

void
funkey_m3g_camera_set_projection(long camera, int mode,
                                 float a, float b, float c, float d) {
    FunKeyM3GObject *obj = funkey_m3g_object(camera);
    if (obj != 0) {
        if (obj->core != 0) {
            if (mode == M3G_PARALLEL) {
                m3gSetParallel((M3GCamera) obj->core, a, b, c, d);
            } else if (mode == M3G_PERSPECTIVE) {
                m3gSetPerspective((M3GCamera) obj->core, a, b, c, d);
            }
        }
        obj->projection_mode = mode;
        obj->projection_params[0] = a;
        obj->projection_params[1] = b;
        obj->projection_params[2] = c;
        obj->projection_params[3] = d;
    }
}

void
funkey_m3g_camera_set_generic(long camera, const float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(camera);
    if (obj != 0 && matrix != 0) {
        if (obj->core != 0) {
            M3GMatrix core_matrix;
            funkey_m3g_matrix_from_float(&core_matrix, matrix);
            m3gSetProjectionMatrix((M3GCamera) obj->core, &core_matrix);
        }
        obj->projection_mode = M3G_GENERIC;
        memcpy(obj->projection_matrix, matrix, 16 * sizeof(float));
    }
}

int
funkey_m3g_camera_get_projection(long camera, float *params) {
    FunKeyM3GObject *obj = funkey_m3g_object(camera);
    int i;
    if (obj == 0) {
        return 50;
    }
    if (obj->core != 0) {
        return m3gGetProjectionAsParams((M3GCamera) obj->core, params);
    }
    if (params != 0) {
        for (i = 0; i < 4; ++i) {
            params[i] = obj->projection_params[i];
        }
    }
    return obj->projection_mode;
}

int
funkey_m3g_camera_get_projection_matrix(long camera, float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(camera);
    int mode;
    if (obj == 0) {
        if (matrix != 0) {
            funkey_m3g_matrix_identity(matrix);
        }
        return M3G_PERSPECTIVE;
    }
    if (obj->core != 0) {
        M3GMatrix core_matrix;
        m3gIdentityMatrix(&core_matrix);
        mode = m3gGetProjectionAsMatrix((M3GCamera) obj->core,
                                        matrix != 0 ? &core_matrix : 0);
        if (matrix != 0) {
            m3gGetMatrixRows(&core_matrix, matrix);
        }
        return mode;
    }
    mode = obj->projection_mode;
    if (matrix != 0) {
        if (mode == M3G_GENERIC) {
            memcpy(matrix, obj->projection_matrix, 16 * sizeof(float));
        } else {
            float height = mode == M3G_PERSPECTIVE ?
                           tanf(obj->projection_params[0] *
                                (float) M_PI / 360.0f) :
                           obj->projection_params[0];
            float aspect = obj->projection_params[1];
            float near_plane = obj->projection_params[2];
            float far_plane = obj->projection_params[3];
            float range = far_plane - near_plane;
            funkey_m3g_matrix_identity(matrix);
            if (height != 0.0f && aspect != 0.0f && range != 0.0f) {
                if (mode == M3G_PERSPECTIVE) {
                    matrix[0] = 1.0f / (aspect * height);
                    matrix[5] = 1.0f / height;
                    matrix[10] = -(far_plane + near_plane) / range;
                    matrix[11] = -1.0f;
                    matrix[14] = -(2.0f * far_plane * near_plane) / range;
                    matrix[15] = 0.0f;
                } else {
                    matrix[0] = 2.0f / (aspect * height);
                    matrix[5] = 2.0f / height;
                    matrix[10] = -2.0f / range;
                    matrix[14] = -(far_plane + near_plane) / range;
                }
            }
        }
    }
    return mode;
}

void
funkey_m3g_background_set_color(long background, unsigned int argb) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBgColor((M3GBackground) obj->core, argb);
        }
        obj->background_color = argb;
    }
}

unsigned int
funkey_m3g_background_get_color(long background) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0 && obj->core != 0) {
        return m3gGetBgColor((M3GBackground) obj->core);
    }
    return obj != 0 ? obj->background_color : 0xff000000U;
}

void
funkey_m3g_background_set_image(long background, long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBgImage((M3GBackground) obj->core,
                          (M3GImage) funkey_m3g_core_object(image));
        }
        obj->background_image = image;
    }
}

long
funkey_m3g_background_get_image(long background) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetBgImage((M3GBackground) obj->core));
    }
    return obj != 0 ? obj->background_image : 0;
}

void
funkey_m3g_background_set_image_mode(long background, int mode_x, int mode_y) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBgMode((M3GBackground) obj->core, mode_x, mode_y);
        }
        obj->image_mode_x = mode_x;
        obj->image_mode_y = mode_y;
    }
}

int
funkey_m3g_background_get_image_mode(long background, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj == 0) {
        return 32;
    }
    if (obj->core != 0) {
        return m3gGetBgMode((M3GBackground) obj->core, which);
    }
    return which == 0 ? obj->image_mode_x : obj->image_mode_y;
}

void
funkey_m3g_background_enable(long background, int which, int enable) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        m3gSetBgEnable((M3GBackground) obj->core, which,
                       enable ? M3G_TRUE : M3G_FALSE);
    }
    if (which == 0) {
        obj->color_clear_enabled = enable != 0;
    } else if (which == 1) {
        obj->depth_clear_enabled = enable != 0;
    }
}

int
funkey_m3g_background_is_enabled(long background, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gIsBgEnabled((M3GBackground) obj->core, which) != 0;
    }
    if (which == 0) {
        return obj->color_clear_enabled;
    }
    if (which == 1) {
        return obj->depth_clear_enabled;
    }
    return 0;
}

void
funkey_m3g_background_set_crop(long background, int x, int y, int w, int h) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBgCrop((M3GBackground) obj->core, x, y, w, h);
        }
        obj->crop_x = x;
        obj->crop_y = y;
        obj->crop_w = w;
        obj->crop_h = h;
    }
}

int
funkey_m3g_background_get_crop(long background, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(background);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gGetBgCrop((M3GBackground) obj->core, which);
    }
    if (which == 0) {
        return obj->crop_x;
    }
    if (which == 1) {
        return obj->crop_y;
    }
    if (which == 2) {
        return obj->crop_w;
    }
    if (which == 3) {
        return obj->crop_h;
    }
    return 0;
}

int
funkey_m3g_vertex_array_init(long handle, int vertices,
                             int components, int component_size) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GInterface m3g;
    M3GVertexArray core;
    int total;
    if (obj == 0 || vertices < 0 || components < 1 || components > 4 ||
            (component_size != 1 && component_size != 2)) {
        return 0;
    }
    total = vertices * components;
    if (total < 0) {
        return 0;
    }
    if (obj->components != 0) {
        free(obj->components);
        obj->components = 0;
    }
    if (total > 0) {
        obj->components = (int *) calloc((size_t) total, sizeof(int));
        if (obj->components == 0) {
            return 0;
        }
    }
    obj->vertex_count = vertices;
    obj->component_count = components;
    obj->component_type = component_size;
    if (obj->core != 0) {
        m3gDeleteRef(obj->core);
        obj->core = 0;
    }
    if (vertices > 0 && components >= 2) {
        m3g = obj->core_interface != 0 ? obj->core_interface :
              funkey_m3g_ensure_core_interface();
        core = m3g != 0 ?
               m3gCreateVertexArray(m3g, vertices, components,
                                    component_size == 1 ? M3G_BYTE : M3G_SHORT) :
               0;
        if (core != 0) {
            obj->core = (M3GObject) core;
            obj->core_interface = m3g;
        }
    }
    return 1;
}

void
funkey_m3g_vertex_array_set(long handle, int first, int count,
                            const int *values, int value_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int total;
    int offset;
    int copy_count;
    if (obj == 0 || values == 0 || first < 0 || count < 0 ||
            first + count > obj->vertex_count || obj->component_count <= 0) {
        return;
    }
    total = count * obj->component_count;
    offset = first * obj->component_count;
    copy_count = total < value_count ? total : value_count;
    if (copy_count > 0 && obj->components != 0) {
        memcpy(&obj->components[offset], values,
               (size_t) copy_count * sizeof(values[0]));
    }
    if (obj->core != 0 && copy_count >= total && total > 0) {
        if (obj->component_type == 1) {
            signed char *tmp = (signed char *) malloc((size_t) total);
            int i;
            if (tmp != 0) {
                for (i = 0; i < total; ++i) {
                    int v = values[i];
                    if (v < -128) v = -128;
                    if (v > 127) v = 127;
                    tmp[i] = (signed char) v;
                }
                m3gSetVertexArrayElements((M3GVertexArray) obj->core, first,
                                          count, total, M3G_BYTE, tmp);
                free(tmp);
            }
        } else {
            short *tmp = (short *) malloc((size_t) total * sizeof(short));
            int i;
            if (tmp != 0) {
                for (i = 0; i < total; ++i) {
                    int v = values[i];
                    if (v < -32768) v = -32768;
                    if (v > 32767) v = 32767;
                    tmp[i] = (short) v;
                }
                m3gSetVertexArrayElements((M3GVertexArray) obj->core, first,
                                          count, total, M3G_SHORT, tmp);
                free(tmp);
            }
        }
    }
}

void
funkey_m3g_vertex_array_get(long handle, int first, int count,
                            int *values, int value_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int vertex_count;
    int component_count;
    int component_type;
    int total;
    int offset;
    int copy_count;
    if (obj == 0 || values == 0 || first < 0 || count < 0) {
        return;
    }
    vertex_count = funkey_m3g_vertex_array_get_vertex_count(handle);
    component_count = funkey_m3g_vertex_array_get_component_count(handle);
    component_type = funkey_m3g_vertex_array_get_component_type(handle);
    if (first + count > vertex_count || component_count <= 0) {
        return;
    }
    total = count * component_count;
    offset = first * component_count;
    copy_count = total < value_count ? total : value_count;
    if (copy_count > 0 && obj->components != 0) {
        memcpy(values, &obj->components[offset],
               (size_t) copy_count * sizeof(values[0]));
    } else if (obj->core != 0 && copy_count > 0) {
        if (component_type == 1) {
            signed char *tmp = (signed char *) calloc((size_t) copy_count, 1);
            int i;
            if (tmp != 0) {
                m3gGetVertexArrayElements((M3GVertexArray) obj->core, first,
                                          count, copy_count, M3G_BYTE, tmp);
                for (i = 0; i < copy_count; ++i) {
                    values[i] = tmp[i];
                }
                free(tmp);
            }
        } else {
            short *tmp = (short *) calloc((size_t) copy_count, sizeof(short));
            int i;
            if (tmp != 0) {
                m3gGetVertexArrayElements((M3GVertexArray) obj->core, first,
                                          count, copy_count, M3G_SHORT, tmp);
                for (i = 0; i < copy_count; ++i) {
                    values[i] = tmp[i];
                }
                free(tmp);
            }
        }
    }
}

int
funkey_m3g_vertex_array_get_component_count(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        M3Gint size = 0;
        m3gGetVertexArrayParams((M3GVertexArray) obj->core, 0, &size, 0, 0);
        return size;
    }
    return obj != 0 ? obj->component_count : 0;
}

int
funkey_m3g_vertex_array_get_component_type(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        M3Gdatatype type = M3G_BYTE;
        m3gGetVertexArrayParams((M3GVertexArray) obj->core, 0, 0, &type, 0);
        return type == M3G_BYTE ? 1 : 2;
    }
    return obj != 0 ? obj->component_type : 0;
}

int
funkey_m3g_vertex_array_get_vertex_count(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj != 0 && obj->core != 0) {
        M3Gsizei count = 0;
        m3gGetVertexArrayParams((M3GVertexArray) obj->core, &count, 0, 0, 0);
        return count;
    }
    return obj != 0 ? obj->vertex_count : 0;
}

void
funkey_m3g_vertex_array_transform(long handle, const float *matrix,
                                  float *out, int out_count, int use_w) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GMatrix core_matrix;
    int i;
    if (obj == 0 || out == 0 || out_count <= 0) {
        return;
    }
    if (obj->core != 0 && obj->component_count != 4) {
        funkey_m3g_matrix_from_float(&core_matrix, matrix);
        m3gTransformArray((M3GVertexArray) obj->core, &core_matrix,
                          out, out_count, use_w ? M3G_TRUE : M3G_FALSE);
        return;
    }
    if (obj->components == 0 || obj->component_count <= 0) {
        return;
    }
    for (i = 0; i < obj->vertex_count && i * 4 + 3 < out_count; ++i) {
        int base = i * obj->component_count;
        float x = obj->components[base];
        float y = obj->component_count > 1 ? obj->components[base + 1] : 0.0f;
        float z = obj->component_count > 2 ? obj->components[base + 2] : 0.0f;
        float w = use_w ? 1.0f :
                  (obj->component_count > 3 ? obj->components[base + 3] : 0.0f);
        if (matrix != 0) {
            out[i * 4]     = matrix[0]*x + matrix[1]*y + matrix[2]*z + matrix[3]*w;
            out[i * 4 + 1] = matrix[4]*x + matrix[5]*y + matrix[6]*z + matrix[7]*w;
            out[i * 4 + 2] = matrix[8]*x + matrix[9]*y + matrix[10]*z + matrix[11]*w;
            out[i * 4 + 3] = matrix[12]*x + matrix[13]*y + matrix[14]*z + matrix[15]*w;
        } else {
            out[i * 4] = x;
            out[i * 4 + 1] = y;
            out[i * 4 + 2] = z;
            out[i * 4 + 3] = w;
        }
    }
}

void
funkey_m3g_vertex_buffer_set_array(long buffer, int which, long array,
                                   float scale, const float *bias,
                                   int bias_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(buffer);
    float *scale_bias = 0;
    int i;
    if (obj == 0) {
        return;
    }
    if (which == 0) {
        obj->vb_positions = array;
        scale_bias = obj->vb_position_scale_bias;
    } else if (which == 1) {
        obj->vb_normals = array;
    } else if (which == 2) {
        obj->vb_colors = array;
    } else if (which >= 3 && which < 3 + FUNKEY_M3G_NUM_TEXTURE_UNITS) {
        obj->vb_texcoords[which - 3] = array;
        scale_bias = obj->vb_texcoord_scale_bias[which - 3];
    }
    if (scale_bias != 0) {
        scale_bias[0] = scale;
        for (i = 0; i < 3; ++i) {
            scale_bias[i + 1] = (bias != 0 && i < bias_count) ? bias[i] : 0.0f;
        }
    }
    if (obj->core != 0) {
        M3GVertexArray core_array = (M3GVertexArray) funkey_m3g_core_object(array);
        if (which == 0) {
            m3gSetVertexArray((M3GVertexBuffer) obj->core, core_array, scale,
                              (M3Gfloat *) bias, bias_count);
        } else if (which == 1) {
            m3gSetNormalArray((M3GVertexBuffer) obj->core, core_array);
        } else if (which == 2) {
            m3gSetColorArray((M3GVertexBuffer) obj->core, core_array);
        } else if (which >= 3 && which < 3 + FUNKEY_M3G_NUM_TEXTURE_UNITS) {
            m3gSetTexCoordArray((M3GVertexBuffer) obj->core, which - 3,
                                core_array, scale, (M3Gfloat *) bias,
                                bias_count);
        }
    }
}

long
funkey_m3g_vertex_buffer_get_array(long buffer, int which,
                                   float *scale_bias, int scale_bias_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(buffer);
    float *stored = 0;
    long handle = 0;
    int i;
    if (obj == 0) {
        return 0;
    }
    if (which == 0) {
        handle = obj->vb_positions;
        stored = obj->vb_position_scale_bias;
    } else if (which == 1) {
        handle = obj->vb_normals;
    } else if (which == 2) {
        handle = obj->vb_colors;
    } else if (which >= 3 && which < 3 + FUNKEY_M3G_NUM_TEXTURE_UNITS) {
        handle = obj->vb_texcoords[which - 3];
        stored = obj->vb_texcoord_scale_bias[which - 3];
    }
    if (obj->core != 0) {
        M3Gfloat sb[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
        M3GVertexArray array = m3gGetVertexArray((M3GVertexBuffer) obj->core,
                                                 which, sb, 4);
        handle = funkey_m3g_wrap_core_object((M3GObject) array);
        if (scale_bias != 0 && scale_bias_count > 0) {
            for (i = 0; i < scale_bias_count && i < 4; ++i) {
                scale_bias[i] = sb[i];
            }
        }
        return handle;
    }
    if (scale_bias != 0 && scale_bias_count > 0) {
        for (i = 0; i < scale_bias_count && i < 4; ++i) {
            scale_bias[i] = stored != 0 ? stored[i] : (i == 0 ? 1.0f : 0.0f);
        }
    }
    return handle;
}

void
funkey_m3g_vertex_buffer_set_default_color(long buffer, unsigned int argb) {
    FunKeyM3GObject *obj = funkey_m3g_object(buffer);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetVertexDefaultColor((M3GVertexBuffer) obj->core, argb);
        }
        obj->default_color = argb;
    }
}

unsigned int
funkey_m3g_vertex_buffer_get_default_color(long buffer) {
    FunKeyM3GObject *obj = funkey_m3g_object(buffer);
    if (obj != 0 && obj->core != 0) {
        return m3gGetVertexDefaultColor((M3GVertexBuffer) obj->core);
    }
    return obj != 0 ? obj->default_color : 0xffffffffU;
}

int
funkey_m3g_vertex_buffer_get_vertex_count(long buffer) {
    FunKeyM3GObject *obj = funkey_m3g_object(buffer);
    if (obj != 0 && obj->core != 0) {
        return m3gGetVertexCount((M3GVertexBuffer) obj->core);
    }
    if (obj == 0 || obj->vb_positions == 0) {
        return 0;
    }
    return funkey_m3g_vertex_array_get_vertex_count(obj->vb_positions);
}

int
funkey_m3g_index_buffer_init(long handle, const int *indices, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    if (obj == 0 || count < 0) {
        return 0;
    }
    if (obj->indices != 0) {
        free(obj->indices);
        obj->indices = 0;
    }
    obj->index_count = count;
    if (count > 0) {
        obj->indices = (int *) malloc((size_t) count * sizeof(int));
        if (obj->indices == 0) {
            obj->index_count = 0;
            return 0;
        }
        memcpy(obj->indices, indices, (size_t) count * sizeof(int));
    }
    return 1;
}

static int
funkey_m3g_expand_strips(const int *source, int source_count, int first,
                         const int *lengths, int length_count,
                         int **out_indices) {
    int i;
    int total = 0;
    int offset = 0;
    int out_pos = 0;
    int *expanded;
    if (out_indices == 0 || lengths == 0 || length_count < 0) {
        return 0;
    }
    *out_indices = 0;
    for (i = 0; i < length_count; ++i) {
        if (lengths[i] >= 3) {
            total += (lengths[i] - 2) * 3;
        }
    }
    expanded = total > 0 ? (int *) malloc((size_t) total * sizeof(int)) : 0;
    if (total > 0 && expanded == 0) {
        return 0;
    }
    for (i = 0; i < length_count; ++i) {
        int j;
        int len = lengths[i];
        for (j = 0; j + 2 < len; ++j) {
            int a;
            int b;
            int c;
            if (source != 0 && offset + j + 2 >= source_count) {
                free(expanded);
                return 0;
            }
            a = source != 0 ? source[offset + j] : first + offset + j;
            b = source != 0 ? source[offset + j + 1] : first + offset + j + 1;
            c = source != 0 ? source[offset + j + 2] : first + offset + j + 2;
            if ((j & 1) != 0) {
                int tmp = a;
                a = b;
                b = tmp;
            }
            expanded[out_pos++] = a;
            expanded[out_pos++] = b;
            expanded[out_pos++] = c;
        }
        offset += len;
    }
    *out_indices = expanded;
    return out_pos;
}

int
funkey_m3g_index_buffer_init_strips(long handle, int first_index,
                                    const int *indices, int index_count,
                                    const int *lengths, int length_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    M3GInterface m3g;
    M3GIndexBuffer core = 0;
    M3Gsizei *core_lengths;
    int *expanded = 0;
    int expanded_count;
    int i;
    if (obj == 0 || lengths == 0 || length_count <= 0 || index_count < 0) {
        return 0;
    }
    expanded_count = funkey_m3g_expand_strips(indices, index_count,
                                              first_index, lengths,
                                              length_count, &expanded);
    funkey_m3g_index_buffer_init(handle, expanded, expanded_count);
    if (expanded != 0) {
        free(expanded);
    }
    core_lengths = (M3Gsizei *) malloc((size_t) length_count *
                                       sizeof(M3Gsizei));
    if (core_lengths == 0) {
        return expanded_count > 0;
    }
    for (i = 0; i < length_count; ++i) {
        core_lengths[i] = lengths[i];
    }
    m3g = obj->core_interface != 0 ? obj->core_interface :
          funkey_m3g_ensure_core_interface();
    if (m3g != 0) {
        if (indices != 0) {
            core = m3gCreateStripBuffer(m3g, M3G_TRIANGLE_STRIPS,
                                        length_count, core_lengths, M3G_INT,
                                        index_count, indices);
        } else {
            core = m3gCreateImplicitStripBuffer(m3g, length_count,
                                                core_lengths, first_index);
        }
    }
    free(core_lengths);
    if (core != 0) {
        if (obj->core != 0) {
            m3gDeleteRef(obj->core);
        }
        obj->core = (M3GObject) core;
        obj->core_interface = m3g;
    }
    return 1;
}

int
funkey_m3g_index_buffer_init_implicit(long handle, int first,
                                      const int *lengths, int length_count) {
    int total = 0;
    int i;
    int *indices;
    if (length_count < 0) {
        return 0;
    }
    for (i = 0; i < length_count; ++i) {
        if (lengths[i] > 0) {
            total += lengths[i];
        }
    }
    indices = total > 0 ? (int *) malloc((size_t) total * sizeof(int)) : 0;
    if (total > 0 && indices == 0) {
        return 0;
    }
    for (i = 0; i < total; ++i) {
        indices[i] = first + i;
    }
    i = funkey_m3g_index_buffer_init(handle, indices, total);
    if (indices != 0) {
        free(indices);
    }
    return i;
}

int
funkey_m3g_index_buffer_get_count(long handle) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    return obj != 0 ? obj->index_count : 0;
}

void
funkey_m3g_index_buffer_get_indices(long handle, int *indices, int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    int copy_count;
    if (obj == 0 || indices == 0 || count <= 0 || obj->indices == 0) {
        return;
    }
    copy_count = count < obj->index_count ? count : obj->index_count;
    memcpy(indices, obj->indices, (size_t) copy_count * sizeof(indices[0]));
}

int
funkey_m3g_mesh_init(long mesh, long vertices, const long *triangles,
                     const long *appearances, int submesh_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GInterface m3g;
    M3GVertexBuffer core_vertices;
    M3Gulong core_triangles[FUNKEY_M3G_MAX_SUBMESHES];
    M3Gulong core_appearances[FUNKEY_M3G_MAX_SUBMESHES];
    M3GMesh core_mesh = 0;
    int i;
    if (obj == 0 || submesh_count < 0 ||
            submesh_count > FUNKEY_M3G_MAX_SUBMESHES) {
        return 0;
    }
    obj->mesh_vertices = vertices;
    obj->mesh_submesh_count = submesh_count;
    for (i = 0; i < submesh_count; ++i) {
        obj->mesh_indices[i] = triangles != 0 ? triangles[i] : 0;
        obj->mesh_appearances[i] = appearances != 0 ? appearances[i] : 0;
    }
    core_vertices = (M3GVertexBuffer) funkey_m3g_core_object(vertices);
    if (core_vertices != 0 && submesh_count > 0) {
        int ready = 1;
        for (i = 0; i < submesh_count; ++i) {
            M3GObject tri = funkey_m3g_core_object(triangles != 0 ? triangles[i] : 0);
            M3GObject app = funkey_m3g_core_object(appearances != 0 ? appearances[i] : 0);
            if (tri == 0) {
                ready = 0;
                break;
            }
            core_triangles[i] = (M3Gulong) (uintptr_t) tri;
            core_appearances[i] = (M3Gulong) (uintptr_t) app;
        }
        if (ready) {
            m3g = obj->core_interface != 0 ? obj->core_interface :
                  funkey_m3g_ensure_core_interface();
            if (m3g != 0) {
                core_mesh = m3gCreateMesh(m3g, core_vertices, core_triangles,
                                          core_appearances, submesh_count);
            }
            if (core_mesh != 0) {
                if (obj->core != 0) {
                    m3gDeleteRef(obj->core);
                }
                obj->core = (M3GObject) core_mesh;
                obj->core_interface = m3g;
            }
        }
    }
    return 1;
}

void
funkey_m3g_mesh_set_appearance(long mesh, int index, long appearance) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        m3gSetAppearance((M3GMesh) obj->core, index,
                         (M3GAppearance) funkey_m3g_core_object(appearance));
        return;
    }
    if (obj != 0 && index >= 0 && index < obj->mesh_submesh_count) {
        obj->mesh_appearances[index] = appearance;
    }
}

long
funkey_m3g_mesh_get_appearance(long mesh, int index) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetAppearance((M3GMesh) obj->core,
                                                            index));
    }
    if (obj == 0 || index < 0 || index >= obj->mesh_submesh_count) {
        return 0;
    }
    return obj->mesh_appearances[index];
}

long
funkey_m3g_mesh_get_index_buffer(long mesh, int index) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetIndexBuffer((M3GMesh) obj->core,
                                                             index));
    }
    if (obj == 0 || index < 0 || index >= obj->mesh_submesh_count) {
        return 0;
    }
    return obj->mesh_indices[index];
}

long
funkey_m3g_mesh_get_vertex_buffer(long mesh) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetVertexBuffer((M3GMesh) obj->core));
    }
    return obj != 0 ? obj->mesh_vertices : 0;
}

int
funkey_m3g_mesh_get_submesh_count(long mesh) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return m3gGetSubmeshCount((M3GMesh) obj->core);
    }
    return obj != 0 ? obj->mesh_submesh_count : 0;
}

int
funkey_m3g_morphing_mesh_init(long mesh, long vertices,
                              const long *targets, int target_count,
                              const long *triangles,
                              const long *appearances,
                              int submesh_count) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GInterface m3g;
    M3GVertexBuffer core_vertices;
    M3Gulong core_targets[FUNKEY_M3G_MAX_SUBMESHES];
    M3Gulong core_triangles[FUNKEY_M3G_MAX_SUBMESHES];
    M3Gulong core_appearances[FUNKEY_M3G_MAX_SUBMESHES];
    M3GMorphingMesh core_mesh = 0;
    int i;
    if (obj == 0 || target_count < 0 ||
            target_count > FUNKEY_M3G_MAX_SUBMESHES) {
        return 0;
    }
    funkey_m3g_mesh_init(mesh, vertices, triangles, appearances, submesh_count);
    obj->morph_target_count = target_count;
    for (i = 0; i < target_count; ++i) {
        obj->morph_targets[i] = targets != 0 ? targets[i] : 0;
        obj->morph_weights[i] = 0.0f;
    }
    core_vertices = (M3GVertexBuffer) funkey_m3g_core_object(vertices);
    if (core_vertices != 0 && submesh_count > 0 && target_count > 0) {
        int ready = 1;
        for (i = 0; i < target_count; ++i) {
            M3GObject target = funkey_m3g_core_object(targets != 0 ? targets[i] : 0);
            if (target == 0) {
                ready = 0;
                break;
            }
            core_targets[i] = (M3Gulong) (uintptr_t) target;
        }
        for (i = 0; ready && i < submesh_count; ++i) {
            M3GObject tri = funkey_m3g_core_object(triangles != 0 ? triangles[i] : 0);
            M3GObject app = funkey_m3g_core_object(appearances != 0 ? appearances[i] : 0);
            if (tri == 0) {
                ready = 0;
                break;
            }
            core_triangles[i] = (M3Gulong) (uintptr_t) tri;
            core_appearances[i] = (M3Gulong) (uintptr_t) app;
        }
        if (ready) {
            m3g = obj->core_interface != 0 ? obj->core_interface :
                  funkey_m3g_ensure_core_interface();
            if (m3g != 0) {
                core_mesh = m3gCreateMorphingMesh(m3g, core_vertices,
                                                  core_targets, core_triangles,
                                                  core_appearances,
                                                  submesh_count, target_count);
            }
            if (core_mesh != 0) {
                if (obj->core != 0) {
                    m3gDeleteRef(obj->core);
                }
                obj->core = (M3GObject) core_mesh;
                obj->core_interface = m3g;
            }
        }
    }
    return 1;
}

void
funkey_m3g_morphing_mesh_set_weights(long mesh, const float *weights,
                                     int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    int i;
    if (obj == 0 || weights == 0 || count < 0) {
        return;
    }
    if (count > obj->morph_target_count) {
        count = obj->morph_target_count;
    }
    for (i = 0; i < count; ++i) {
        obj->morph_weights[i] = weights[i];
    }
    if (obj->core != 0) {
        m3gSetWeights((M3GMorphingMesh) obj->core, (M3Gfloat *) weights, count);
    }
}

void
funkey_m3g_morphing_mesh_get_weights(long mesh, float *weights,
                                     int count) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    int i;
    if (obj == 0 || weights == 0 || count <= 0) {
        return;
    }
    if (obj->core != 0) {
        m3gGetWeights((M3GMorphingMesh) obj->core, weights, count);
        return;
    }
    if (count > obj->morph_target_count) {
        count = obj->morph_target_count;
    }
    for (i = 0; i < count; ++i) {
        weights[i] = obj->morph_weights[i];
    }
}

long
funkey_m3g_morphing_mesh_get_target(long mesh, int index) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetMorphTarget((M3GMorphingMesh)
                                                             obj->core, index));
    }
    if (obj == 0 || index < 0 || index >= obj->morph_target_count) {
        return 0;
    }
    return obj->morph_targets[index];
}

int
funkey_m3g_morphing_mesh_get_target_count(long mesh) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return m3gGetMorphTargetCount((M3GMorphingMesh) obj->core);
    }
    return obj != 0 ? obj->morph_target_count : 0;
}

int
funkey_m3g_skinned_mesh_init(long mesh, long vertices,
                             const long *triangles,
                             const long *appearances,
                             int submesh_count, long skeleton) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GInterface m3g;
    M3GVertexBuffer core_vertices;
    M3GGroup core_skeleton;
    M3Gulong core_triangles[FUNKEY_M3G_MAX_SUBMESHES];
    M3Gulong core_appearances[FUNKEY_M3G_MAX_SUBMESHES];
    M3GSkinnedMesh core_mesh = 0;
    int i;
    if (obj == 0) {
        return 0;
    }
    funkey_m3g_mesh_init(mesh, vertices, triangles, appearances, submesh_count);
    obj->skin_skeleton = skeleton;
    core_vertices = (M3GVertexBuffer) funkey_m3g_core_object(vertices);
    core_skeleton = (M3GGroup) funkey_m3g_core_object(skeleton);
    if (core_vertices != 0 && core_skeleton != 0 && submesh_count > 0) {
        int ready = 1;
        for (i = 0; i < submesh_count; ++i) {
            M3GObject tri = funkey_m3g_core_object(triangles != 0 ? triangles[i] : 0);
            M3GObject app = funkey_m3g_core_object(appearances != 0 ? appearances[i] : 0);
            if (tri == 0) {
                ready = 0;
                break;
            }
            core_triangles[i] = (M3Gulong) (uintptr_t) tri;
            core_appearances[i] = (M3Gulong) (uintptr_t) app;
        }
        if (ready) {
            m3g = obj->core_interface != 0 ? obj->core_interface :
                  funkey_m3g_ensure_core_interface();
            if (m3g != 0) {
                core_mesh = m3gCreateSkinnedMesh(m3g, core_vertices,
                                                 core_triangles,
                                                 core_appearances,
                                                 submesh_count,
                                                 core_skeleton);
            }
            if (core_mesh != 0) {
                if (obj->core != 0) {
                    m3gDeleteRef(obj->core);
                }
                obj->core = (M3GObject) core_mesh;
                obj->core_interface = m3g;
            }
        }
    }
    return 1;
}

void
funkey_m3g_skinned_mesh_add_transform(long mesh, long bone, int weight,
                                      int first_vertex, int num_vertices) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GObject core_bone = funkey_m3g_core_object(bone);
    if (obj != 0 && obj->core != 0 && core_bone != 0) {
        m3gAddTransform((M3GSkinnedMesh) obj->core, (M3GNode) core_bone,
                        weight, first_vertex, num_vertices);
    }
}

long
funkey_m3g_skinned_mesh_get_skeleton(long mesh) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetSkeleton((M3GSkinnedMesh)
                                                          obj->core));
    }
    return obj != 0 ? obj->skin_skeleton : 0;
}

void
funkey_m3g_skinned_mesh_get_bone_transform(long mesh, long bone,
                                           float *matrix) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GObject core_bone = funkey_m3g_core_object(bone);
    M3GMatrix core_matrix;
    if (matrix == 0) {
        return;
    }
    funkey_m3g_matrix_identity(matrix);
    if (obj != 0 && obj->core != 0 && core_bone != 0) {
        m3gGetBoneTransform((M3GSkinnedMesh) obj->core,
                            (M3GNode) core_bone, &core_matrix);
        m3gGetMatrixRows(&core_matrix, matrix);
    }
}

int
funkey_m3g_skinned_mesh_get_bone_vertices(long mesh, long bone,
                                          int *indices, float *weights) {
    FunKeyM3GObject *obj = funkey_m3g_object(mesh);
    M3GObject core_bone = funkey_m3g_core_object(bone);
    if (obj != 0 && obj->core != 0 && core_bone != 0) {
        return m3gGetBoneVertices((M3GSkinnedMesh) obj->core,
                                  (M3GNode) core_bone,
                                  (M3Gint *) indices, weights);
    }
    return 0;
}

int
funkey_m3g_sprite_init(long sprite, int scaled, long image,
                       long appearance) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    M3GInterface m3g;
    M3GSprite core_sprite;
    if (obj == 0) {
        return 0;
    }
    obj->sprite_scaled = scaled != 0;
    obj->sprite_image = image;
    obj->sprite_appearance = appearance;
    m3g = obj->core_interface != 0 ? obj->core_interface :
          funkey_m3g_ensure_core_interface();
    core_sprite = m3g != 0 ?
        m3gCreateSprite(m3g, obj->sprite_scaled ? M3G_TRUE : M3G_FALSE,
                        (M3GImage) funkey_m3g_core_object(image),
                        (M3GAppearance) funkey_m3g_core_object(appearance)) :
        0;
    if (core_sprite != 0) {
        if (obj->core != 0) {
            m3gDeleteRef(obj->core);
        }
        obj->core = (M3GObject) core_sprite;
        obj->core_interface = m3g;
    }
    return 1;
}

int
funkey_m3g_sprite_is_scaled(long sprite) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0 && obj->core != 0) {
        return m3gIsScaledSprite((M3GSprite) obj->core) != 0;
    }
    return obj != 0 ? obj->sprite_scaled : 0;
}

void
funkey_m3g_sprite_set_appearance(long sprite, long appearance) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetSpriteAppearance((M3GSprite) obj->core,
                                   (M3GAppearance)
                                   funkey_m3g_core_object(appearance));
        }
        obj->sprite_appearance = appearance;
    }
}

long
funkey_m3g_sprite_get_appearance(long sprite) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetSpriteAppearance((M3GSprite)
                                                                  obj->core));
    }
    return obj != 0 ? obj->sprite_appearance : 0;
}

void
funkey_m3g_sprite_set_image(long sprite, long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetSpriteImage((M3GSprite) obj->core,
                              (M3GImage) funkey_m3g_core_object(image));
        }
        obj->sprite_image = image;
    }
}

long
funkey_m3g_sprite_get_image(long sprite) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetSpriteImage((M3GSprite)
                                                             obj->core));
    }
    return obj != 0 ? obj->sprite_image : 0;
}

void
funkey_m3g_sprite_set_crop(long sprite, int x, int y, int w, int h) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetCrop((M3GSprite) obj->core, x, y, w, h);
        }
        obj->crop_x = x;
        obj->crop_y = y;
        obj->crop_w = w;
        obj->crop_h = h;
    }
}

int
funkey_m3g_sprite_get_crop(long sprite, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(sprite);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gGetCrop((M3GSprite) obj->core, which);
    }
    if (which == 0) return obj->crop_x;
    if (which == 1) return obj->crop_y;
    if (which == 2) return obj->crop_w;
    if (which == 3) return obj->crop_h;
    return 0;
}

int
funkey_m3g_image_init(long image, int format, int width, int height,
                      const unsigned char *pixels, int pixel_count,
                      int mutable_image) {
    FunKeyM3GObject *obj = funkey_m3g_object(image);
    if (obj == 0 || width < 0 || height < 0 || pixel_count < 0) {
        return 0;
    }
    if (obj->image_pixels != 0) {
        free(obj->image_pixels);
        obj->image_pixels = 0;
    }
    if (obj->image_palette != 0) {
        free(obj->image_palette);
        obj->image_palette = 0;
    }
    if (pixel_count > 0 && pixels != 0) {
        obj->image_pixels = (unsigned char *) malloc((size_t) pixel_count);
        if (obj->image_pixels == 0) {
            return 0;
        }
        memcpy(obj->image_pixels, pixels, (size_t) pixel_count);
    }
    obj->image_format = format;
    obj->image_width = width;
    obj->image_height = height;
    obj->image_mutable = mutable_image != 0;
    obj->image_pixel_count = pixel_count;
    obj->image_palette_count = 0;
    if (obj->image_palette == 0) {
        funkey_m3g_sync_core_image(obj);
    }
    return 1;
}

int
funkey_m3g_image_init_palette(long image, int format, int width, int height,
                              const unsigned char *pixels, int pixel_count,
                              const unsigned char *palette, int palette_count) {
    FunKeyM3GObject *obj;
    if (!funkey_m3g_image_init(image, format, width, height, pixels,
                               pixel_count, 0)) {
        return 0;
    }
    obj = funkey_m3g_object(image);
    if (obj == 0 || palette_count < 0) {
        return 0;
    }
    if (palette_count > 0 && palette != 0) {
        obj->image_palette = (unsigned char *) malloc((size_t) palette_count);
        if (obj->image_palette == 0) {
            return 0;
        }
        memcpy(obj->image_palette, palette, (size_t) palette_count);
        obj->image_palette_count = palette_count;
    }
    funkey_m3g_sync_core_image(obj);
    return 1;
}

void
funkey_m3g_image_set(long image, int x, int y, int width, int height,
                     const unsigned char *pixels, int pixel_count) {
    (void) x;
    (void) y;
    funkey_m3g_image_init(image, funkey_m3g_image_get_format(image),
                          width, height, pixels, pixel_count, 1);
}

int
funkey_m3g_image_get_format(long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(image);
    if (obj != 0 && obj->core != 0) {
        return (int) m3gGetFormat((M3GImage) obj->core);
    }
    return obj != 0 ? obj->image_format : 0;
}

int
funkey_m3g_image_get_width(long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(image);
    if (obj != 0 && obj->core != 0) {
        return m3gGetWidth((M3GImage) obj->core);
    }
    return obj != 0 ? obj->image_width : 0;
}

int
funkey_m3g_image_get_height(long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(image);
    if (obj != 0 && obj->core != 0) {
        return m3gGetHeight((M3GImage) obj->core);
    }
    return obj != 0 ? obj->image_height : 0;
}

int
funkey_m3g_image_is_mutable(long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(image);
    if (obj != 0 && obj->core != 0) {
        return m3gIsMutable((M3GImage) obj->core) != 0;
    }
    return obj != 0 ? obj->image_mutable : 0;
}

static int
funkey_m3g_texture_create_core(FunKeyM3GObject *obj, M3GImage image) {
    M3GInterface m3g;
    M3GMatrix transform;
    M3GTexture texture;
    if (obj == 0 || obj->class_id != FUNKEY_M3G_CLASS_TEXTURE_2D ||
            obj->core != 0 || image == 0) {
        return obj != 0 && obj->core != 0;
    }
    m3g = obj->core_interface != 0 ? obj->core_interface :
          funkey_m3g_ensure_core_interface();
    if (m3g == 0) {
        return 0;
    }
    texture = m3gCreateTexture(m3g, image);
    if (texture == 0) {
        return 0;
    }
    obj->core = (M3GObject) texture;
    obj->core_interface = m3g;

    m3gSetFiltering(texture, obj->texture_level_filter,
                    obj->texture_image_filter);
    m3gSetWrapping(texture, obj->texture_wrap_s, obj->texture_wrap_t);
    m3gTextureSetBlending(texture, obj->texture_blending);
    m3gSetBlendColor(texture, obj->texture_blend_color);
    funkey_m3g_matrix_from_float(&transform, obj->transform);
    m3gSetTransform((M3GTransformable) texture, &transform);
    m3gSetOrientation((M3GTransformable) texture, obj->orientation[0],
                      obj->orientation[1], obj->orientation[2],
                      obj->orientation[3]);
    m3gSetScale((M3GTransformable) texture, obj->scale[0], obj->scale[1],
                obj->scale[2]);
    m3gSetTranslation((M3GTransformable) texture, obj->translation[0],
                      obj->translation[1], obj->translation[2]);
    return 1;
}

void
funkey_m3g_texture_set_image(long texture, long image) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    M3GObject image_core = funkey_m3g_core_object(image);
    if (obj != 0) {
        if (obj->core == 0 && image_core != 0) {
            funkey_m3g_texture_create_core(obj, (M3GImage) image_core);
        }
        if (obj->core != 0) {
            m3gSetTextureImage((M3GTexture) obj->core,
                               (M3GImage) image_core);
            if (g_texture_set_trace_count < 96 && image_core != 0) {
                fprintf(stderr,
                        "[M3G map] texture=%ld uid=%d image=%ld size=%dx%d format=%d\n",
                        texture, m3gGetUserID(obj->core), image,
                        m3gGetWidth((M3GImage) image_core),
                        m3gGetHeight((M3GImage) image_core),
                        (int)m3gGetFormat((M3GImage) image_core));
                ++g_texture_set_trace_count;
            }
        }
        obj->texture_image = image;
    }
}

long
funkey_m3g_texture_get_image(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetTextureImage((M3GTexture) obj->core));
    }
    return obj != 0 ? obj->texture_image : 0;
}

void
funkey_m3g_texture_set_filtering(long texture, int level, int image) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetFiltering((M3GTexture) obj->core, level, image);
        }
        obj->texture_level_filter = level;
        obj->texture_image_filter = image;
    }
}

void
funkey_m3g_texture_set_wrapping(long texture, int s, int t) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetWrapping((M3GTexture) obj->core, s, t);
        }
        obj->texture_wrap_s = s;
        obj->texture_wrap_t = t;
    }
}

int
funkey_m3g_texture_get_wrapping_s(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        return m3gGetWrappingS((M3GTexture) obj->core);
    }
    return obj != 0 ? obj->texture_wrap_s : 241;
}

int
funkey_m3g_texture_get_wrapping_t(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        return m3gGetWrappingT((M3GTexture) obj->core);
    }
    return obj != 0 ? obj->texture_wrap_t : 241;
}

void
funkey_m3g_texture_set_blending(long texture, int func) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gTextureSetBlending((M3GTexture) obj->core, func);
        }
        obj->texture_blending = func;
    }
}

int
funkey_m3g_texture_get_blending(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        return m3gTextureGetBlending((M3GTexture) obj->core);
    }
    return obj != 0 ? obj->texture_blending : 227;
}

void
funkey_m3g_texture_set_blend_color(long texture, unsigned int rgb) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBlendColor((M3GTexture) obj->core, rgb);
        }
        obj->texture_blend_color = rgb;
    }
}

unsigned int
funkey_m3g_texture_get_blend_color(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        return m3gGetBlendColor((M3GTexture) obj->core);
    }
    return obj != 0 ? obj->texture_blend_color : 0;
}

int
funkey_m3g_texture_get_image_filter(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        int level_filter;
        int image_filter;
        m3gGetFiltering((M3GTexture) obj->core, &level_filter, &image_filter);
        return image_filter;
    }
    return obj != 0 ? obj->texture_image_filter : 210;
}

int
funkey_m3g_texture_get_level_filter(long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(texture);
    if (obj != 0 && obj->core != 0) {
        int level_filter;
        int image_filter;
        m3gGetFiltering((M3GTexture) obj->core, &level_filter, &image_filter);
        return level_filter;
    }
    return obj != 0 ? obj->texture_level_filter : 208;
}

void
funkey_m3g_appearance_set(long appearance, int slot, long value) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            m3gSetCompositingMode((M3GAppearance) obj->core,
                                  (M3GCompositingMode) funkey_m3g_core_object(value));
        } else if (slot == 1) {
            m3gSetFog((M3GAppearance) obj->core,
                      (M3GFog) funkey_m3g_core_object(value));
        } else if (slot == 2) {
            m3gSetMaterial((M3GAppearance) obj->core,
                           (M3GMaterial) funkey_m3g_core_object(value));
        } else if (slot == 3) {
            m3gSetPolygonMode((M3GAppearance) obj->core,
                              (M3GPolygonMode) funkey_m3g_core_object(value));
        }
    }
    if (slot == 0) {
        obj->appearance_compositing = value;
    } else if (slot == 1) {
        obj->appearance_fog = value;
    } else if (slot == 2) {
        obj->appearance_material = value;
    } else if (slot == 3) {
        obj->appearance_polygon = value;
    }
}

long
funkey_m3g_appearance_get(long appearance, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            return funkey_m3g_wrap_core_object((M3GObject)
                                               m3gGetCompositingMode((M3GAppearance) obj->core));
        }
        if (slot == 1) {
            return funkey_m3g_wrap_core_object((M3GObject)
                                               m3gGetFog((M3GAppearance) obj->core));
        }
        if (slot == 2) {
            return funkey_m3g_wrap_core_object((M3GObject)
                                               m3gGetMaterial((M3GAppearance) obj->core));
        }
        if (slot == 3) {
            return funkey_m3g_wrap_core_object((M3GObject)
                                               m3gGetPolygonMode((M3GAppearance) obj->core));
        }
        return 0;
    }
    if (slot == 0) {
        return obj->appearance_compositing;
    }
    if (slot == 1) {
        return obj->appearance_fog;
    }
    if (slot == 2) {
        return obj->appearance_material;
    }
    if (slot == 3) {
        return obj->appearance_polygon;
    }
    return 0;
}

void
funkey_m3g_appearance_set_texture(long appearance, int unit, long texture) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj != 0 && obj->core != 0) {
        m3gSetTexture((M3GAppearance) obj->core, unit,
                      (M3GTexture) funkey_m3g_core_object(texture));
        return;
    }
    if (obj != 0 && unit >= 0 && unit < FUNKEY_M3G_NUM_TEXTURE_UNITS) {
        obj->appearance_textures[unit] = texture;
    }
}

long
funkey_m3g_appearance_get_texture(long appearance, int unit) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj != 0 && obj->core != 0) {
        return funkey_m3g_wrap_core_object((M3GObject)
                                           m3gGetTexture((M3GAppearance) obj->core,
                                                         unit));
    }
    if (obj == 0 || unit < 0 || unit >= FUNKEY_M3G_NUM_TEXTURE_UNITS) {
        return 0;
    }
    return obj->appearance_textures[unit];
}

void
funkey_m3g_appearance_set_layer(long appearance, int layer) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetLayer((M3GAppearance) obj->core, layer);
        }
        obj->appearance_layer = layer;
    }
}

int
funkey_m3g_appearance_get_layer(long appearance) {
    FunKeyM3GObject *obj = funkey_m3g_object(appearance);
    if (obj != 0 && obj->core != 0) {
        return m3gGetLayer((M3GAppearance) obj->core);
    }
    return obj != 0 ? obj->appearance_layer : 0;
}

void
funkey_m3g_material_set_color(long material, int target, unsigned int argb) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        m3gSetColor((M3GMaterial) obj->core, target, argb);
    }
    if ((target & 1024) != 0) {
        obj->material_ambient = argb;
    }
    if ((target & 2048) != 0) {
        obj->material_diffuse = argb;
    }
    if ((target & 4096) != 0) {
        obj->material_emissive = argb;
    }
    if ((target & 8192) != 0) {
        obj->material_specular = argb;
    }
}

unsigned int
funkey_m3g_material_get_color(long material, int target) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        return m3gGetColor((M3GMaterial) obj->core, target);
    }
    if ((target & 1024) != 0) {
        return obj->material_ambient;
    }
    if ((target & 2048) != 0) {
        return obj->material_diffuse;
    }
    if ((target & 4096) != 0) {
        return obj->material_emissive;
    }
    if ((target & 8192) != 0) {
        return obj->material_specular;
    }
    return 0;
}

void
funkey_m3g_material_set_shininess(long material, float shininess) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetShininess((M3GMaterial) obj->core, shininess);
        }
        obj->material_shininess = shininess;
    }
}

float
funkey_m3g_material_get_shininess(long material) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj != 0 && obj->core != 0) {
        return m3gGetShininess((M3GMaterial) obj->core);
    }
    return obj != 0 ? obj->material_shininess : 0.0f;
}

void
funkey_m3g_material_set_vertex_color_tracking(long material, int enable) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetVertexColorTrackingEnable((M3GMaterial) obj->core,
                                            enable ? M3G_TRUE : M3G_FALSE);
        }
        obj->material_vertex_color_tracking = enable != 0;
    }
}

int
funkey_m3g_material_get_vertex_color_tracking(long material) {
    FunKeyM3GObject *obj = funkey_m3g_object(material);
    if (obj != 0 && obj->core != 0) {
        return m3gIsVertexColorTrackingEnabled((M3GMaterial) obj->core) != 0;
    }
    return obj != 0 ? obj->material_vertex_color_tracking : 0;
}

void
funkey_m3g_compositing_set_blending(long mode, int blending) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetBlending((M3GCompositingMode) obj->core, blending);
        }
        obj->compositing_blending = blending;
    }
}

int
funkey_m3g_compositing_get_blending(long mode) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj != 0 && obj->core != 0) {
        return m3gGetBlending((M3GCompositingMode) obj->core);
    }
    return obj != 0 ? obj->compositing_blending : 64;
}

void
funkey_m3g_compositing_set_alpha_threshold(long mode, float threshold) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetAlphaThreshold((M3GCompositingMode) obj->core, threshold);
        }
        obj->compositing_alpha_threshold = threshold;
    }
}

float
funkey_m3g_compositing_get_alpha_threshold(long mode) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj != 0 && obj->core != 0) {
        return m3gGetAlphaThreshold((M3GCompositingMode) obj->core);
    }
    return obj != 0 ? obj->compositing_alpha_threshold : 0.0f;
}

void
funkey_m3g_compositing_set_enable(long mode, int slot, int enable) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            m3gSetAlphaWriteEnable((M3GCompositingMode) obj->core,
                                   enable ? M3G_TRUE : M3G_FALSE);
        } else if (slot == 1) {
            m3gEnableDepthTest((M3GCompositingMode) obj->core,
                               enable ? M3G_TRUE : M3G_FALSE);
        } else if (slot == 2) {
            m3gEnableDepthWrite((M3GCompositingMode) obj->core,
                                enable ? M3G_TRUE : M3G_FALSE);
        } else if (slot == 3) {
            m3gEnableColorWrite((M3GCompositingMode) obj->core,
                                enable ? M3G_TRUE : M3G_FALSE);
        }
    }
    if (slot == 0) {
        obj->compositing_alpha_write = enable != 0;
    } else if (slot == 1) {
        obj->compositing_depth_test = enable != 0;
    } else if (slot == 2) {
        obj->compositing_depth_write = enable != 0;
    } else if (slot == 3) {
        obj->compositing_color_write = enable != 0;
    }
}

int
funkey_m3g_compositing_get_enable(long mode, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            return m3gIsAlphaWriteEnabled((M3GCompositingMode) obj->core) != 0;
        }
        if (slot == 1) {
            return m3gIsDepthTestEnabled((M3GCompositingMode) obj->core) != 0;
        }
        if (slot == 2) {
            return m3gIsDepthWriteEnabled((M3GCompositingMode) obj->core) != 0;
        }
        if (slot == 3) {
            return m3gIsColorWriteEnabled((M3GCompositingMode) obj->core) != 0;
        }
        return 0;
    }
    if (slot == 0) {
        return obj->compositing_alpha_write;
    }
    if (slot == 1) {
        return obj->compositing_depth_test;
    }
    if (slot == 2) {
        return obj->compositing_depth_write;
    }
    if (slot == 3) {
        return obj->compositing_color_write;
    }
    return 0;
}

void
funkey_m3g_compositing_set_depth_offset(long mode, float factor, float units) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetDepthOffset((M3GCompositingMode) obj->core, factor, units);
        }
        obj->compositing_depth_offset_factor = factor;
        obj->compositing_depth_offset_units = units;
    }
}

float
funkey_m3g_compositing_get_depth_offset(long mode, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(mode);
    if (obj == 0) {
        return 0.0f;
    }
    if (obj->core != 0) {
        return slot == 0 ?
               m3gGetDepthOffsetFactor((M3GCompositingMode) obj->core) :
               m3gGetDepthOffsetUnits((M3GCompositingMode) obj->core);
    }
    return slot == 0 ? obj->compositing_depth_offset_factor
                     : obj->compositing_depth_offset_units;
}

void
funkey_m3g_polygon_set_mode(long polygon, int slot, int mode) {
    FunKeyM3GObject *obj = funkey_m3g_object(polygon);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            m3gSetCulling((M3GPolygonMode) obj->core, mode);
        } else if (slot == 1) {
            m3gSetWinding((M3GPolygonMode) obj->core, mode);
        } else if (slot == 2) {
            m3gSetShading((M3GPolygonMode) obj->core, mode);
        }
    }
    if (slot == 0) {
        obj->polygon_culling = mode;
    } else if (slot == 1) {
        obj->polygon_winding = mode;
    } else if (slot == 2) {
        obj->polygon_shading = mode;
    }
}

int
funkey_m3g_polygon_get_mode(long polygon, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(polygon);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            return m3gGetCulling((M3GPolygonMode) obj->core);
        }
        if (slot == 1) {
            return m3gGetWinding((M3GPolygonMode) obj->core);
        }
        if (slot == 2) {
            return m3gGetShading((M3GPolygonMode) obj->core);
        }
        return 0;
    }
    if (slot == 0) {
        return obj->polygon_culling;
    }
    if (slot == 1) {
        return obj->polygon_winding;
    }
    if (slot == 2) {
        return obj->polygon_shading;
    }
    return 0;
}

void
funkey_m3g_polygon_set_enable(long polygon, int slot, int enable) {
    FunKeyM3GObject *obj = funkey_m3g_object(polygon);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            m3gSetTwoSidedLightingEnable((M3GPolygonMode) obj->core,
                                         enable ? M3G_TRUE : M3G_FALSE);
        } else if (slot == 1) {
            m3gSetLocalCameraLightingEnable((M3GPolygonMode) obj->core,
                                            enable ? M3G_TRUE : M3G_FALSE);
        } else if (slot == 2) {
            m3gSetPerspectiveCorrectionEnable((M3GPolygonMode) obj->core,
                                              enable ? M3G_TRUE : M3G_FALSE);
        }
    }
    if (slot == 0) {
        obj->polygon_two_sided_lighting = enable != 0;
    } else if (slot == 1) {
        obj->polygon_local_camera_lighting = enable != 0;
    } else if (slot == 2) {
        obj->polygon_perspective_correction = enable != 0;
    }
}

int
funkey_m3g_polygon_get_enable(long polygon, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(polygon);
    if (obj == 0) {
        return 0;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            return m3gIsTwoSidedLightingEnabled((M3GPolygonMode) obj->core) != 0;
        }
        if (slot == 1) {
            return m3gIsLocalCameraLightingEnabled((M3GPolygonMode) obj->core) != 0;
        }
        if (slot == 2) {
            return m3gIsPerspectiveCorrectionEnabled((M3GPolygonMode) obj->core) != 0;
        }
        return 0;
    }
    if (slot == 0) {
        return obj->polygon_two_sided_lighting;
    }
    if (slot == 1) {
        return obj->polygon_local_camera_lighting;
    }
    if (slot == 2) {
        return obj->polygon_perspective_correction;
    }
    return 0;
}

void
funkey_m3g_fog_set_mode(long fog, int mode) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetFogMode((M3GFog) obj->core, mode);
        }
        obj->fog_mode = mode;
    }
}

int
funkey_m3g_fog_get_mode(long fog) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0 && obj->core != 0) {
        return m3gGetFogMode((M3GFog) obj->core);
    }
    return obj != 0 ? obj->fog_mode : 80;
}

void
funkey_m3g_fog_set_linear(long fog, float near_distance, float far_distance) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetFogLinear((M3GFog) obj->core, near_distance, far_distance);
        }
        obj->fog_near = near_distance;
        obj->fog_far = far_distance;
    }
}

float
funkey_m3g_fog_get_distance(long fog, int which) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj == 0) {
        return 0.0f;
    }
    if (obj->core != 0) {
        return m3gGetFogDistance((M3GFog) obj->core, which);
    }
    return which == 0 ? obj->fog_near : obj->fog_far;
}

void
funkey_m3g_fog_set_density(long fog, float density) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetFogDensity((M3GFog) obj->core, density);
        }
        obj->fog_density = density;
    }
}

float
funkey_m3g_fog_get_density(long fog) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0 && obj->core != 0) {
        return m3gGetFogDensity((M3GFog) obj->core);
    }
    return obj != 0 ? obj->fog_density : 1.0f;
}

void
funkey_m3g_fog_set_color(long fog, unsigned int rgb) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetFogColor((M3GFog) obj->core, rgb);
        }
        obj->fog_color = rgb;
    }
}

unsigned int
funkey_m3g_fog_get_color(long fog) {
    FunKeyM3GObject *obj = funkey_m3g_object(fog);
    if (obj != 0 && obj->core != 0) {
        return m3gGetFogColor((M3GFog) obj->core);
    }
    return obj != 0 ? obj->fog_color : 0;
}

void
funkey_m3g_light_set_intensity(long light, float intensity) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetIntensity((M3GLight) obj->core, intensity);
        }
        obj->light_intensity = intensity;
    }
}

float
funkey_m3g_light_get_intensity(long light) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0 && obj->core != 0) {
        return m3gGetIntensity((M3GLight) obj->core);
    }
    return obj != 0 ? obj->light_intensity : 1.0f;
}

void
funkey_m3g_light_set_color(long light, unsigned int rgb) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetLightColor((M3GLight) obj->core, rgb);
        }
        obj->light_color = rgb;
    }
}

unsigned int
funkey_m3g_light_get_color(long light) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0 && obj->core != 0) {
        return m3gGetLightColor((M3GLight) obj->core);
    }
    return obj != 0 ? obj->light_color : 0x00ffffffU;
}

void
funkey_m3g_light_set_mode(long light, int mode) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetLightMode((M3GLight) obj->core, mode);
        }
        obj->light_mode = mode;
    }
}

int
funkey_m3g_light_get_mode(long light) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0 && obj->core != 0) {
        return m3gGetLightMode((M3GLight) obj->core);
    }
    return obj != 0 ? obj->light_mode : 130;
}

void
funkey_m3g_light_set_spot(long light, int slot, float value) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj == 0) {
        return;
    }
    if (obj->core != 0) {
        if (slot == 0) {
            m3gSetSpotAngle((M3GLight) obj->core, value);
        } else {
            m3gSetSpotExponent((M3GLight) obj->core, value);
        }
    }
    if (slot == 0) {
        obj->light_spot_angle = value;
    } else {
        obj->light_spot_exponent = value;
    }
}

float
funkey_m3g_light_get_spot(long light, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj == 0) {
        return slot == 0 ? 45.0f : 0.0f;
    }
    if (obj->core != 0) {
        return slot == 0 ? m3gGetSpotAngle((M3GLight) obj->core) :
                           m3gGetSpotExponent((M3GLight) obj->core);
    }
    return slot == 0 ? obj->light_spot_angle : obj->light_spot_exponent;
}

void
funkey_m3g_light_set_attenuation(long light, float constant,
                                 float linear, float quadratic) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj != 0) {
        if (obj->core != 0) {
            m3gSetAttenuation((M3GLight) obj->core,
                              constant, linear, quadratic);
        }
        obj->light_attenuation[0] = constant;
        obj->light_attenuation[1] = linear;
        obj->light_attenuation[2] = quadratic;
    }
}

float
funkey_m3g_light_get_attenuation(long light, int slot) {
    FunKeyM3GObject *obj = funkey_m3g_object(light);
    if (obj == 0 || slot < 0 || slot > 2) {
        return slot == 0 ? 1.0f : 0.0f;
    }
    if (obj->core != 0) {
        return m3gGetAttenuation((M3GLight) obj->core, slot);
    }
    return obj->light_attenuation[slot];
}

void
funkey_m3g_surface_init(FunKeyM3GSurface *surface,
                        int width, int height,
                        int clip_x, int clip_y,
                        int clip_w, int clip_h) {
    unsigned short *pixels;
    int stride;
    float *depth;
    int depth_count;
    int requested_depth;
    if (surface == 0) {
        return;
    }

    pixels = surface->pixels;
    stride = surface->stride;
    depth = surface->depth;
    depth_count = surface->depth_count;
    requested_depth = width > 0 && height > 0 ? width * height : 0;
    if (requested_depth != depth_count) {
        if (depth != 0) {
            free(depth);
            depth = 0;
        }
        depth_count = 0;
        if (requested_depth > 0) {
            depth = (float *) malloc((size_t) requested_depth * sizeof(float));
            if (depth != 0) {
                depth_count = requested_depth;
            }
        }
    }
    surface->width = width;
    surface->height = height;
    surface->clip_x = clip_x;
    surface->clip_y = clip_y;
    surface->clip_w = clip_w;
    surface->clip_h = clip_h;
    surface->clear_argb = 0xff000000U;
    surface->pixels = pixels;
    surface->stride = stride;
    surface->depth = depth;
    surface->depth_count = depth_count;
}

void
funkey_m3g_surface_bind_pixels(FunKeyM3GSurface *surface,
                               unsigned short *pixels, int stride) {
    if (surface == 0) {
        return;
    }
    surface->pixels = pixels;
    surface->stride = stride;
}

static unsigned short
funkey_m3g_argb_to_rgb565(unsigned int argb) {
    unsigned int r = (argb >> 16) & 0xffU;
    unsigned int g = (argb >> 8) & 0xffU;
    unsigned int b = argb & 0xffU;
    return (unsigned short) (((r & 0xf8U) << 8) |
                             ((g & 0xfcU) << 3) |
                             (b >> 3));
}

static unsigned int
funkey_m3g_image_argb(FunKeyM3GObject *image, int x, int y) {
    int bpp;
    int offset;
    const unsigned char *source;
    unsigned int a = 0xffU;
    unsigned int r = 0xffU;
    unsigned int g = 0xffU;
    unsigned int b = 0xffU;
    if (image == 0 || image->image_pixels == 0 ||
            image->image_width <= 0 || image->image_height <= 0 ||
            x < 0 || y < 0 || x >= image->image_width ||
            y >= image->image_height) {
        return 0x00000000U;
    }
    switch (image->image_format) {
    case 96:
    case 97:
        bpp = 1;
        break;
    case 98:
        bpp = 2;
        break;
    case 99:
        bpp = 3;
        break;
    case 100:
        bpp = 4;
        break;
    default:
        return 0xffffffffU;
    }
    if (image->image_palette != 0 && image->image_palette_count >= bpp) {
        offset = (int) image->image_pixels[y * image->image_width + x] * bpp;
        if (offset < 0 || offset + bpp > image->image_palette_count) {
            return 0x00000000U;
        }
        source = image->image_palette + offset;
    } else {
        offset = (y * image->image_width + x) * bpp;
        if (offset < 0 || offset + bpp > image->image_pixel_count) {
            return 0x00000000U;
        }
        source = image->image_pixels + offset;
    }
    if (image->image_format == 96) {
        a = source[0];
    } else if (image->image_format == 97) {
        r = g = b = source[0];
    } else if (image->image_format == 98) {
        r = g = b = source[0];
        a = source[1];
    } else if (image->image_format == 99) {
        r = source[0];
        g = source[1];
        b = source[2];
    } else {
        r = source[0];
        g = source[1];
        b = source[2];
        a = source[3];
    }
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static int
funkey_m3g_wrap_texel(int coord, int extent, int wrap) {
    if (extent <= 0) {
        return 0;
    }
    if (wrap == 241) {
        coord %= extent;
        if (coord < 0) {
            coord += extent;
        }
        return coord;
    }
    if (coord < 0) {
        return 0;
    }
    return coord >= extent ? extent - 1 : coord;
}

static void funkey_m3g_transform_point(const float *m, float x, float y,
                                       float z, float *out);
static void funkey_m3g_local_matrix(FunKeyM3GObject *obj, float *out);

static unsigned int
funkey_m3g_sample_texture(FunKeyM3GObject *texture, float u, float v) {
    FunKeyM3GObject *image;
    float texture_matrix[16];
    float transformed[3];
    int x;
    int y;
    if (texture == 0 || texture->texture_image == 0) {
        return 0xffffffffU;
    }
    image = funkey_m3g_object(texture->texture_image);
    if (image == 0 || image->image_width <= 0 || image->image_height <= 0) {
        return 0xffffffffU;
    }
    /*
     * Texture2D is Transformable: authored scenes use this transform to
     * select atlas areas and orient textured road and vehicle geometry.
     */
    funkey_m3g_local_matrix(texture, texture_matrix);
    funkey_m3g_transform_point(texture_matrix, u, v, 0.0f, transformed);
    u = transformed[0];
    v = transformed[1];
    x = (int) floorf(u * (float) image->image_width);
    y = (int) floorf((1.0f - v) * (float) image->image_height);
    x = funkey_m3g_wrap_texel(x, image->image_width, texture->texture_wrap_s);
    y = funkey_m3g_wrap_texel(y, image->image_height, texture->texture_wrap_t);
    return funkey_m3g_image_argb(image, x, y);
}

static void
funkey_m3g_put_pixel(FunKeyM3GSurface *surface, int x, int y,
                     unsigned short color) {
    if (surface == 0 || surface->pixels == 0 || surface->stride <= 0) {
        return;
    }
    if (x < surface->clip_x || y < surface->clip_y ||
            x >= surface->clip_x + surface->clip_w ||
            y >= surface->clip_y + surface->clip_h ||
            x < 0 || y < 0 || x >= surface->width || y >= surface->height) {
        return;
    }
    surface->pixels[y * surface->stride + x] = color;
}

static void
funkey_m3g_put_argb(FunKeyM3GSurface *surface, int x, int y,
                    unsigned int argb) {
    unsigned int alpha;
    unsigned short dst;
    unsigned int sr;
    unsigned int sg;
    unsigned int sb;
    unsigned int dr;
    unsigned int dg;
    unsigned int db;
    unsigned int out;
    if (surface == 0 || surface->pixels == 0 || surface->stride <= 0 ||
            x < surface->clip_x || y < surface->clip_y ||
            x >= surface->clip_x + surface->clip_w ||
            y >= surface->clip_y + surface->clip_h ||
            x < 0 || y < 0 || x >= surface->width || y >= surface->height) {
        return;
    }
    alpha = (argb >> 24) & 0xffU;
    if (alpha == 0) {
        return;
    }
    if (alpha == 0xffU) {
        surface->pixels[y * surface->stride + x] =
            funkey_m3g_argb_to_rgb565(argb);
        return;
    }
    dst = surface->pixels[y * surface->stride + x];
    sr = (argb >> 16) & 0xffU;
    sg = (argb >> 8) & 0xffU;
    sb = argb & 0xffU;
    dr = ((dst >> 11) & 0x1fU) * 255U / 31U;
    dg = ((dst >> 5) & 0x3fU) * 255U / 63U;
    db = (dst & 0x1fU) * 255U / 31U;
    out = 0xff000000U |
          (((sr * alpha + dr * (255U - alpha)) / 255U) << 16) |
          (((sg * alpha + dg * (255U - alpha)) / 255U) << 8) |
          ((sb * alpha + db * (255U - alpha)) / 255U);
    surface->pixels[y * surface->stride + x] =
        funkey_m3g_argb_to_rgb565(out);
}

void
funkey_m3g_surface_clear(FunKeyM3GSurface *surface, unsigned int argb) {
    int x;
    int y;
    int i;
    int x0;
    int y0;
    int x1;
    int y1;
    unsigned short color;
    if (surface == 0) {
        return;
    }

    surface->clear_argb = argb;
    if (surface->pixels == 0 || surface->stride <= 0) {
        return;
    }
    color = funkey_m3g_argb_to_rgb565(argb);
    for (i = 0; i < surface->depth_count; ++i) {
        surface->depth[i] = 1.0e30f;
    }
    x0 = surface->clip_x < 0 ? 0 : surface->clip_x;
    y0 = surface->clip_y < 0 ? 0 : surface->clip_y;
    x1 = surface->clip_x + surface->clip_w;
    y1 = surface->clip_y + surface->clip_h;
    if (x1 > surface->width) {
        x1 = surface->width;
    }
    if (y1 > surface->height) {
        y1 = surface->height;
    }
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            surface->pixels[y * surface->stride + x] = color;
        }
    }
}

static void
funkey_m3g_draw_line(FunKeyM3GSurface *surface, int x0, int y0,
                     int x1, int y1, unsigned short color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        int e2;
        funkey_m3g_put_pixel(surface, x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static int
funkey_m3g_project_coord(FunKeyM3GSurface *surface, float v, int axis) {
    float half = axis == 0 ? (float) surface->width * 0.5f
                           : (float) surface->height * 0.5f;
    if (v >= -1.5f && v <= 1.5f) {
        return (int) (half + v * half);
    }
    return (int) (half + v);
}

typedef struct FunKeyM3GProjectedVertex {
    int x;
    int y;
    float z;
    float u;
    float v;
    float inv_z;
    int visible;
} FunKeyM3GProjectedVertex;

typedef struct FunKeyM3GCoreTexture {
    unsigned int *argb;
    int width;
    int height;
    int wrap_s;
    int wrap_t;
} FunKeyM3GCoreTexture;

typedef struct FunKeyM3GRenderState {
    float view[16];
    float fovy;
    float aspect;
    float near_plane;
    int has_camera;
} FunKeyM3GRenderState;

static void
funkey_m3g_matrix_identity(float *m) {
    int i;
    for (i = 0; i < 16; ++i) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void
funkey_m3g_matrix_mul(float *dst, const float *a, const float *b) {
    float out[16];
    int r;
    int c;
    int k;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            float v = 0.0f;
            for (k = 0; k < 4; ++k) {
                v += a[r * 4 + k] * b[k * 4 + c];
            }
            out[r * 4 + c] = v;
        }
    }
    memcpy(dst, out, sizeof(out));
}

static int
funkey_m3g_matrix_invert(float *dst, const float *m) {
    float inv[16];
    float det;
    int i;
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] -
             m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
             m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] -
             m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
              m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
             m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] -
             m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] +
             m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] -
              m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] -
             m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] +
             m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] -
              m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] +
              m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] +
             m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] +
              m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (det > -0.000001f && det < 0.000001f) {
        return 0;
    }
    det = 1.0f / det;
    for (i = 0; i < 16; ++i) {
        dst[i] = inv[i] * det;
    }
    return 1;
}

static void
funkey_m3g_transform_point(const float *m, float x, float y, float z,
                           float *out) {
    out[0] = m[0] * x + m[1] * y + m[2] * z + m[3];
    out[1] = m[4] * x + m[5] * y + m[6] * z + m[7];
    out[2] = m[8] * x + m[9] * y + m[10] * z + m[11];
}

static void
funkey_m3g_local_matrix(FunKeyM3GObject *obj, float *out) {
    float component[16];
    float scale[16];
    float translate[16];
    float rotated[16];
    float axis_len;
    float angle;
    float c;
    float s;
    float t;
    float x;
    float y;
    float z;
    funkey_m3g_matrix_identity(component);
    if (obj == 0) {
        funkey_m3g_matrix_identity(out);
        return;
    }
    axis_len = sqrtf(obj->orientation[1] * obj->orientation[1] +
                     obj->orientation[2] * obj->orientation[2] +
                     obj->orientation[3] * obj->orientation[3]);
    if (axis_len > 0.000001f && obj->orientation[0] != 0.0f) {
        angle = obj->orientation[0] * (float) M_PI / 180.0f;
        c = cosf(angle);
        s = sinf(angle);
        t = 1.0f - c;
        x = obj->orientation[1] / axis_len;
        y = obj->orientation[2] / axis_len;
        z = obj->orientation[3] / axis_len;
        component[0] = t * x * x + c;
        component[1] = t * x * y - s * z;
        component[2] = t * x * z + s * y;
        component[4] = t * x * y + s * z;
        component[5] = t * y * y + c;
        component[6] = t * y * z - s * x;
        component[8] = t * x * z - s * y;
        component[9] = t * y * z + s * x;
        component[10] = t * z * z + c;
    }
    funkey_m3g_matrix_identity(scale);
    scale[0] = obj->scale[0];
    scale[5] = obj->scale[1];
    scale[10] = obj->scale[2];
    funkey_m3g_matrix_mul(rotated, component, scale);
    funkey_m3g_matrix_identity(translate);
    translate[3] = obj->translation[0];
    translate[7] = obj->translation[1];
    translate[11] = obj->translation[2];
    funkey_m3g_matrix_mul(component, translate, rotated);
    funkey_m3g_matrix_mul(out, component, obj->transform);
}

static void
funkey_m3g_world_matrix(long handle, float *out) {
    FunKeyM3GObject *obj = funkey_m3g_object(handle);
    float local[16];
    float parent[16];
    if (obj == 0) {
        funkey_m3g_matrix_identity(out);
        return;
    }
    funkey_m3g_local_matrix(obj, local);
    if (obj->parent != 0) {
        funkey_m3g_world_matrix(obj->parent, parent);
        funkey_m3g_matrix_mul(out, parent, local);
    } else {
        memcpy(out, local, sizeof(local));
    }
}

static void
funkey_m3g_get_projected_vertex(FunKeyM3GSurface *surface,
                                FunKeyM3GObject *array,
                                int index, float scale, const float *bias,
                                const float *model_view,
                                const FunKeyM3GRenderState *state,
                                FunKeyM3GProjectedVertex *out) {
    int offset;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float view[3];
    float zc;
    float fovy;
    float aspect;
    float ndc_x;
    float ndc_y;
    if (array == 0 || array->components == 0 ||
            index < 0 || index >= array->vertex_count) {
        out->x = surface->width / 2;
        out->y = surface->height / 2;
        out->z = 0.0f;
        out->u = 0.0f;
        out->v = 0.0f;
        out->inv_z = 1.0f;
        out->visible = 0;
        return;
    }
    offset = index * array->component_count;
    if (array->component_count > 0) {
        vx = (float) array->components[offset] * scale + bias[0];
    }
    if (array->component_count > 1) {
        vy = (float) array->components[offset + 1] * scale + bias[1];
    }
    if (array->component_count > 2) {
        vz = (float) array->components[offset + 2] * scale + bias[2];
    }
    funkey_m3g_transform_point(model_view, vx, vy, vz, view);
    zc = -view[2];
    if (!state->has_camera) {
        out->x = funkey_m3g_project_coord(surface, view[0], 0);
        out->y = funkey_m3g_project_coord(surface, -view[1], 1);
        out->z = view[2];
        out->inv_z = 1.0f;
        out->visible = 1;
        return;
    }
    if (zc <= state->near_plane) {
        out->visible = 0;
        return;
    }
    fovy = state->fovy > 1.0f ? state->fovy : 45.0f;
    aspect = state->aspect > 0.001f ? state->aspect :
             surface->height != 0 ? (float) surface->width / (float) surface->height : 1.0f;
    ndc_y = view[1] / (zc * tanf(fovy * (float) M_PI / 360.0f));
    ndc_x = view[0] / (zc * tanf(fovy * (float) M_PI / 360.0f) * aspect);
    out->x = (int) ((float) surface->clip_x +
                    ((ndc_x + 1.0f) * 0.5f) * (float) surface->clip_w);
    out->y = (int) ((float) surface->clip_y +
                    ((1.0f - ndc_y) * 0.5f) * (float) surface->clip_h);
    out->z = zc;
    out->inv_z = 1.0f / zc;
    out->visible = 1;
}

static void
funkey_m3g_get_texcoord(FunKeyM3GObject *array, int index,
                        const float *scale_bias,
                        FunKeyM3GProjectedVertex *out) {
    int offset;
    if (array == 0 || array->components == 0 ||
            index < 0 || index >= array->vertex_count) {
        out->u = 0.0f;
        out->v = 0.0f;
        return;
    }
    offset = index * array->component_count;
    out->u = (float) array->components[offset] * scale_bias[0] +
             scale_bias[1];
    out->v = array->component_count > 1 ?
             (float) array->components[offset + 1] * scale_bias[0] +
             scale_bias[2] : 0.0f;
}

static int
funkey_m3g_min3(int a, int b, int c) {
    int m = a < b ? a : b;
    return m < c ? m : c;
}

static int
funkey_m3g_max3(int a, int b, int c) {
    int m = a > b ? a : b;
    return m > c ? m : c;
}

static unsigned short
funkey_m3g_mesh_color(FunKeyM3GObject *mesh, FunKeyM3GObject *vb, int submesh) {
    unsigned int argb = vb != 0 ? vb->default_color : 0xffffffffU;
    FunKeyM3GObject *appearance = 0;
    FunKeyM3GObject *material = 0;
    if (mesh != 0 && submesh >= 0 && submesh < mesh->mesh_submesh_count) {
        appearance = funkey_m3g_object(mesh->mesh_appearances[submesh]);
    }
    if (appearance != 0) {
        material = funkey_m3g_object(appearance->appearance_material);
    }
    if (material != 0) {
        argb = material->material_diffuse;
    }
    return funkey_m3g_argb_to_rgb565(argb);
}

static void
funkey_m3g_fill_triangle(FunKeyM3GSurface *surface,
                         const FunKeyM3GProjectedVertex *a,
                         const FunKeyM3GProjectedVertex *b,
                         const FunKeyM3GProjectedVertex *c,
                         unsigned short color,
                         FunKeyM3GObject *texture) {
    int x;
    int y;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    float area;
    if (surface == 0 || surface->pixels == 0 || surface->stride <= 0) {
        return;
    }
    area = (float) ((b->x - a->x) * (c->y - a->y) -
                    (b->y - a->y) * (c->x - a->x));
    if (area == 0.0f) {
        return;
    }
    min_x = funkey_m3g_min3(a->x, b->x, c->x);
    min_y = funkey_m3g_min3(a->y, b->y, c->y);
    max_x = funkey_m3g_max3(a->x, b->x, c->x);
    max_y = funkey_m3g_max3(a->y, b->y, c->y);
    if (min_x < surface->clip_x) min_x = surface->clip_x;
    if (min_y < surface->clip_y) min_y = surface->clip_y;
    if (max_x >= surface->clip_x + surface->clip_w) {
        max_x = surface->clip_x + surface->clip_w - 1;
    }
    if (max_y >= surface->clip_y + surface->clip_h) {
        max_y = surface->clip_y + surface->clip_h - 1;
    }
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= surface->width) max_x = surface->width - 1;
    if (max_y >= surface->height) max_y = surface->height - 1;

    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            float px = (float) x + 0.5f;
            float py = (float) y + 0.5f;
            float w0 = ((b->x - px) * (c->y - py) -
                        (b->y - py) * (c->x - px)) / area;
            float w1 = ((c->x - px) * (a->y - py) -
                        (c->y - py) * (a->x - px)) / area;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                int offset = y * surface->stride + x;
                int depth_offset = y * surface->width + x;
                float z = w0 * a->z + w1 * b->z + w2 * c->z;
                if (surface->depth == 0 ||
                        depth_offset >= surface->depth_count ||
                        z <= surface->depth[depth_offset]) {
                    if (texture != 0) {
                        float recip = w0 * a->inv_z + w1 * b->inv_z +
                                      w2 * c->inv_z;
                        float u;
                        float v;
                        if (recip != 0.0f) {
                            u = (w0 * a->u * a->inv_z +
                                 w1 * b->u * b->inv_z +
                                 w2 * c->u * c->inv_z) / recip;
                            v = (w0 * a->v * a->inv_z +
                                 w1 * b->v * b->inv_z +
                                 w2 * c->v * c->inv_z) / recip;
                            funkey_m3g_put_argb(surface, x, y,
                                               funkey_m3g_sample_texture(texture,
                                                                         u, v));
                        }
                    } else {
                        surface->pixels[offset] = color;
                    }
                    if (surface->depth != 0 &&
                            depth_offset < surface->depth_count) {
                        surface->depth[depth_offset] = z;
                    }
                }
            }
        }
    }
}

static void
funkey_m3g_surface_render_mesh_state(FunKeyM3GSurface *surface, long mesh,
                                     const float *world,
                                     const FunKeyM3GRenderState *state) {
    FunKeyM3GObject *m = funkey_m3g_object(mesh);
    FunKeyM3GObject *vb;
    FunKeyM3GObject *positions;
    float model_view[16];
    int sub;
    if (surface == 0 || m == 0 || m->class_id != FUNKEY_M3G_CLASS_MESH) {
        return;
    }
    vb = funkey_m3g_object(m->mesh_vertices);
    if (vb == 0) {
        return;
    }
    positions = funkey_m3g_object(vb->vb_positions);
    if (positions == 0) {
        return;
    }
    funkey_m3g_matrix_mul(model_view, state->view, world);
    if (g_mesh_trace_count < 48) {
        int textured = 0;
        for (sub = 0; sub < m->mesh_submesh_count; ++sub) {
            FunKeyM3GObject *app = funkey_m3g_object(m->mesh_appearances[sub]);
            if (app != 0 && app->appearance_textures[0] != 0 &&
                    vb->vb_texcoords[0] != 0) {
                ++textured;
            }
        }
        fprintf(stderr, "[M3G raster] mesh=%ld sub=%d textured=%d vertices=%d\n",
                mesh, m->mesh_submesh_count, textured,
                positions->vertex_count);
        ++g_mesh_trace_count;
    }
    for (sub = 0; sub < m->mesh_submesh_count; ++sub) {
        FunKeyM3GObject *idx = funkey_m3g_object(m->mesh_indices[sub]);
        FunKeyM3GObject *appearance = funkey_m3g_object(m->mesh_appearances[sub]);
        FunKeyM3GObject *texture = appearance != 0 ?
            funkey_m3g_object(appearance->appearance_textures[0]) : 0;
        FunKeyM3GObject *texcoords = vb->vb_texcoords[0] != 0 ?
            funkey_m3g_object(vb->vb_texcoords[0]) : 0;
        unsigned short color = funkey_m3g_mesh_color(m, vb, sub);
        int i;
        if (texture == 0 || texture->texture_image == 0 || texcoords == 0) {
            texture = 0;
        }
        if (idx == 0 || idx->indices == 0) {
            continue;
        }
        for (i = 0; i + 2 < idx->index_count; i += 3) {
            FunKeyM3GProjectedVertex a, b, c;
            funkey_m3g_get_projected_vertex(surface, positions,
                                            idx->indices[i],
                                            vb->vb_position_scale_bias[0],
                                            &vb->vb_position_scale_bias[1],
                                            model_view, state,
                                            &a);
            funkey_m3g_get_projected_vertex(surface, positions,
                                            idx->indices[i + 1],
                                            vb->vb_position_scale_bias[0],
                                            &vb->vb_position_scale_bias[1],
                                            model_view, state,
                                            &b);
            funkey_m3g_get_projected_vertex(surface, positions,
                                            idx->indices[i + 2],
                                            vb->vb_position_scale_bias[0],
                                            &vb->vb_position_scale_bias[1],
                                            model_view, state,
                                            &c);
            if (texture != 0) {
                funkey_m3g_get_texcoord(texcoords, idx->indices[i],
                                        vb->vb_texcoord_scale_bias[0], &a);
                funkey_m3g_get_texcoord(texcoords, idx->indices[i + 1],
                                        vb->vb_texcoord_scale_bias[0], &b);
                funkey_m3g_get_texcoord(texcoords, idx->indices[i + 2],
                                        vb->vb_texcoord_scale_bias[0], &c);
            }
            if (!a.visible || !b.visible || !c.visible) {
                continue;
            }
            funkey_m3g_fill_triangle(surface, &a, &b, &c, color, texture);
            if (texture == 0) {
                funkey_m3g_draw_line(surface, a.x, a.y, b.x, b.y, color);
                funkey_m3g_draw_line(surface, b.x, b.y, c.x, c.y, color);
                funkey_m3g_draw_line(surface, c.x, c.y, a.x, a.y, color);
            }
        }
    }
}

static void
funkey_m3g_core_matrix_rows(M3GMatrix *matrix, float *out) {
    m3gGetMatrixRows(matrix, out);
}

static void
funkey_m3g_core_project(FunKeyM3GSurface *surface, const float *model_view,
                        const FunKeyM3GRenderState *state,
                        float x, float y, float z,
                        FunKeyM3GProjectedVertex *out) {
    float view[3];
    float zc;
    float fovy;
    float aspect;
    float ndc_x;
    float ndc_y;
    out->u = 0.0f;
    out->v = 0.0f;
    funkey_m3g_transform_point(model_view, x, y, z, view);
    zc = -view[2];
    if (!state->has_camera) {
        out->x = funkey_m3g_project_coord(surface, view[0], 0);
        out->y = funkey_m3g_project_coord(surface, -view[1], 1);
        out->z = view[2];
        out->inv_z = 1.0f;
        out->visible = 1;
        return;
    }
    if (zc <= state->near_plane) {
        out->visible = 0;
        return;
    }
    fovy = state->fovy > 1.0f ? state->fovy : 45.0f;
    aspect = state->aspect > 0.001f ? state->aspect :
             surface->height != 0 ? (float) surface->width / (float) surface->height : 1.0f;
    ndc_y = view[1] / (zc * tanf(fovy * (float) M_PI / 360.0f));
    ndc_x = view[0] / (zc * tanf(fovy * (float) M_PI / 360.0f) * aspect);
    out->x = (int) ((float) surface->clip_x +
                    ((ndc_x + 1.0f) * 0.5f) * (float) surface->clip_w);
    out->y = (int) ((float) surface->clip_y +
                    ((1.0f - ndc_y) * 0.5f) * (float) surface->clip_h);
    out->z = zc;
    out->inv_z = 1.0f / zc;
    out->visible = 1;
}

static void
funkey_m3g_core_free_texture(FunKeyM3GCoreTexture *texture) {
    if (texture != 0 && texture->argb != 0) {
        free(texture->argb);
        texture->argb = 0;
    }
}

static int
funkey_m3g_core_load_texture(M3GAppearance app, FunKeyM3GCoreTexture *out) {
    M3GTexture texture;
    M3GImage image;
    size_t pixels;
    if (out == 0) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    if (app == 0) {
        return 0;
    }
    texture = m3gGetTexture(app, 0);
    if (texture == 0) {
        return 0;
    }
    image = m3gGetTextureImage(texture);
    if (image == 0) {
        return 0;
    }
    out->width = m3gGetWidth(image);
    out->height = m3gGetHeight(image);
    out->wrap_s = m3gGetWrappingS(texture);
    out->wrap_t = m3gGetWrappingT(texture);
    if (out->width <= 0 || out->height <= 0) {
        return 0;
    }
    pixels = (size_t) out->width * (size_t) out->height;
    out->argb = (unsigned int *) malloc(pixels * sizeof(unsigned int));
    if (out->argb == 0) {
        return 0;
    }
    m3gGetImageARGB(image, (M3Guint *) out->argb);
    return 1;
}

static unsigned int
funkey_m3g_core_sample_texture(const FunKeyM3GCoreTexture *texture,
                               float u, float v) {
    int x;
    int y;
    if (texture == 0 || texture->argb == 0 ||
            texture->width <= 0 || texture->height <= 0) {
        return 0xffffffffU;
    }
    x = (int) floorf(u * (float) texture->width);
    y = (int) floorf((1.0f - v) * (float) texture->height);
    x = funkey_m3g_wrap_texel(x, texture->width, texture->wrap_s);
    y = funkey_m3g_wrap_texel(y, texture->height, texture->wrap_t);
    return texture->argb[y * texture->width + x];
}

static void
funkey_m3g_core_fill_triangle(FunKeyM3GSurface *surface,
                              const FunKeyM3GProjectedVertex *a,
                              const FunKeyM3GProjectedVertex *b,
                              const FunKeyM3GProjectedVertex *c,
                              unsigned short color,
                              const FunKeyM3GCoreTexture *texture) {
    int x;
    int y;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    float area;
    if (surface == 0 || surface->pixels == 0 || surface->stride <= 0 ||
            !a->visible || !b->visible || !c->visible) {
        return;
    }
    area = (float) ((b->x - a->x) * (c->y - a->y) -
                    (b->y - a->y) * (c->x - a->x));
    if (area == 0.0f) {
        return;
    }
    min_x = funkey_m3g_min3(a->x, b->x, c->x);
    min_y = funkey_m3g_min3(a->y, b->y, c->y);
    max_x = funkey_m3g_max3(a->x, b->x, c->x);
    max_y = funkey_m3g_max3(a->y, b->y, c->y);
    if (min_x < surface->clip_x) min_x = surface->clip_x;
    if (min_y < surface->clip_y) min_y = surface->clip_y;
    if (max_x >= surface->clip_x + surface->clip_w) {
        max_x = surface->clip_x + surface->clip_w - 1;
    }
    if (max_y >= surface->clip_y + surface->clip_h) {
        max_y = surface->clip_y + surface->clip_h - 1;
    }
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= surface->width) max_x = surface->width - 1;
    if (max_y >= surface->height) max_y = surface->height - 1;

    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            float px = (float) x + 0.5f;
            float py = (float) y + 0.5f;
            float w0 = ((b->x - px) * (c->y - py) -
                        (b->y - py) * (c->x - px)) / area;
            float w1 = ((c->x - px) * (a->y - py) -
                        (c->y - py) * (a->x - px)) / area;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                int depth_offset = y * surface->width + x;
                float z = w0 * a->z + w1 * b->z + w2 * c->z;
                if (surface->depth == 0 ||
                        depth_offset >= surface->depth_count ||
                        z <= surface->depth[depth_offset]) {
                    if (texture != 0 && texture->argb != 0) {
                        float recip = w0 * a->inv_z + w1 * b->inv_z +
                                      w2 * c->inv_z;
                        if (recip != 0.0f) {
                            float u = (w0 * a->u * a->inv_z +
                                       w1 * b->u * b->inv_z +
                                       w2 * c->u * c->inv_z) / recip;
                            float v = (w0 * a->v * a->inv_z +
                                       w1 * b->v * b->inv_z +
                                       w2 * c->v * c->inv_z) / recip;
                            funkey_m3g_put_argb(surface, x, y,
                                funkey_m3g_core_sample_texture(texture, u, v));
                        }
                    } else {
                        surface->pixels[y * surface->stride + x] = color;
                    }
                    if (surface->depth != 0 &&
                            depth_offset < surface->depth_count) {
                        surface->depth[depth_offset] = z;
                    }
                }
            }
        }
    }
}

static void
funkey_m3g_core_draw_triangle(FunKeyM3GSurface *surface,
                              const FunKeyM3GProjectedVertex *a,
                              const FunKeyM3GProjectedVertex *b,
                              const FunKeyM3GProjectedVertex *c,
                              unsigned short color,
                              const FunKeyM3GCoreTexture *texture) {
    if (!a->visible || !b->visible || !c->visible) {
        return;
    }
    funkey_m3g_core_fill_triangle(surface, a, b, c, color, texture);
    if (texture == 0 || texture->argb == 0) {
        funkey_m3g_draw_line(surface, a->x, a->y, b->x, b->y, color);
        funkey_m3g_draw_line(surface, b->x, b->y, c->x, c->y, color);
        funkey_m3g_draw_line(surface, c->x, c->y, a->x, a->y, color);
    }
}

static void
funkey_m3g_core_render_mesh(FunKeyM3GSurface *surface, M3GMesh mesh,
                            const FunKeyM3GRenderState *state) {
    M3GVertexBuffer vb;
    M3GVertexArray pos;
    M3GMatrix composite;
    float world[16];
    float model_view[16];
    float scale_bias[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    M3Gsizei count = 0;
    M3Gsizei stride = 0;
    M3Gint size = 0;
    M3Gdatatype type = M3G_FLOAT;
    float *xyz;
    M3GVertexArray texcoord;
    float tex_scale_bias[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    float *uv = 0;
    M3Gsizei tex_count = 0;
    M3Gint tex_size = 0;
    M3Gsizei tex_stride = 0;
    M3Gdatatype tex_type = M3G_FLOAT;
    int sub_count;
    int sub;
    if (surface == 0 || mesh == 0) {
        return;
    }
    vb = m3gGetVertexBuffer(mesh);
    if (vb == 0) {
        return;
    }
    pos = m3gGetVertexArray(vb, M3G_GET_POSITIONS, scale_bias, 4);
    if (pos == 0) {
        return;
    }
    m3gGetVertexArrayParams(pos, &count, &size, &type, &stride);
    if (count <= 0 || size < 2) {
        return;
    }
    xyz = (float *) malloc((size_t) count * 3U * sizeof(float));
    if (xyz == 0) {
        return;
    }
    memset(xyz, 0, (size_t) count * 3U * sizeof(float));
    m3gGetVertexArrayElements(pos, 0, count,
                              (M3Gsizei) ((size_t) count * (size_t) size * sizeof(float)),
                              M3G_FLOAT, xyz);
    {
        int i;
        for (i = 0; i < count; ++i) {
            xyz[i * 3 + 0] = xyz[i * 3 + 0] * scale_bias[0] + scale_bias[1];
            xyz[i * 3 + 1] = xyz[i * 3 + 1] * scale_bias[0] + scale_bias[2];
            xyz[i * 3 + 2] = (size > 2 ? xyz[i * 3 + 2] : 0.0f) * scale_bias[0] + scale_bias[3];
        }
    }
    texcoord = m3gGetVertexArray(vb, M3G_GET_TEXCOORDS0, tex_scale_bias, 4);
    if (texcoord != 0) {
        m3gGetVertexArrayParams(texcoord, &tex_count, &tex_size, &tex_type,
                                &tex_stride);
        if (tex_count > 0 && tex_size >= 2) {
            uv = (float *) malloc((size_t) tex_count * 2U * sizeof(float));
            if (uv != 0) {
                int i;
                memset(uv, 0, (size_t) tex_count * 2U * sizeof(float));
                m3gGetVertexArrayElements(texcoord, 0, tex_count,
                    (M3Gsizei) ((size_t) tex_count * (size_t) tex_size *
                                sizeof(float)),
                    M3G_FLOAT, uv);
                for (i = 0; i < tex_count; ++i) {
                    uv[i * 2 + 0] = uv[i * 2 + 0] * tex_scale_bias[0] +
                                    tex_scale_bias[1];
                    uv[i * 2 + 1] = uv[i * 2 + 1] * tex_scale_bias[0] +
                                    tex_scale_bias[2];
                }
            }
        }
    }
    m3gGetCompositeTransform((M3GTransformable) mesh, &composite);
    funkey_m3g_core_matrix_rows(&composite, world);
    funkey_m3g_matrix_mul(model_view, state->view, world);
    sub_count = m3gGetSubmeshCount(mesh);
    for (sub = 0; sub < sub_count; ++sub) {
        M3GAppearance app = m3gGetAppearance(mesh, sub);
        M3GIndexBuffer ib = m3gGetIndexBuffer(mesh, sub);
        M3GMaterial mat = app != 0 ? m3gGetMaterial(app) : 0;
        FunKeyM3GCoreTexture texture;
        int has_texture;
        unsigned int argb = m3gGetVertexDefaultColor(vb);
        unsigned short color;
        int batches;
        int batch;
        memset(&texture, 0, sizeof(texture));
        if (ib == 0) {
            continue;
        }
        if (mat != 0) {
            argb = m3gGetColor(mat, M3G_DIFFUSE_BIT);
        }
        color = funkey_m3g_argb_to_rgb565(argb | 0xff000000U);
        has_texture = uv != 0 && funkey_m3g_core_load_texture(app, &texture);
        batches = m3gGetBatchCount(ib);
        for (batch = 0; batch < batches; ++batch) {
            int batch_size = m3gGetBatchSize(ib, batch);
            int *indices;
            int i;
            if (batch_size < 3) {
                continue;
            }
            indices = (int *) malloc((size_t) batch_size * sizeof(int));
            if (indices == 0) {
                continue;
            }
            if (!m3gGetBatchIndices(ib, batch, indices)) {
                free(indices);
                continue;
            }
            for (i = 0; i + 2 < batch_size; ++i) {
                int ia = indices[i];
                int ibb = indices[i + 1 + (i & 1)];
                int ic = indices[i + 2 - (i & 1)];
                FunKeyM3GProjectedVertex a, b, c;
                if (ia < 0 || ibb < 0 || ic < 0 ||
                        ia >= count || ibb >= count || ic >= count) {
                    continue;
                }
                funkey_m3g_core_project(surface, model_view, state,
                                        xyz[ia * 3], xyz[ia * 3 + 1],
                                        xyz[ia * 3 + 2], &a);
                funkey_m3g_core_project(surface, model_view, state,
                                        xyz[ibb * 3], xyz[ibb * 3 + 1],
                                        xyz[ibb * 3 + 2], &b);
                funkey_m3g_core_project(surface, model_view, state,
                                        xyz[ic * 3], xyz[ic * 3 + 1],
                                        xyz[ic * 3 + 2], &c);
                if (has_texture && ia < tex_count && ibb < tex_count &&
                        ic < tex_count) {
                    a.u = uv[ia * 2 + 0];
                    a.v = uv[ia * 2 + 1];
                    b.u = uv[ibb * 2 + 0];
                    b.v = uv[ibb * 2 + 1];
                    c.u = uv[ic * 2 + 0];
                    c.v = uv[ic * 2 + 1];
                } else {
                    a.u = b.u = c.u = 0.0f;
                    a.v = b.v = c.v = 0.0f;
                }
                funkey_m3g_core_draw_triangle(surface, &a, &b, &c, color,
                                              has_texture ? &texture : 0);
            }
            free(indices);
        }
        funkey_m3g_core_free_texture(&texture);
    }
    if (uv != 0) {
        free(uv);
    }
    free(xyz);
}

static void
funkey_m3g_core_render_node(FunKeyM3GSurface *surface, M3GNode node,
                            const FunKeyM3GRenderState *state) {
    M3GClass cls;
    int i;
    int count;
    if (node == 0) {
        return;
    }
    cls = m3gGetClass((M3GObject) node);
    if (cls == M3G_CLASS_MESH || cls == M3G_CLASS_MORPHING_MESH ||
            cls == M3G_CLASS_SKINNED_MESH) {
        funkey_m3g_core_render_mesh(surface, (M3GMesh) node, state);
    }
    if (cls != M3G_CLASS_GROUP && cls != M3G_CLASS_WORLD) {
        return;
    }
    count = m3gGetChildCount((M3GGroup) node);
    for (i = 0; i < count; ++i) {
        funkey_m3g_core_render_node(surface, m3gGetChild((M3GGroup) node, i),
                                    state);
    }
}

static int
funkey_m3g_surface_render_core_world(FunKeyM3GSurface *surface, long world) {
    FunKeyM3GObject *w = funkey_m3g_object(world);
    M3GInterface m3g;
    M3GRenderContext ctx;
    if (surface == 0 || surface->pixels == 0 ||
            surface->width <= 0 || surface->height <= 0 ||
            w == 0 || w->core == 0) {
        return 0;
    }
    m3g = w->core_interface != 0 ? w->core_interface :
          funkey_m3g_ensure_core_interface();
    if (m3g == 0) {
        return 0;
    }
    ctx = m3gCreateContext(m3g);
    if (ctx == 0) {
        return 0;
    }
    m3gBindMemoryTarget(ctx, surface->pixels,
                        (M3Guint) surface->width,
                        (M3Guint) surface->height,
                        M3G_RGB565,
                        (M3Guint) surface->stride,
                        0);
    m3gSetViewport(ctx, 0, 0, surface->width, surface->height);
    if (surface->clip_w > 0 && surface->clip_h > 0) {
        m3gSetClipRect(ctx, surface->clip_x, surface->clip_y,
                       surface->clip_w, surface->clip_h);
    }
    m3gRenderWorld(ctx, (M3GWorld) w->core);
    m3gReleaseTarget(ctx);
    m3gDeleteRef((M3GObject) ctx);
    return 1;
}

static M3GRenderContext
funkey_m3g_context_core(long context) {
    FunKeyM3GObject *ctx = funkey_m3g_object(context);
    if (ctx == 0 || ctx->class_id != FUNKEY_M3G_CLASS_RENDER_CONTEXT) {
        return 0;
    }
    if (ctx->core == 0) {
        ctx->core_interface = ctx->core_interface != 0 ? ctx->core_interface :
                              funkey_m3g_ensure_core_interface();
        if (ctx->core_interface != 0) {
            ctx->core = (M3GObject) m3gCreateContext(ctx->core_interface);
        }
    }
    return (M3GRenderContext) ctx->core;
}

static void
funkey_m3g_matrix_from_float(M3GMatrix *dst, const float *src) {
    m3gIdentityMatrix(dst);
    if (src != 0) {
        m3gSetMatrixRows(dst, src);
    }
}

void
funkey_m3g_context_bind_surface(long context, FunKeyM3GSurface *surface) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    if (ctx == 0 || surface == 0 || surface->pixels == 0 ||
            surface->width <= 0 || surface->height <= 0) {
        return;
    }
    m3gBindMemoryTarget(ctx, surface->pixels,
                        (M3Guint) surface->width,
                        (M3Guint) surface->height,
                        M3G_RGB565,
                        (M3Guint) surface->stride,
                        0);
    m3gSetViewport(ctx, 0, 0, surface->width, surface->height);
    if (surface->clip_w > 0 && surface->clip_h > 0) {
        m3gSetClipRect(ctx, surface->clip_x, surface->clip_y,
                       surface->clip_w, surface->clip_h);
    }
}

void
funkey_m3g_context_release_target(long context) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    if (ctx != 0) {
        m3gReleaseTarget(ctx);
    }
}

void
funkey_m3g_context_clear(long context, long background) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GObject bg = funkey_m3g_core_object(background);
    if (ctx != 0) {
        m3gClear(ctx, (M3GBackground) bg);
    }
}

void
funkey_m3g_context_render(long context, long vertices, long indices,
                          long appearance, const float *transform,
                          int scope) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix matrix;
    M3GMatrix *matrix_ptr = 0;
    M3GObject vb = funkey_m3g_core_object(vertices);
    M3GObject ib = funkey_m3g_core_object(indices);
    M3GObject app = funkey_m3g_core_object(appearance);
    if (ctx == 0 || vb == 0 || ib == 0 || app == 0) {
        fprintf(stderr, "[M3G NGL] render immediate skipped ctx=%ld vb=%ld ib=%ld app=%ld\n",
                context, vertices, indices, appearance);
        return;
    }
    if (transform != 0) {
        funkey_m3g_matrix_from_float(&matrix, transform);
        matrix_ptr = &matrix;
    }
    fprintf(stderr, "[M3G NGL] render immediate vb=%ld ib=%ld app=%ld\n",
            vertices, indices, appearance);
    m3gRender(ctx, (M3GVertexBuffer) vb, (M3GIndexBuffer) ib,
              (M3GAppearance) app, matrix_ptr, 1.0f, scope);
}

void
funkey_m3g_context_render_node(long context, long node, const float *transform) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix matrix;
    M3GMatrix *matrix_ptr = 0;
    M3GObject n = funkey_m3g_core_object(node);
    if (ctx == 0 || n == 0) {
        fprintf(stderr, "[M3G NGL] render node skipped ctx=%ld node=%ld\n",
                context, node);
        return;
    }
    if (transform != 0) {
        funkey_m3g_matrix_from_float(&matrix, transform);
        matrix_ptr = &matrix;
    }
    fprintf(stderr, "[M3G NGL] render node=%ld\n", node);
    m3gRenderNode(ctx, (M3GNode) n, matrix_ptr);
}

void
funkey_m3g_context_render_world(long context, long world) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GObject w = funkey_m3g_core_object(world);
    if (ctx == 0 || w == 0) {
        fprintf(stderr, "[M3G NGL] render world skipped ctx=%ld world=%ld\n",
                context, world);
        return;
    }
    if (world != g_ngl_trace_world) {
        g_ngl_trace_world = world;
        g_ngl_trace_frame = 0;
    }
    if (g_ngl_trace_frame == 0) {
        fprintf(stderr, "[M3G NGL] render new world=%ld trace first frame\n",
                world);
        nglTraceFrame(64, 64);
    } else if (g_ngl_trace_frame < 3) {
        fprintf(stderr, "[M3G NGL] render world=%ld frame=%d\n",
                world, g_ngl_trace_frame + 1);
    }
    ++g_ngl_trace_frame;
    m3gRenderWorld(ctx, (M3GWorld) w);
}

int
funkey_m3g_context_add_light(long context, long light, const float *transform) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix matrix;
    M3GMatrix *matrix_ptr = 0;
    M3GObject l = funkey_m3g_core_object(light);
    if (ctx == 0 || l == 0) {
        return -1;
    }
    if (transform != 0) {
        funkey_m3g_matrix_from_float(&matrix, transform);
        matrix_ptr = &matrix;
    }
    return (int) m3gAddLight(ctx, (M3GLight) l, matrix_ptr);
}

void
funkey_m3g_context_set_camera(long context, long camera, const float *transform) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix matrix;
    M3GMatrix *matrix_ptr = 0;
    M3GObject c = funkey_m3g_core_object(camera);
    if (ctx == 0 || c == 0) {
        return;
    }
    if (transform != 0) {
        funkey_m3g_matrix_from_float(&matrix, transform);
        matrix_ptr = &matrix;
    }
    m3gSetCamera(ctx, (M3GCamera) c, matrix_ptr);
}

void
funkey_m3g_context_set_viewport(long context, int x, int y,
                                int width, int height) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    if (ctx != 0) {
        m3gSetViewport(ctx, x, y, width, height);
    }
}

void
funkey_m3g_context_set_light(long context, int index, long light,
                             const float *transform) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix matrix;
    M3GMatrix *matrix_ptr = 0;
    M3GObject l = funkey_m3g_core_object(light);
    if (ctx == 0 || l == 0) {
        return;
    }
    if (transform != 0) {
        funkey_m3g_matrix_from_float(&matrix, transform);
        matrix_ptr = &matrix;
    }
    m3gSetLight(ctx, index, (M3GLight) l, matrix_ptr);
}

void
funkey_m3g_context_set_depth_range(long context, float near_value,
                                   float far_value) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    if (ctx != 0) {
        m3gSetDepthRange(ctx, near_value, far_value);
    }
}

void
funkey_m3g_context_get_view_transform(long context, float *matrix) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix core_matrix;
    if (matrix == 0) {
        return;
    }
    funkey_m3g_matrix_identity(matrix);
    if (ctx != 0) {
        m3gGetViewTransform(ctx, &core_matrix);
        m3gGetMatrixRows(&core_matrix, matrix);
    }
}

long
funkey_m3g_context_get_camera(long context) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    if (ctx == 0) {
        return 0;
    }
    return funkey_m3g_wrap_core_object((M3GObject) m3gGetCamera(ctx));
}

long
funkey_m3g_context_get_light_transform(long context, int index,
                                       float *matrix) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    M3GMatrix core_matrix;
    M3GLight light;
    if (matrix != 0) {
        funkey_m3g_matrix_identity(matrix);
    }
    if (ctx == 0) {
        return 0;
    }
    light = m3gGetLightTransform(ctx, index, &core_matrix);
    if (matrix != 0) {
        m3gGetMatrixRows(&core_matrix, matrix);
    }
    return funkey_m3g_wrap_core_object((M3GObject) light);
}

int
funkey_m3g_context_get_light_count(long context) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    return ctx != 0 ? (int) m3gGetLightCount(ctx) : 0;
}

void
funkey_m3g_context_get_depth_range(long context, float *near_value,
                                   float *far_value) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    float n = 0.0f;
    float f = 1.0f;
    if (ctx != 0) {
        m3gGetDepthRange(ctx, &n, &f);
    }
    if (near_value != 0) {
        *near_value = n;
    }
    if (far_value != 0) {
        *far_value = f;
    }
}

void
funkey_m3g_context_get_viewport(long context, int *x, int *y,
                                int *width, int *height) {
    M3GRenderContext ctx = funkey_m3g_context_core(context);
    int vx = 0;
    int vy = 0;
    int vw = 0;
    int vh = 0;
    if (ctx != 0) {
        m3gGetViewport(ctx, &vx, &vy, &vw, &vh);
    }
    if (x != 0) *x = vx;
    if (y != 0) *y = vy;
    if (width != 0) *width = vw;
    if (height != 0) *height = vh;
}

static void
funkey_m3g_surface_render_background(FunKeyM3GSurface *surface,
                                     FunKeyM3GObject *background) {
    FunKeyM3GObject *image;
    int x;
    int y;
    int source_x;
    int source_y;
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    if (surface == 0 || background == 0 || background->background_image == 0) {
        return;
    }
    image = funkey_m3g_object(background->background_image);
    if (image == 0 || image->image_pixels == 0) {
        return;
    }
    start_x = surface->clip_x < 0 ? 0 : surface->clip_x;
    start_y = surface->clip_y < 0 ? 0 : surface->clip_y;
    end_x = surface->clip_x + surface->clip_w;
    end_y = surface->clip_y + surface->clip_h;
    if (end_x > surface->width) end_x = surface->width;
    if (end_y > surface->height) end_y = surface->height;
    for (y = start_y; y < end_y; ++y) {
        for (x = start_x; x < end_x; ++x) {
            source_x = background->crop_x + x - surface->clip_x;
            source_y = background->crop_y + y - surface->clip_y;
            source_x = funkey_m3g_wrap_texel(source_x, image->image_width,
                                             background->image_mode_x == 33 ?
                                             241 : 240);
            source_y = funkey_m3g_wrap_texel(source_y, image->image_height,
                                             background->image_mode_y == 33 ?
                                             241 : 240);
            funkey_m3g_put_argb(surface, x, y,
                               funkey_m3g_image_argb(image, source_x, source_y));
        }
    }
}

void
funkey_m3g_surface_render_mesh(FunKeyM3GSurface *surface, long mesh) {
    FunKeyM3GRenderState state;
    float world[16];
    funkey_m3g_matrix_identity(state.view);
    state.fovy = 45.0f;
    state.aspect = surface != 0 && surface->height != 0 ?
                   (float) surface->width / (float) surface->height : 1.0f;
    state.near_plane = 0.1f;
    state.has_camera = 0;
    funkey_m3g_matrix_identity(world);
    funkey_m3g_surface_render_mesh_state(surface, mesh, world, &state);
}

static void
funkey_m3g_surface_render_node_state(FunKeyM3GSurface *surface, long node,
                                     const float *parent_world,
                                     const FunKeyM3GRenderState *state) {
    FunKeyM3GObject *obj = funkey_m3g_object(node);
    float local[16];
    float world[16];
    int i;
    if (obj == 0 || obj->rendering_enabled == 0) {
        return;
    }
    funkey_m3g_local_matrix(obj, local);
    funkey_m3g_matrix_mul(world, parent_world, local);
    if (obj->class_id == FUNKEY_M3G_CLASS_MESH) {
        funkey_m3g_surface_render_mesh_state(surface, node, world, state);
        return;
    }
    for (i = 0; i < obj->child_count; ++i) {
        funkey_m3g_surface_render_node_state(surface, obj->children[i],
                                             world, state);
    }
}

void
funkey_m3g_surface_render_node(FunKeyM3GSurface *surface, long node) {
    FunKeyM3GRenderState state;
    float world[16];
    funkey_m3g_matrix_identity(state.view);
    state.fovy = 45.0f;
    state.aspect = surface != 0 && surface->height != 0 ?
                   (float) surface->width / (float) surface->height : 1.0f;
    state.near_plane = 0.1f;
    state.has_camera = 0;
    funkey_m3g_matrix_identity(world);
    funkey_m3g_surface_render_node_state(surface, node, world, &state);
}

void
funkey_m3g_surface_render_world(FunKeyM3GSurface *surface, long world) {
    FunKeyM3GRenderState state;
    FunKeyM3GObject *w = funkey_m3g_object(world);
    float identity[16];
    if (world != g_mesh_trace_world) {
        g_mesh_trace_world = world;
        g_mesh_trace_count = 0;
        fprintf(stderr, "[M3G raster] world=%ld\n", world);
    }
    if (w != 0 && w->core != 0) {
        funkey_m3g_surface_render_core_world(surface, world);
        return;
    }
    funkey_m3g_matrix_identity(identity);
    funkey_m3g_matrix_identity(state.view);
    state.fovy = 45.0f;
    state.aspect = surface != 0 && surface->height != 0 ?
                   (float) surface->width / (float) surface->height : 1.0f;
    state.near_plane = 0.1f;
    state.has_camera = 0;
    if (w != 0 && w->background != 0) {
        funkey_m3g_surface_render_background(surface,
                                             funkey_m3g_object(w->background));
    }
    if (w != 0 && w->active_camera != 0) {
        FunKeyM3GObject *camera = funkey_m3g_object(w->active_camera);
        float camera_world[16];
        funkey_m3g_world_matrix(w->active_camera, camera_world);
        if (funkey_m3g_matrix_invert(state.view, camera_world)) {
            state.has_camera = 1;
        }
        if (camera != 0) {
            if (camera->projection_mode == 50) {
                state.fovy = camera->projection_params[0];
                state.aspect = camera->projection_params[1];
                state.near_plane = camera->projection_params[2] > 0.001f ?
                                   camera->projection_params[2] : 0.1f;
            } else if (camera->projection_mode == 49) {
                state.has_camera = 0;
            }
        }
    }
    funkey_m3g_surface_render_node_state(surface, world, identity, &state);
}
