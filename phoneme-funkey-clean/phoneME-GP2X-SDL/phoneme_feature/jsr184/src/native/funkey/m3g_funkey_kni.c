/*
 * Initial phoneME KNI bridge for JSR-184.
 *
 * This file is intentionally small: it proves the native binding shape used by
 * phoneME before the full Nokia M3G core is wired behind it. Native method
 * groups should be added here in narrow, testable batches.
 */

#include <kni.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <gxapi_graphics.h>
#include <gxj_putpixel.h>

#include "m3g_funkey_soft.h"

static FunKeyM3GSurface g_surface;

#define M3G_LONG_PARAM(index) ((long) KNI_GetParameterAsLong(index))

static void
m3g_write_float_array_param(int param, const float *values, int count) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        int len = KNI_GetArrayLength(array);
        if (count > len) {
            count = len;
        }
        if (count > 0) {
            KNI_SetRawArrayRegion(array, 0, count * (int) sizeof(float),
                                  (const jbyte *) values);
        }
    }
    KNI_EndHandles();
}

static int
m3g_read_float_array_param(int param, float *values, int count) {
    int len = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > count) {
            len = count;
        }
        if (len > 0) {
            KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(float),
                                  (jbyte *) values);
        }
    }
    KNI_EndHandles();
    return len;
}

static void
m3g_read_matrix_param(int param, float *matrix) {
    int i;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array) && KNI_GetArrayLength(array) >= 64) {
        KNI_GetRawArrayRegion(array, 0, 64, (jbyte *) matrix);
    } else {
        for (i = 0; i < 16; ++i) {
            matrix[i] = 0.0f;
        }
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    }
    KNI_EndHandles();
}

static void
m3g_write_matrix_param(int param, const float *matrix) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array) && KNI_GetArrayLength(array) >= 64) {
        KNI_SetRawArrayRegion(array, 0, 64, (const jbyte *) matrix);
    }
    KNI_EndHandles();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Interface__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_interface());
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Interface__1getClassID(void) {
    KNI_ReturnInt(funkey_m3g_get_class_id(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Platform_finalizeInterface(void) {
    funkey_m3g_finalize_interface(M3G_LONG_PARAM(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Platform__1finalizeObject(void) {
    funkey_m3g_release(M3G_LONG_PARAM(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Object3D__1addRef(void) {
    funkey_m3g_add_ref(M3G_LONG_PARAM(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Object3D__1addAnimationTrack(void) {
    KNI_ReturnInt(funkey_m3g_object_add_animation_track(M3G_LONG_PARAM(1),
                                                         M3G_LONG_PARAM(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Object3D__1removeAnimationTrack(void) {
    funkey_m3g_object_remove_animation_track(M3G_LONG_PARAM(1),
                                              M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Object3D__1getAnimationTrackCount(void) {
    KNI_ReturnInt(funkey_m3g_object_get_animation_track_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Object3D__1animate(void) {
    KNI_ReturnInt(funkey_m3g_object_animate(M3G_LONG_PARAM(1),
                                            KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Object3D__1setUserID(void) {
    funkey_m3g_set_user_id(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Object3D__1getUserID(void) {
    KNI_ReturnInt(funkey_m3g_get_user_id(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Object3D__1getAnimationTrack(void) {
    KNI_ReturnLong(funkey_m3g_object_get_animation_track(M3G_LONG_PARAM(1),
                                                          KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Object3D__1duplicate(void) {
    long original = M3G_LONG_PARAM(1);
    long *pairs = 0;
    jlong *out = 0;
    int max_pairs = 0;
    int pair_count = 0;
    long clone;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(3, array);
    if (!KNI_IsNullHandle(array)) {
        int len = KNI_GetArrayLength(array);
        max_pairs = len / 2;
        if (max_pairs > 0) {
            pairs = (long *) malloc((size_t) max_pairs * 2 * sizeof(long));
            out = (jlong *) malloc((size_t) max_pairs * 2 * sizeof(jlong));
        }
    }
    clone = funkey_m3g_duplicate(original, pairs, max_pairs, &pair_count);
    if (!KNI_IsNullHandle(array) && pair_count > 0 &&
            pairs != 0 && out != 0) {
        int i;
        for (i = 0; i < pair_count * 2; ++i) {
            out[i] = (jlong) pairs[i];
        }
        KNI_SetRawArrayRegion(array, 0, pair_count * 2 * (int) sizeof(jlong),
                              (const jbyte *) out);
    }
    KNI_EndHandles();
    if (pairs != 0) {
        free(pairs);
    }
    if (out != 0) {
        free(out);
    }
    KNI_ReturnLong(clone);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Object3D__1getReferences(void) {
    long refs[256];
    int len = 0;
    int count;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(3, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > (int) (sizeof(refs) / sizeof(refs[0]))) {
            len = (int) (sizeof(refs) / sizeof(refs[0]));
        }
    }
    count = funkey_m3g_object_get_references(M3G_LONG_PARAM(1),
                                             len > 0 ? refs : 0, len);
    if (!KNI_IsNullHandle(array) && len > 0) {
        int i;
        int out_count = count < len ? count : len;
        jlong out[256];
        for (i = 0; i < out_count; ++i) {
            out[i] = (jlong) refs[i];
        }
        KNI_SetRawArrayRegion(array, 0, out_count * (int) sizeof(jlong),
                              (const jbyte *) out);
    }
    KNI_EndHandles();
    KNI_ReturnInt(count);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Object3D__1find(void) {
    long handle = M3G_LONG_PARAM(1);
    int user_id = KNI_GetParameterAsInt(3);
    KNI_ReturnLong(funkey_m3g_find(handle, user_id));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Node__1getTransformTo(void) {
    float matrix[16];
    int ok = funkey_m3g_node_get_transform_to(M3G_LONG_PARAM(1),
                                              M3G_LONG_PARAM(3), matrix);
    if (ok) {
        m3g_write_matrix_param(5, matrix);
    }
    KNI_ReturnBoolean(ok ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Node__1align(void) {
    funkey_m3g_node_align(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Node__1setAlignment(void) {
    funkey_m3g_node_set_alignment(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3),
                                  KNI_GetParameterAsInt(5),
                                  M3G_LONG_PARAM(6),
                                  KNI_GetParameterAsInt(8));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Node__1setAlphaFactor(void) {
    funkey_m3g_node_set_alpha(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Node__1getAlphaFactor(void) {
    KNI_ReturnFloat(funkey_m3g_node_get_alpha(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Node__1enable(void) {
    funkey_m3g_node_enable(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                           KNI_GetParameterAsBoolean(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Node__1isEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_node_is_enabled(M3G_LONG_PARAM(1),
                                                 KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Node__1setScope(void) {
    funkey_m3g_node_set_scope(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Node__1getScope(void) {
    KNI_ReturnInt(funkey_m3g_node_get_scope(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Node__1getParent(void) {
    KNI_ReturnLong(funkey_m3g_node_get_parent(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Node__1getZRef(void) {
    KNI_ReturnLong(funkey_m3g_node_get_z_ref(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Node__1getYRef(void) {
    KNI_ReturnLong(funkey_m3g_node_get_y_ref(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Node__1getSubtreeSize(void) {
    KNI_ReturnInt(funkey_m3g_node_get_subtree_size(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Node__1getAlignmentTarget(void) {
    KNI_ReturnInt(funkey_m3g_node_get_alignment_target(M3G_LONG_PARAM(1),
                                                       KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1setOrientation(void) {
    funkey_m3g_transform_set_orientation(M3G_LONG_PARAM(1),
                                         KNI_GetParameterAsFloat(3),
                                         KNI_GetParameterAsFloat(4),
                                         KNI_GetParameterAsFloat(5),
                                         KNI_GetParameterAsFloat(6),
                                         KNI_GetParameterAsBoolean(7));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1preRotate(void) {
    funkey_m3g_transform_pre_rotate(M3G_LONG_PARAM(1),
                                    KNI_GetParameterAsFloat(3),
                                    KNI_GetParameterAsFloat(4),
                                    KNI_GetParameterAsFloat(5),
                                    KNI_GetParameterAsFloat(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1getOrientation(void) {
    float values[4];
    funkey_m3g_transform_get_orientation(M3G_LONG_PARAM(1), values, 4);
    m3g_write_float_array_param(3, values, 4);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1setScale(void) {
    funkey_m3g_transform_set_scale(M3G_LONG_PARAM(1),
                                   KNI_GetParameterAsFloat(3),
                                   KNI_GetParameterAsFloat(4),
                                   KNI_GetParameterAsFloat(5),
                                   KNI_GetParameterAsBoolean(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1getScale(void) {
    float values[3];
    funkey_m3g_transform_get_scale(M3G_LONG_PARAM(1), values, 3);
    m3g_write_float_array_param(3, values, 3);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1setTranslation(void) {
    funkey_m3g_transform_set_translation(M3G_LONG_PARAM(1),
                                         KNI_GetParameterAsFloat(3),
                                         KNI_GetParameterAsFloat(4),
                                         KNI_GetParameterAsFloat(5),
                                         KNI_GetParameterAsBoolean(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1getTranslation(void) {
    float values[3];
    funkey_m3g_transform_get_translation(M3G_LONG_PARAM(1), values, 3);
    m3g_write_float_array_param(3, values, 3);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1setTransform(void) {
    float matrix[16];
    m3g_read_matrix_param(3, matrix);
    funkey_m3g_transform_set_matrix(M3G_LONG_PARAM(1), matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1getTransform(void) {
    float matrix[16];
    funkey_m3g_transform_get_matrix(M3G_LONG_PARAM(1), matrix);
    m3g_write_matrix_param(3, matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transformable__1getComposite(void) {
    float matrix[16];
    funkey_m3g_transform_get_composite(M3G_LONG_PARAM(1), matrix);
    m3g_write_matrix_param(3, matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Group__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_GROUP));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Group__1addChild(void) {
    funkey_m3g_group_add_child(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Group__1removeChild(void) {
    funkey_m3g_group_remove_child(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Group__1getChildCount(void) {
    KNI_ReturnInt(funkey_m3g_group_get_child_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Group__1getChild(void) {
    KNI_ReturnLong(funkey_m3g_group_get_child(M3G_LONG_PARAM(1),
                                              KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Group__1pick3D(void) {
    float ray[6];
    float result[15];
    long picked = 0;
    memset(result, 0, sizeof(result));
    if (m3g_read_float_array_param(4, ray, 6) == 6) {
        picked = funkey_m3g_group_pick3d(M3G_LONG_PARAM(1),
                                         KNI_GetParameterAsInt(3),
                                         ray, result);
        if (picked != 0) {
            m3g_write_float_array_param(5, result, 15);
        }
    }
    KNI_ReturnLong(picked);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Group__1pick2D(void) {
    float result[15];
    long picked;
    memset(result, 0, sizeof(result));
    picked = funkey_m3g_group_pick2d(M3G_LONG_PARAM(1),
                                     KNI_GetParameterAsInt(3),
                                     KNI_GetParameterAsFloat(4),
                                     KNI_GetParameterAsFloat(5),
                                     M3G_LONG_PARAM(6), result);
    if (picked != 0) {
        m3g_write_float_array_param(8, result, 15);
    }
    KNI_ReturnLong(picked);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_World__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_WORLD));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_World__1setActiveCamera(void) {
    funkey_m3g_world_set_active_camera(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_World__1setBackground(void) {
    funkey_m3g_world_set_background(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_World__1getActiveCamera(void) {
    KNI_ReturnLong(funkey_m3g_world_get_active_camera(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_World__1getBackground(void) {
    KNI_ReturnLong(funkey_m3g_world_get_background(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Camera__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_CAMERA));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Camera__1setParallel(void) {
    funkey_m3g_camera_set_projection(M3G_LONG_PARAM(1), 49,
                                     KNI_GetParameterAsFloat(3),
                                     KNI_GetParameterAsFloat(4),
                                     KNI_GetParameterAsFloat(5),
                                     KNI_GetParameterAsFloat(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Camera__1setPerspective(void) {
    funkey_m3g_camera_set_projection(M3G_LONG_PARAM(1), 50,
                                     KNI_GetParameterAsFloat(3),
                                     KNI_GetParameterAsFloat(4),
                                     KNI_GetParameterAsFloat(5),
                                     KNI_GetParameterAsFloat(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Camera__1setGeneric(void) {
    float matrix[16];
    m3g_read_matrix_param(3, matrix);
    funkey_m3g_camera_set_generic(M3G_LONG_PARAM(1), matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Camera__1getProjectionAsTransform(void) {
    float matrix[16];
    int mode = funkey_m3g_camera_get_projection_matrix(M3G_LONG_PARAM(1),
                                                        matrix);
    m3g_write_matrix_param(3, matrix);
    KNI_ReturnInt(mode);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Camera__1getProjectionAsParams(void) {
    float params[4];
    int mode = funkey_m3g_camera_get_projection(M3G_LONG_PARAM(1), params);

    if (mode != 48) {
        m3g_write_float_array_param(3, params, 4);
    }
    KNI_ReturnInt(mode);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Background__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_BACKGROUND));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Background__1setColor(void) {
    funkey_m3g_background_set_color(M3G_LONG_PARAM(1),
                                    (unsigned int) KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Background__1getColor(void) {
    KNI_ReturnInt((jint) funkey_m3g_background_get_color(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Background__1setImage(void) {
    funkey_m3g_background_set_image(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Background__1getImage(void) {
    KNI_ReturnLong(funkey_m3g_background_get_image(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Background__1setImageMode(void) {
    funkey_m3g_background_set_image_mode(M3G_LONG_PARAM(1),
                                         KNI_GetParameterAsInt(3),
                                         KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Background__1getImageMode(void) {
    KNI_ReturnInt(funkey_m3g_background_get_image_mode(M3G_LONG_PARAM(1),
                                                       KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Background__1enable(void) {
    funkey_m3g_background_enable(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                                 KNI_GetParameterAsBoolean(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Background__1isEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_background_is_enabled(M3G_LONG_PARAM(1),
                                                       KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Background__1setCrop(void) {
    funkey_m3g_background_set_crop(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                                   KNI_GetParameterAsInt(4),
                                   KNI_GetParameterAsInt(5),
                                   KNI_GetParameterAsInt(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Background__1getCrop(void) {
    KNI_ReturnInt(funkey_m3g_background_get_crop(M3G_LONG_PARAM(1),
                                                 KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Graphics3D__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_RENDER_CONTEXT));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1addRef(void) {
    funkey_m3g_add_ref(M3G_LONG_PARAM(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Graphics3D__1bindGraphics(void) {
    int clip_h = KNI_GetParameterAsInt(9);
    int clip_w = KNI_GetParameterAsInt(8);
    int clip_y = KNI_GetParameterAsInt(7);
    int clip_x = KNI_GetParameterAsInt(6);
    int height = KNI_GetParameterAsInt(5);
    int width = KNI_GetParameterAsInt(4);
    gxj_screen_buffer sbuf;
    gxj_screen_buffer *target = 0;

    funkey_m3g_surface_init(&g_surface, width, height, clip_x, clip_y,
                            clip_w, clip_h);

    KNI_StartHandles(1);
    KNI_DeclareHandle(graphics);
    KNI_GetParameterAsObject(3, graphics);
    if (!KNI_IsNullHandle(graphics)) {
        target = GXJ_GET_GRAPHICS_SCREEN_BUFFER(graphics, &sbuf);
        if (target != 0 && target->pixelData != 0) {
            funkey_m3g_surface_bind_pixels(&g_surface, target->pixelData,
                                           target->width);
            if (width <= 0 || height <= 0) {
                g_surface.width = target->width;
                g_surface.height = target->height;
            }
        }
    }
    KNI_EndHandles();

    if (g_surface.pixels == 0 && gxj_system_screen_buffer.pixelData != 0) {
        funkey_m3g_surface_bind_pixels(&g_surface,
                                       gxj_system_screen_buffer.pixelData,
                                       gxj_system_screen_buffer.width);
        if (g_surface.width <= 0 || g_surface.height <= 0) {
            g_surface.width = gxj_system_screen_buffer.width;
            g_surface.height = gxj_system_screen_buffer.height;
        }
    }

    funkey_m3g_context_bind_surface(M3G_LONG_PARAM(1), &g_surface);
    KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1bindImage(void) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1releaseGraphics(void) {
    funkey_m3g_context_release_target(M3G_LONG_PARAM(1));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1releaseImage(void) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1resetLights(void) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1clear(void) {
    long background = M3G_LONG_PARAM(3);
    unsigned int argb = 0xff000000U;
    if (background != 0) {
        argb = funkey_m3g_background_get_color(background);
    }
    funkey_m3g_surface_clear(&g_surface, argb);
    funkey_m3g_context_clear(M3G_LONG_PARAM(1), background);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1render(void) {
    float matrix[16];
    m3g_read_matrix_param(9, matrix);
    funkey_m3g_context_render(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3),
                              M3G_LONG_PARAM(5), M3G_LONG_PARAM(7),
                              matrix, KNI_GetParameterAsInt(10));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1renderNode(void) {
    float matrix[16];
    m3g_read_matrix_param(5, matrix);
    funkey_m3g_context_render_node(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3),
                                   matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1renderWorld(void) {
    long world = M3G_LONG_PARAM(3);
    funkey_m3g_context_render_world(M3G_LONG_PARAM(1), world);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1addLight(void) {
    float matrix[16];
    m3g_read_matrix_param(5, matrix);
    KNI_ReturnInt(funkey_m3g_context_add_light(M3G_LONG_PARAM(1),
                                               M3G_LONG_PARAM(3), matrix));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1setCamera(void) {
    float matrix[16];
    m3g_read_matrix_param(5, matrix);
    funkey_m3g_context_set_camera(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3),
                                  matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1setViewport(void) {
    funkey_m3g_context_set_viewport(M3G_LONG_PARAM(1),
                                    KNI_GetParameterAsInt(3),
                                    KNI_GetParameterAsInt(4),
                                    KNI_GetParameterAsInt(5),
                                    KNI_GetParameterAsInt(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1setLight(void) {
    float matrix[16];
    m3g_read_matrix_param(6, matrix);
    funkey_m3g_context_set_light(M3G_LONG_PARAM(1),
                                 KNI_GetParameterAsInt(3),
                                 M3G_LONG_PARAM(4), matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1setDepthRange(void) {
    funkey_m3g_context_set_depth_range(M3G_LONG_PARAM(1),
                                       KNI_GetParameterAsFloat(3),
                                       KNI_GetParameterAsFloat(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Graphics3D__1getViewTransform(void) {
    float matrix[16];
    funkey_m3g_context_get_view_transform(M3G_LONG_PARAM(1), matrix);
    m3g_write_matrix_param(3, matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Graphics3D__1getCamera(void) {
    KNI_ReturnLong(funkey_m3g_context_get_camera(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Graphics3D__1getLightTransform(void) {
    float matrix[16];
    long light = funkey_m3g_context_get_light_transform(M3G_LONG_PARAM(1),
                                                        KNI_GetParameterAsInt(3),
                                                        matrix);
    m3g_write_matrix_param(4, matrix);
    KNI_ReturnLong(light);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1getLightCount(void) {
    KNI_ReturnInt(funkey_m3g_context_get_light_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Graphics3D__1getDepthRangeNear(void) {
    float near_value;
    float far_value;
    funkey_m3g_context_get_depth_range(M3G_LONG_PARAM(1),
                                       &near_value, &far_value);
    KNI_ReturnFloat(near_value);
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Graphics3D__1getDepthRangeFar(void) {
    float near_value;
    float far_value;
    funkey_m3g_context_get_depth_range(M3G_LONG_PARAM(1),
                                       &near_value, &far_value);
    KNI_ReturnFloat(far_value);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1getViewportX(void) {
    int x, y, w, h;
    funkey_m3g_context_get_viewport(M3G_LONG_PARAM(1), &x, &y, &w, &h);
    KNI_ReturnInt(w > 0 ? x : g_surface.clip_x);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1getViewportY(void) {
    int x, y, w, h;
    funkey_m3g_context_get_viewport(M3G_LONG_PARAM(1), &x, &y, &w, &h);
    KNI_ReturnInt(h > 0 ? y : g_surface.clip_y);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1getViewportWidth(void) {
    int x, y, w, h;
    funkey_m3g_context_get_viewport(M3G_LONG_PARAM(1), &x, &y, &w, &h);
    (void) x;
    (void) y;
    KNI_ReturnInt(w > 0 ? w : g_surface.width);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D__1getViewportHeight(void) {
    int x, y, w, h;
    funkey_m3g_context_get_viewport(M3G_LONG_PARAM(1), &x, &y, &w, &h);
    (void) x;
    (void) y;
    KNI_ReturnInt(h > 0 ? h : g_surface.height);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Graphics3D_getStatistics(void) {
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Graphics3D__1isAASupported(void) {
    KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Graphics3D__1isProperRenderer(void) {
    KNI_ReturnBoolean(KNI_TRUE);
}
