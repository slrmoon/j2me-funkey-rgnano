/*
 * Generated KNI coverage for the still-unported JSR-184 native methods.
 *
 * Constructors allocate real FunKey M3G handles so Java objects have native
 * peers. Method bodies here are deliberately conservative until their backing
 * state is promoted into m3g_funkey_soft.c and the software rasterizer.
 */

#include <kni.h>

#include "m3g_funkey_soft.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define M3G_LONG_PARAM(index) ((long) KNI_GetParameterAsLong(index))

static int m3g_read_byte_array(int param, unsigned char **out) {
    int len = 0;
    *out = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > 0) {
            *out = (unsigned char *) malloc((size_t) len);
            if (*out != 0) {
                KNI_GetRawArrayRegion(array, 0, len, (jbyte *) *out);
            } else {
                len = 0;
            }
        }
    }
    KNI_EndHandles();
    return len;
}

static int m3g_read_int_array(int param, int **out) {
    int len = 0;
    *out = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > 0) {
            *out = (int *) malloc((size_t) len * sizeof(int));
            if (*out != 0) {
                KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(int),
                                      (jbyte *) *out);
            } else {
                len = 0;
            }
        }
    }
    KNI_EndHandles();
    return len;
}

static int m3g_read_long_array(int param, long **out) {
    int i;
    int len = 0;
    jlong *tmp = 0;
    *out = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > 0) {
            tmp = (jlong *) malloc((size_t) len * sizeof(jlong));
            *out = (long *) malloc((size_t) len * sizeof(long));
            if (tmp != 0 && *out != 0) {
                KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(jlong),
                                      (jbyte *) tmp);
                for (i = 0; i < len; ++i) {
                    (*out)[i] = (long) tmp[i];
                }
            } else {
                len = 0;
            }
        }
    }
    KNI_EndHandles();
    if (tmp != 0) {
        free(tmp);
    }
    return len;
}

static int m3g_read_float_array(int param, float **out) {
    int len = 0;
    *out = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        if (len > 0) {
            *out = (float *) malloc((size_t) len * sizeof(float));
            if (*out != 0) {
                KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(float),
                                      (jbyte *) *out);
            } else {
                len = 0;
            }
        }
    }
    KNI_EndHandles();
    return len;
}

static void m3g_set_int_array_param(int param, const int *values, int count) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array)) {
        int len = KNI_GetArrayLength(array);
        if (count > len) {
            count = len;
        }
        if (count > 0) {
            KNI_SetRawArrayRegion(array, 0, count * (int) sizeof(int),
                                  (const jbyte *) values);
        }
    }
    KNI_EndHandles();
}


static void m3g_matrix_identity(float *m) {
    int i;
    for (i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static int m3g_read_matrix_param(int param, float *m) {
    int ok = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array) && KNI_GetArrayLength(array) >= 64) {
        KNI_GetRawArrayRegion(array, 0, 64, (jbyte *) m);
        ok = 1;
    }
    KNI_EndHandles();
    if (!ok) m3g_matrix_identity(m);
    return ok;
}

static void m3g_write_matrix_param(int param, const float *m) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(param, array);
    if (!KNI_IsNullHandle(array) && KNI_GetArrayLength(array) >= 64) {
        KNI_SetRawArrayRegion(array, 0, 64, (const jbyte *) m);
    }
    KNI_EndHandles();
}

static void m3g_matrix_mul(float *dst, const float *a, const float *b) {
    int r, c, k;
    float out[16];
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            float v = 0.0f;
            for (k = 0; k < 4; ++k) v += a[r * 4 + k] * b[k * 4 + c];
            out[r * 4 + c] = v;
        }
    }
    memcpy(dst, out, sizeof(out));
}

static int m3g_matrix_invert(float *dst, const float *m) {
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
    det = m[0] * inv[0] + m[1] * inv[4] +
          m[2] * inv[8] + m[3] * inv[12];
    if (det > -0.000001f && det < 0.000001f) {
        return 0;
    }
    det = 1.0f / det;
    for (i = 0; i < 16; ++i) {
        dst[i] = inv[i] * det;
    }
    return 1;
}

static void m3g_matrix_rotate_axis(float *dst, float angle,
                                   float ax, float ay, float az) {
    float len = sqrtf(ax * ax + ay * ay + az * az);
    float rad;
    float c;
    float s;
    float t;
    m3g_matrix_identity(dst);
    if (len <= 0.000001f) {
        return;
    }
    ax /= len;
    ay /= len;
    az /= len;
    rad = angle * 0.017453292519943295f;
    c = cosf(rad);
    s = sinf(rad);
    t = 1.0f - c;
    dst[0] = t * ax * ax + c;
    dst[1] = t * ax * ay - s * az;
    dst[2] = t * ax * az + s * ay;
    dst[4] = t * ax * ay + s * az;
    dst[5] = t * ay * ay + c;
    dst[6] = t * ay * az - s * ax;
    dst[8] = t * ax * az - s * ay;
    dst[9] = t * ay * az + s * ax;
    dst[10] = t * az * az + c;
}

static void m3g_matrix_rotate_quat(float *dst, float qx, float qy,
                                   float qz, float qw) {
    float len = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
    float xx, yy, zz, xy, xz, yz, wx, wy, wz;
    m3g_matrix_identity(dst);
    if (len <= 0.000001f) {
        return;
    }
    qx /= len;
    qy /= len;
    qz /= len;
    qw /= len;
    xx = qx * qx;
    yy = qy * qy;
    zz = qz * qz;
    xy = qx * qy;
    xz = qx * qz;
    yz = qy * qz;
    wx = qw * qx;
    wy = qw * qy;
    wz = qw * qz;
    dst[0] = 1.0f - 2.0f * (yy + zz);
    dst[1] = 2.0f * (xy - wz);
    dst[2] = 2.0f * (xz + wy);
    dst[4] = 2.0f * (xy + wz);
    dst[5] = 1.0f - 2.0f * (xx + zz);
    dst[6] = 2.0f * (yz - wx);
    dst[8] = 2.0f * (xz - wy);
    dst[9] = 2.0f * (yz + wx);
    dst[10] = 1.0f - 2.0f * (xx + yy);
}

static void m3g_set_float_array_param(int param, const float *values, int count) {
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


/* AnimationController */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_AnimationController__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_ANIMATION_CONTROLLER));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_AnimationController__1getActiveIntervalEnd(void) {
    KNI_ReturnInt(funkey_m3g_anim_get_active_interval(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_AnimationController__1getActiveIntervalStart(void) {
    KNI_ReturnInt(funkey_m3g_anim_get_active_interval(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_AnimationController__1getPosition(void) {
    KNI_ReturnFloat(funkey_m3g_anim_get_position(M3G_LONG_PARAM(1),
                                                 KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_AnimationController__1getRefWorldTime(void) {
    KNI_ReturnInt(funkey_m3g_anim_get_ref_world_time(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_AnimationController__1getSpeed(void) {
    KNI_ReturnFloat(funkey_m3g_anim_get_speed(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_AnimationController__1getWeight(void) {
    KNI_ReturnFloat(funkey_m3g_anim_get_weight(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_AnimationController__1setActiveInterval(void) {
    funkey_m3g_anim_set_active_interval(M3G_LONG_PARAM(1),
                                        KNI_GetParameterAsInt(3),
                                        KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_AnimationController__1setPosition(void) {
    funkey_m3g_anim_set_position(M3G_LONG_PARAM(1),
                                 KNI_GetParameterAsFloat(3),
                                 KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_AnimationController__1setSpeed(void) {
    funkey_m3g_anim_set_speed(M3G_LONG_PARAM(1),
                              KNI_GetParameterAsFloat(3),
                              KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_AnimationController__1setWeight(void) {
    funkey_m3g_anim_set_weight(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}


/* AnimationTrack */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_AnimationTrack__1ctor(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_ANIMATION_TRACK);
    funkey_m3g_animation_track_init(handle, M3G_LONG_PARAM(3),
                                    KNI_GetParameterAsInt(4));
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_AnimationTrack__1getController(void) {
    KNI_ReturnLong(funkey_m3g_animation_track_get_controller(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_AnimationTrack__1getSequence(void) {
    KNI_ReturnLong(funkey_m3g_animation_track_get_sequence(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_AnimationTrack__1getTargetProperty(void) {
    KNI_ReturnInt(funkey_m3g_animation_track_get_property(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_AnimationTrack__1setController(void) {
    funkey_m3g_animation_track_set_controller(M3G_LONG_PARAM(1),
                                               M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}


/* Appearance */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_APPEARANCE));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1getCompositingMode(void) {
    KNI_ReturnLong(funkey_m3g_appearance_get(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1getFog(void) {
    KNI_ReturnLong(funkey_m3g_appearance_get(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Appearance__1getLayer(void) {
    KNI_ReturnInt(funkey_m3g_appearance_get_layer(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1getMaterial(void) {
    KNI_ReturnLong(funkey_m3g_appearance_get(M3G_LONG_PARAM(1), 2));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1getPolygonMode(void) {
    KNI_ReturnLong(funkey_m3g_appearance_get(M3G_LONG_PARAM(1), 3));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Appearance__1getTexture(void) {
    KNI_ReturnLong(funkey_m3g_appearance_get_texture(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setCompositingMode(void) {
    funkey_m3g_appearance_set(M3G_LONG_PARAM(1), 0, M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setFog(void) {
    funkey_m3g_appearance_set(M3G_LONG_PARAM(1), 1, M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setLayer(void) {
    funkey_m3g_appearance_set_layer(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setMaterial(void) {
    funkey_m3g_appearance_set(M3G_LONG_PARAM(1), 2, M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setPolygonMode(void) {
    funkey_m3g_appearance_set(M3G_LONG_PARAM(1), 3, M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Appearance__1setTexture(void) {
    funkey_m3g_appearance_set_texture(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), M3G_LONG_PARAM(4));
    KNI_ReturnVoid();
}


/* CompositingMode */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_CompositingMode__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_COMPOSITING_MODE));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1enableColorWrite(void) {
    funkey_m3g_compositing_set_enable(M3G_LONG_PARAM(1), 3, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1enableDepthTest(void) {
    funkey_m3g_compositing_set_enable(M3G_LONG_PARAM(1), 1, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1enableDepthWrite(void) {
    funkey_m3g_compositing_set_enable(M3G_LONG_PARAM(1), 2, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_CompositingMode__1getAlphaThreshold(void) {
    KNI_ReturnFloat(funkey_m3g_compositing_get_alpha_threshold(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_CompositingMode__1getBlending(void) {
    KNI_ReturnInt(funkey_m3g_compositing_get_blending(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_CompositingMode__1getDepthOffsetFactor(void) {
    KNI_ReturnFloat(funkey_m3g_compositing_get_depth_offset(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_CompositingMode__1getDepthOffsetUnits(void) {
    KNI_ReturnFloat(funkey_m3g_compositing_get_depth_offset(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_CompositingMode__1isAlphaWriteEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_compositing_get_enable(M3G_LONG_PARAM(1), 0) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_CompositingMode__1isColorWriteEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_compositing_get_enable(M3G_LONG_PARAM(1), 3) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_CompositingMode__1isDepthTestEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_compositing_get_enable(M3G_LONG_PARAM(1), 1) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_CompositingMode__1isDepthWriteEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_compositing_get_enable(M3G_LONG_PARAM(1), 2) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1setAlphaThreshold(void) {
    funkey_m3g_compositing_set_alpha_threshold(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1setAlphaWriteEnable(void) {
    funkey_m3g_compositing_set_enable(M3G_LONG_PARAM(1), 0, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1setBlending(void) {
    funkey_m3g_compositing_set_blending(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_CompositingMode__1setDepthOffset(void) {
    funkey_m3g_compositing_set_depth_offset(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3), KNI_GetParameterAsFloat(4));
    KNI_ReturnVoid();
}


/* Fog */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Fog__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_FOG));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Fog__1getColor(void) {
    KNI_ReturnInt((int) funkey_m3g_fog_get_color(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Fog__1getDensity(void) {
    KNI_ReturnFloat(funkey_m3g_fog_get_density(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Fog__1getDistance(void) {
    KNI_ReturnFloat(funkey_m3g_fog_get_distance(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Fog__1getMode(void) {
    KNI_ReturnInt(funkey_m3g_fog_get_mode(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Fog__1setColor(void) {
    funkey_m3g_fog_set_color(M3G_LONG_PARAM(1), (unsigned int) KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Fog__1setDensity(void) {
    funkey_m3g_fog_set_density(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Fog__1setLinear(void) {
    funkey_m3g_fog_set_linear(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3), KNI_GetParameterAsFloat(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Fog__1setMode(void) {
    funkey_m3g_fog_set_mode(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}


/* Image2D */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Image2D__1ctorSize(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_IMAGE_2D);
    funkey_m3g_image_init(handle, KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4),
                          KNI_GetParameterAsInt(5), 0, 0, 1);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Image2D__1ctorSizePixels(void) {
    unsigned char *pixels;
    int len = m3g_read_byte_array(6, &pixels);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_IMAGE_2D);
    funkey_m3g_image_init(handle, KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4),
                          KNI_GetParameterAsInt(5), pixels, len, 0);
    if (pixels != 0) free(pixels);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Image2D__1ctorSizePixelsPalette(void) {
    unsigned char *pixels;
    unsigned char *palette;
    int len = m3g_read_byte_array(6, &pixels);
    int palette_len = m3g_read_byte_array(7, &palette);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_IMAGE_2D);
    funkey_m3g_image_init_palette(handle, KNI_GetParameterAsInt(3),
                                  KNI_GetParameterAsInt(4),
                                  KNI_GetParameterAsInt(5), pixels, len,
                                  palette, palette_len);
    if (pixels != 0) free(pixels);
    if (palette != 0) free(palette);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Image2D__1getFormat(void) {
    KNI_ReturnInt(funkey_m3g_image_get_format(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Image2D__1getHeight(void) {
    KNI_ReturnInt(funkey_m3g_image_get_height(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Image2D__1getWidth(void) {
    KNI_ReturnInt(funkey_m3g_image_get_width(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Image2D__1isMutable(void) {
    KNI_ReturnBoolean(funkey_m3g_image_is_mutable(M3G_LONG_PARAM(1)) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Image2D__1set(void) {
    unsigned char *pixels;
    int len = m3g_read_byte_array(8, &pixels);
    funkey_m3g_image_set(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4),
                         KNI_GetParameterAsInt(5), KNI_GetParameterAsInt(6), pixels, len);
    if (pixels != 0) free(pixels);
    KNI_ReturnVoid();
}


/* KeyframeSequence */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_KeyframeSequence__1ctor(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_KEYFRAME_SEQUENCE);
    funkey_m3g_keyframe_init(handle, KNI_GetParameterAsInt(3),
                             KNI_GetParameterAsInt(4),
                             KNI_GetParameterAsInt(5));
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getComponentCount(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getDuration(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getInterpolationType(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 2));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getKeyframe(void) {
    float *values = 0;
    int count = 0;
    int time;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(4, array);
    if (!KNI_IsNullHandle(array)) {
        count = KNI_GetArrayLength(array);
        if (count > 0) {
            values = (float *) malloc((size_t) count * sizeof(float));
        }
    }
    KNI_EndHandles();
    time = funkey_m3g_keyframe_get(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                                   values, count);
    if (values != 0) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(array);
        KNI_GetParameterAsObject(4, array);
        if (!KNI_IsNullHandle(array)) {
            KNI_SetRawArrayRegion(array, 0, count * (int) sizeof(float),
                                  (const jbyte *) values);
        }
        KNI_EndHandles();
        free(values);
    }
    KNI_ReturnInt(time);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getKeyframeCount(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 3));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getRepeatMode(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 4));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getValidRangeFirst(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 5));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_KeyframeSequence__1getValidRangeLast(void) {
    KNI_ReturnInt(funkey_m3g_keyframe_get_int(M3G_LONG_PARAM(1), 6));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_KeyframeSequence__1setDuration(void) {
    funkey_m3g_keyframe_set_duration(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_KeyframeSequence__1setKeyframe(void) {
    float *values = 0;
    int count = m3g_read_float_array(5, &values);
    funkey_m3g_keyframe_set(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                            KNI_GetParameterAsInt(4), values, count);
    if (values != 0) {
        free(values);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_KeyframeSequence__1setRepeatMode(void) {
    funkey_m3g_keyframe_set_repeat(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_KeyframeSequence__1setValidRange(void) {
    funkey_m3g_keyframe_set_valid_range(M3G_LONG_PARAM(1),
                                        KNI_GetParameterAsInt(3),
                                        KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}


/* Light */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Light__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_LIGHT));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Light__1getAttenuation(void) {
    KNI_ReturnFloat(funkey_m3g_light_get_attenuation(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Light__1getColor(void) {
    KNI_ReturnInt((int) funkey_m3g_light_get_color(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Light__1getIntensity(void) {
    KNI_ReturnFloat(funkey_m3g_light_get_intensity(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Light__1getMode(void) {
    KNI_ReturnInt(funkey_m3g_light_get_mode(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Light__1getSpotAngle(void) {
    KNI_ReturnFloat(funkey_m3g_light_get_spot(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Light__1getSpotExponent(void) {
    KNI_ReturnFloat(funkey_m3g_light_get_spot(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setAttenuation(void) {
    funkey_m3g_light_set_attenuation(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3), KNI_GetParameterAsFloat(4), KNI_GetParameterAsFloat(5));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setColor(void) {
    funkey_m3g_light_set_color(M3G_LONG_PARAM(1), (unsigned int) KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setIntensity(void) {
    funkey_m3g_light_set_intensity(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setMode(void) {
    funkey_m3g_light_set_mode(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setSpotAngle(void) {
    funkey_m3g_light_set_spot(M3G_LONG_PARAM(1), 0, KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Light__1setSpotExponent(void) {
    funkey_m3g_light_set_spot(M3G_LONG_PARAM(1), 1, KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}


/* Loader */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Loader__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_loader(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Loader__1decodeData(void) {
    int result = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(4, array);
    if (!KNI_IsNullHandle(array)) {
        int offset = KNI_GetParameterAsInt(3);
        int len = KNI_GetArrayLength(array);
        unsigned char *data;
        if (offset < 0) {
            offset = 0;
        }
        if (offset > len) {
            offset = len;
        }
        data = (unsigned char *) malloc((size_t) (len - offset));
        if (data != 0) {
            KNI_GetRawArrayRegion(array, offset, len - offset, (jbyte *) data);
            result = funkey_m3g_loader_decode(M3G_LONG_PARAM(1), len - offset, data);
            free(data);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Loader__1getLoadedObjects(void) {
    int result;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(3, array);
    if (KNI_IsNullHandle(array)) {
        result = funkey_m3g_loader_get_loaded_objects(M3G_LONG_PARAM(1), 0, 0);
    } else {
        int len = KNI_GetArrayLength(array);
        long tmp[512];
        int i;
        jlong out[512];
        if (len > 512) {
            len = 512;
        }
        result = funkey_m3g_loader_get_loaded_objects(M3G_LONG_PARAM(1), tmp, len);
        for (i = 0; i < result; ++i) {
            out[i] = (jlong) tmp[i];
        }
        if (result > 0) {
            KNI_SetRawArrayRegion(array, 0, result * (int) sizeof(jlong),
                                  (const jbyte *) out);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Loader__1getNumUserParameters(void) {
    KNI_ReturnInt(funkey_m3g_loader_get_num_user_params(M3G_LONG_PARAM(1),
                                                        KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Loader__1getObjectsWithUserParameters(void) {
    int result;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(3, array);
    if (KNI_IsNullHandle(array)) {
        result = funkey_m3g_loader_get_objects_with_user_params(M3G_LONG_PARAM(1),
                                                                0, 0);
    } else {
        int len = KNI_GetArrayLength(array);
        long tmp[512];
        int i;
        jlong out[512];
        if (len > 512) {
            len = 512;
        }
        result = funkey_m3g_loader_get_objects_with_user_params(M3G_LONG_PARAM(1),
                                                                tmp, len);
        for (i = 0; i < result; ++i) {
            out[i] = (jlong) tmp[i];
        }
        if (result > 0) {
            KNI_SetRawArrayRegion(array, 0, result * (int) sizeof(jlong),
                                  (const jbyte *) out);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Loader__1getUserParameter(void) {
    int result = 0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(5, array);
    if (KNI_IsNullHandle(array)) {
        result = funkey_m3g_loader_get_user_param(M3G_LONG_PARAM(1),
                                                  KNI_GetParameterAsInt(3),
                                                  KNI_GetParameterAsInt(4),
                                                  0, 0);
    } else {
        int len = KNI_GetArrayLength(array);
        int source_len = funkey_m3g_loader_get_user_param(M3G_LONG_PARAM(1),
                                                          KNI_GetParameterAsInt(3),
                                                          KNI_GetParameterAsInt(4),
                                                          0, 0);
        signed char *tmp = 0;
        if (source_len >= 0) {
            size_t alloc_len = source_len > 0 ? (size_t) source_len : 1u;
            tmp = (signed char *) malloc(alloc_len);
        }
        if (tmp != 0) {
            result = funkey_m3g_loader_get_user_param(M3G_LONG_PARAM(1),
                                                      KNI_GetParameterAsInt(3),
                                                      KNI_GetParameterAsInt(4),
                                                      tmp, source_len);
            if (tmp != 0 && len > 0) {
                int copy_len = source_len < len ? source_len : len;
                KNI_SetRawArrayRegion(array, 0, copy_len,
                                      (const jbyte *) tmp);
            }
        }
        if (tmp != 0) {
            free(tmp);
        }
    }
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Loader__1inflate(void) {
    int src_len = 0;
    int dst_len = 0;
    unsigned char *src = 0;
    int ok = 0;
    KNI_StartHandles(2);
    KNI_DeclareHandle(src_array);
    KNI_DeclareHandle(dst_array);
    KNI_GetParameterAsObject(1, src_array);
    KNI_GetParameterAsObject(2, dst_array);
    if (!KNI_IsNullHandle(src_array) && !KNI_IsNullHandle(dst_array)) {
        src_len = KNI_GetArrayLength(src_array);
        dst_len = KNI_GetArrayLength(dst_array);
        if (src_len > 0 && dst_len >= 0) {
            src = (unsigned char *) malloc((size_t) src_len);
            if (src != 0) {
                z_stream zs;
                unsigned char chunk[1024];
                int status;
                int out_pos = 0;
                memset(&zs, 0, sizeof(zs));
                KNI_GetRawArrayRegion(src_array, 0, src_len, (jbyte *) src);
                zs.next_in = src;
                zs.avail_in = (uInt) src_len;
                status = inflateInit(&zs);
                while (status == Z_OK && out_pos < dst_len) {
                    int produced;
                    zs.next_out = chunk;
                    zs.avail_out = sizeof(chunk);
                    status = inflate(&zs, Z_NO_FLUSH);
                    produced = (int) sizeof(chunk) - (int) zs.avail_out;
                    if (produced > dst_len - out_pos) {
                        produced = dst_len - out_pos;
                    }
                    if (produced > 0) {
                        KNI_SetRawArrayRegion(dst_array, out_pos, produced,
                                              (const jbyte *) chunk);
                        out_pos += produced;
                    }
                }
                inflateEnd(&zs);
                free(src);
                ok = (status == Z_STREAM_END && out_pos == dst_len);
            }
        }
    }
    KNI_EndHandles();
    KNI_ReturnBoolean(ok ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Loader__1setExternalReferences(void) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(3, array);
    if (!KNI_IsNullHandle(array)) {
        int len = KNI_GetArrayLength(array);
        jlong in[512];
        long refs[512];
        int i;
        if (len > 512) {
            len = 512;
        }
        KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(jlong),
                              (jbyte *) in);
        for (i = 0; i < len; ++i) {
            refs[i] = (long) in[i];
        }
        funkey_m3g_loader_set_external_refs(M3G_LONG_PARAM(1), refs, len);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}


/* Material */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Material__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_MATERIAL));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Material__1getColor(void) {
    KNI_ReturnInt((int) funkey_m3g_material_get_color(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_javax_microedition_m3g_Material__1getShininess(void) {
    KNI_ReturnFloat(funkey_m3g_material_get_shininess(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Material__1isVertexColorTrackingEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_material_get_vertex_color_tracking(M3G_LONG_PARAM(1)) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Material__1setColor(void) {
    funkey_m3g_material_set_color(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), (unsigned int) KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Material__1setShininess(void) {
    funkey_m3g_material_set_shininess(M3G_LONG_PARAM(1), KNI_GetParameterAsFloat(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Material__1setVertexColorTrackingEnable(void) {
    funkey_m3g_material_set_vertex_color_tracking(M3G_LONG_PARAM(1), KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}


/* Mesh */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Mesh__1ctor(void) {
    long *triangles;
    long *appearances;
    int tri_count = m3g_read_long_array(5, &triangles);
    int app_count = m3g_read_long_array(6, &appearances);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_MESH);
    funkey_m3g_mesh_init(handle, M3G_LONG_PARAM(3), triangles, appearances, tri_count);
    (void) app_count;
    if (triangles != 0) free(triangles);
    if (appearances != 0) free(appearances);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Mesh__1getAppearance(void) {
    KNI_ReturnLong(funkey_m3g_mesh_get_appearance(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Mesh__1getIndexBuffer(void) {
    KNI_ReturnLong(funkey_m3g_mesh_get_index_buffer(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Mesh__1getSubmeshCount(void) {
    KNI_ReturnInt(funkey_m3g_mesh_get_submesh_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Mesh__1getVertexBuffer(void) {
    KNI_ReturnLong(funkey_m3g_mesh_get_vertex_buffer(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Mesh__1setAppearance(void) {
    funkey_m3g_mesh_set_appearance(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), M3G_LONG_PARAM(4));
    KNI_ReturnVoid();
}


/* MorphingMesh */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_MorphingMesh__1ctor(void) {
    long *targets;
    long *triangles;
    long *appearances;
    int target_count = m3g_read_long_array(5, &targets);
    int tri_count = m3g_read_long_array(6, &triangles);
    int app_count = m3g_read_long_array(7, &appearances);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_MORPHING_MESH);
    (void) app_count;
    funkey_m3g_morphing_mesh_init(handle, M3G_LONG_PARAM(3),
                                  targets, target_count,
                                  triangles, appearances, tri_count);
    if (targets != 0) free(targets);
    if (triangles != 0) free(triangles);
    if (appearances != 0) free(appearances);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_MorphingMesh__1getMorphTarget(void) {
    KNI_ReturnLong(funkey_m3g_morphing_mesh_get_target(M3G_LONG_PARAM(1),
                                                        KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_MorphingMesh__1getMorphTargetCount(void) {
    KNI_ReturnInt(funkey_m3g_morphing_mesh_get_target_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_MorphingMesh__1getWeights(void) {
    float *weights;
    int len = m3g_read_float_array(3, &weights);
    if (weights != 0) {
        funkey_m3g_morphing_mesh_get_weights(M3G_LONG_PARAM(1), weights, len);
        m3g_set_float_array_param(3, weights, len);
        free(weights);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_MorphingMesh__1setWeights(void) {
    float *weights;
    int len = m3g_read_float_array(3, &weights);
    if (weights != 0) {
        funkey_m3g_morphing_mesh_set_weights(M3G_LONG_PARAM(1), weights, len);
        free(weights);
    }
    KNI_ReturnVoid();
}


/* PolygonMode */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_PolygonMode__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_POLYGON_MODE));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_PolygonMode__1getCulling(void) {
    KNI_ReturnInt(funkey_m3g_polygon_get_mode(M3G_LONG_PARAM(1), 0));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_PolygonMode__1getShading(void) {
    KNI_ReturnInt(funkey_m3g_polygon_get_mode(M3G_LONG_PARAM(1), 2));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_PolygonMode__1getWinding(void) {
    KNI_ReturnInt(funkey_m3g_polygon_get_mode(M3G_LONG_PARAM(1), 1));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_PolygonMode__1isLocalCameraLightingEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_polygon_get_enable(M3G_LONG_PARAM(1), 1) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_PolygonMode__1isPerspectiveCorrectionEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_polygon_get_enable(M3G_LONG_PARAM(1), 2) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_PolygonMode__1isTwoSidedLightingEnabled(void) {
    KNI_ReturnBoolean(funkey_m3g_polygon_get_enable(M3G_LONG_PARAM(1), 0) ? KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setCulling(void) {
    funkey_m3g_polygon_set_mode(M3G_LONG_PARAM(1), 0, KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setLocalCameraLightingEnable(void) {
    funkey_m3g_polygon_set_enable(M3G_LONG_PARAM(1), 1, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setPerspectiveCorrectionEnable(void) {
    funkey_m3g_polygon_set_enable(M3G_LONG_PARAM(1), 2, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setShading(void) {
    funkey_m3g_polygon_set_mode(M3G_LONG_PARAM(1), 2, KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setTwoSidedLightingEnable(void) {
    funkey_m3g_polygon_set_enable(M3G_LONG_PARAM(1), 0, KNI_GetParameterAsBoolean(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_PolygonMode__1setWinding(void) {
    funkey_m3g_polygon_set_mode(M3G_LONG_PARAM(1), 1, KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}


/* SkinnedMesh */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_SkinnedMesh__1addTransform(void) {
    funkey_m3g_skinned_mesh_add_transform(M3G_LONG_PARAM(1),
                                          M3G_LONG_PARAM(3),
                                          KNI_GetParameterAsInt(5),
                                          KNI_GetParameterAsInt(6),
                                          KNI_GetParameterAsInt(7));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_SkinnedMesh__1ctor(void) {
    long *triangles;
    long *appearances;
    int tri_count = m3g_read_long_array(5, &triangles);
    int app_count = m3g_read_long_array(6, &appearances);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_SKINNED_MESH);
    (void) app_count;
    funkey_m3g_skinned_mesh_init(handle, M3G_LONG_PARAM(3),
                                 triangles, appearances, tri_count,
                                 M3G_LONG_PARAM(7));
    if (triangles != 0) free(triangles);
    if (appearances != 0) free(appearances);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_SkinnedMesh__1getBoneTransform(void) {
    float matrix[16];
    funkey_m3g_skinned_mesh_get_bone_transform(M3G_LONG_PARAM(1),
                                               M3G_LONG_PARAM(3), matrix);
    m3g_write_matrix_param(5, matrix);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_SkinnedMesh__1getBoneVertices(void) {
    int *indices = 0;
    float *weights = 0;
    int *tmp_indices = 0;
    float *tmp_weights = 0;
    int index_len = m3g_read_int_array(5, &indices);
    int weight_len = m3g_read_float_array(6, &weights);
    int count = funkey_m3g_skinned_mesh_get_bone_vertices(M3G_LONG_PARAM(1),
                                                          M3G_LONG_PARAM(3),
                                                          0, 0);
    if (count > 0 && indices != 0 && weights != 0) {
        int copy_count = count;
        if (copy_count > index_len) copy_count = index_len;
        if (copy_count > weight_len) copy_count = weight_len;
        tmp_indices = (int *) malloc((size_t) count * sizeof(int));
        tmp_weights = (float *) malloc((size_t) count * sizeof(float));
        if (tmp_indices != 0 && tmp_weights != 0) {
            funkey_m3g_skinned_mesh_get_bone_vertices(M3G_LONG_PARAM(1),
                                                      M3G_LONG_PARAM(3),
                                                      tmp_indices,
                                                      tmp_weights);
            if (copy_count > 0) {
                m3g_set_int_array_param(5, tmp_indices, copy_count);
                m3g_set_float_array_param(6, tmp_weights, copy_count);
            }
        }
    }
    if (indices != 0) free(indices);
    if (weights != 0) free(weights);
    if (tmp_indices != 0) free(tmp_indices);
    if (tmp_weights != 0) free(tmp_weights);
    KNI_ReturnInt(count);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_SkinnedMesh__1getSkeleton(void) {
    KNI_ReturnLong(funkey_m3g_skinned_mesh_get_skeleton(M3G_LONG_PARAM(1)));
}


/* Sprite3D */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Sprite3D__1ctor(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_SPRITE_3D);
    funkey_m3g_sprite_init(handle, KNI_GetParameterAsBoolean(3),
                           M3G_LONG_PARAM(4), M3G_LONG_PARAM(6));
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Sprite3D__1getAppearance(void) {
    KNI_ReturnLong(funkey_m3g_sprite_get_appearance(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Sprite3D__1getCrop(void) {
    KNI_ReturnInt(funkey_m3g_sprite_get_crop(M3G_LONG_PARAM(1),
                                             KNI_GetParameterAsInt(3)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Sprite3D__1getImage(void) {
    KNI_ReturnLong(funkey_m3g_sprite_get_image(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_m3g_Sprite3D__1isScaled(void) {
    KNI_ReturnBoolean(funkey_m3g_sprite_is_scaled(M3G_LONG_PARAM(1)) ?
                      KNI_TRUE : KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Sprite3D__1setAppearance(void) {
    funkey_m3g_sprite_set_appearance(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Sprite3D__1setCrop(void) {
    funkey_m3g_sprite_set_crop(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                               KNI_GetParameterAsInt(4),
                               KNI_GetParameterAsInt(5),
                               KNI_GetParameterAsInt(6));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Sprite3D__1setImage(void) {
    funkey_m3g_sprite_set_image(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}


/* Texture2D */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Texture2D__1ctor(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_TEXTURE_2D);
    funkey_m3g_texture_set_image(handle, M3G_LONG_PARAM(3));
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getBlendColor(void) {
    KNI_ReturnInt((int) funkey_m3g_texture_get_blend_color(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getBlending(void) {
    KNI_ReturnInt(funkey_m3g_texture_get_blending(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_Texture2D__1getImage(void) {
    KNI_ReturnLong(funkey_m3g_texture_get_image(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getImageFilter(void) {
    KNI_ReturnInt(funkey_m3g_texture_get_image_filter(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getLevelFilter(void) {
    KNI_ReturnInt(funkey_m3g_texture_get_level_filter(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getWrappingS(void) {
    KNI_ReturnInt(funkey_m3g_texture_get_wrapping_s(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_Texture2D__1getWrappingT(void) {
    KNI_ReturnInt(funkey_m3g_texture_get_wrapping_t(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Texture2D__1setBlendColor(void) {
    funkey_m3g_texture_set_blend_color(M3G_LONG_PARAM(1), (unsigned int) KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Texture2D__1setBlending(void) {
    funkey_m3g_texture_set_blending(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Texture2D__1setFiltering(void) {
    funkey_m3g_texture_set_filtering(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Texture2D__1setImage(void) {
    funkey_m3g_texture_set_image(M3G_LONG_PARAM(1), M3G_LONG_PARAM(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Texture2D__1setWrapping(void) {
    funkey_m3g_texture_set_wrapping(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), KNI_GetParameterAsInt(4));
    KNI_ReturnVoid();
}


/* Transform */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1getMatrix(void) {
    float m[16];
    m3g_read_matrix_param(1, m);
    m3g_set_float_array_param(2, m, 16);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1invert(void) {
    float m[16], out[16];
    m3g_read_matrix_param(1, m);
    if (m3g_matrix_invert(out, m)) {
        m3g_write_matrix_param(1, out);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1mul(void) {
    float a[16], b[16], out[16];
    m3g_read_matrix_param(2, a);
    m3g_read_matrix_param(3, b);
    m3g_matrix_mul(out, a, b);
    m3g_write_matrix_param(1, out);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1rotate(void) {
    float m[16], r[16];
    m3g_read_matrix_param(1, m);
    m3g_matrix_rotate_axis(r, KNI_GetParameterAsFloat(2),
                           KNI_GetParameterAsFloat(3),
                           KNI_GetParameterAsFloat(4),
                           KNI_GetParameterAsFloat(5));
    m3g_matrix_mul(m, m, r);
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1rotateQuat(void) {
    float m[16], r[16];
    m3g_read_matrix_param(1, m);
    m3g_matrix_rotate_quat(r, KNI_GetParameterAsFloat(2),
                           KNI_GetParameterAsFloat(3),
                           KNI_GetParameterAsFloat(4),
                           KNI_GetParameterAsFloat(5));
    m3g_matrix_mul(m, m, r);
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1scale(void) {
    float m[16], s[16];
    m3g_read_matrix_param(1, m);
    m3g_matrix_identity(s);
    s[0] = KNI_GetParameterAsFloat(2);
    s[5] = KNI_GetParameterAsFloat(3);
    s[10] = KNI_GetParameterAsFloat(4);
    m3g_matrix_mul(m, m, s);
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1setIdentity(void) {
    float m[16];
    m3g_matrix_identity(m);
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1setMatrix(void) {
    float m[16];
    float *src;
    int len = m3g_read_float_array(2, &src);
    int i;
    m3g_matrix_identity(m);
    if (src != 0) {
        for (i = 0; i < len && i < 16; ++i) m[i] = src[i];
        free(src);
    }
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1transformArray(void) {
    float m[16];
    float *out;
    int len = m3g_read_float_array(4, &out);
    m3g_read_matrix_param(1, m);
    if (out != 0) {
        funkey_m3g_vertex_array_transform(M3G_LONG_PARAM(2), m, out, len,
                                          KNI_GetParameterAsBoolean(5));
        m3g_set_float_array_param(4, out, len);
        free(out);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1transformTable(void) {
    float m[16];
    float *v;
    int len = m3g_read_float_array(2, &v);
    int i;
    m3g_read_matrix_param(1, m);
    if (v != 0) {
        for (i = 0; i + 3 < len; i += 4) {
            float x = v[i], y = v[i + 1], z = v[i + 2], w = v[i + 3];
            v[i] = m[0]*x + m[1]*y + m[2]*z + m[3]*w;
            v[i + 1] = m[4]*x + m[5]*y + m[6]*z + m[7]*w;
            v[i + 2] = m[8]*x + m[9]*y + m[10]*z + m[11]*w;
            v[i + 3] = m[12]*x + m[13]*y + m[14]*z + m[15]*w;
        }
        m3g_set_float_array_param(2, v, len);
        free(v);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1translate(void) {
    float m[16], t[16];
    m3g_read_matrix_param(1, m);
    m3g_matrix_identity(t);
    t[3] = KNI_GetParameterAsFloat(2);
    t[7] = KNI_GetParameterAsFloat(3);
    t[11] = KNI_GetParameterAsFloat(4);
    m3g_matrix_mul(m, m, t);
    m3g_write_matrix_param(1, m);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_Transform__1transpose(void) {
    float m[16], out[16];
    int r, c;
    m3g_read_matrix_param(1, m);
    for (r = 0; r < 4; ++r) for (c = 0; c < 4; ++c) out[r * 4 + c] = m[c * 4 + r];
    m3g_write_matrix_param(1, out);
    KNI_ReturnVoid();
}


/* TriangleStripArray */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_TriangleStripArray__1createExplicit(void) {
    int *indices;
    int *lengths;
    int len = m3g_read_int_array(3, &indices);
    int length_count = m3g_read_int_array(4, &lengths);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_INDEX_BUFFER);
    funkey_m3g_index_buffer_init_strips(handle, 0, indices, len,
                                        lengths, length_count);
    if (indices != 0) free(indices);
    if (lengths != 0) free(lengths);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_TriangleStripArray__1createImplicit(void) {
    int *lengths;
    int len = m3g_read_int_array(4, &lengths);
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_INDEX_BUFFER);
    funkey_m3g_index_buffer_init_strips(handle, KNI_GetParameterAsInt(3),
                                        0, 0, lengths, len);
    if (lengths != 0) free(lengths);
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_TriangleStripArray__1getIndexCount(void) {
    KNI_ReturnInt(funkey_m3g_index_buffer_get_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_TriangleStripArray__1getIndices(void) {
    int count = funkey_m3g_index_buffer_get_count(M3G_LONG_PARAM(1));
    int *indices = count > 0 ? (int *) malloc((size_t) count * sizeof(int)) : 0;
    if (indices != 0) {
        funkey_m3g_index_buffer_get_indices(M3G_LONG_PARAM(1), indices, count);
        m3g_set_int_array_param(3, indices, count);
        free(indices);
    }
    KNI_ReturnVoid();
}


/* VertexArray */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_VertexArray__1ctor(void) {
    long handle = funkey_m3g_create_object(FUNKEY_M3G_CLASS_VERTEX_ARRAY);
    funkey_m3g_vertex_array_init(handle, KNI_GetParameterAsInt(3),
                                 KNI_GetParameterAsInt(4),
                                 KNI_GetParameterAsInt(5));
    KNI_ReturnLong(handle);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexArray__1getByte(void) {
    int count = KNI_GetParameterAsInt(4);
    int comps = funkey_m3g_vertex_array_get_component_count(M3G_LONG_PARAM(1));
    int total = count * comps;
    int *values = total > 0 ? (int *) malloc((size_t) total * sizeof(int)) : 0;
    unsigned char *bytes = total > 0 ? (unsigned char *) malloc((size_t) total) : 0;
    int i;
    if (values != 0 && bytes != 0) {
        funkey_m3g_vertex_array_get(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), count, values, total);
        for (i = 0; i < total; ++i) bytes[i] = (unsigned char) values[i];
        KNI_StartHandles(1);
        KNI_DeclareHandle(array);
        KNI_GetParameterAsObject(5, array);
        if (!KNI_IsNullHandle(array)) {
            int len = KNI_GetArrayLength(array);
            if (total > len) total = len;
            KNI_SetRawArrayRegion(array, 0, total, (const jbyte *) bytes);
        }
        KNI_EndHandles();
    }
    if (values != 0) free(values);
    if (bytes != 0) free(bytes);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_VertexArray__1getComponentCount(void) {
    KNI_ReturnInt(funkey_m3g_vertex_array_get_component_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_VertexArray__1getComponentType(void) {
    KNI_ReturnInt(funkey_m3g_vertex_array_get_component_type(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexArray__1getShort(void) {
    int count = KNI_GetParameterAsInt(4);
    int comps = funkey_m3g_vertex_array_get_component_count(M3G_LONG_PARAM(1));
    int total = count * comps;
    int *values = total > 0 ? (int *) malloc((size_t) total * sizeof(int)) : 0;
    jshort *shorts = total > 0 ? (jshort *) malloc((size_t) total * sizeof(jshort)) : 0;
    int i;
    if (values != 0 && shorts != 0) {
        funkey_m3g_vertex_array_get(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), count, values, total);
        for (i = 0; i < total; ++i) shorts[i] = (jshort) values[i];
        KNI_StartHandles(1);
        KNI_DeclareHandle(array);
        KNI_GetParameterAsObject(5, array);
        if (!KNI_IsNullHandle(array)) {
            int len = KNI_GetArrayLength(array);
            if (total > len) total = len;
            KNI_SetRawArrayRegion(array, 0, total * (int) sizeof(jshort), (const jbyte *) shorts);
        }
        KNI_EndHandles();
    }
    if (values != 0) free(values);
    if (shorts != 0) free(shorts);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_VertexArray__1getVertexCount(void) {
    KNI_ReturnInt(funkey_m3g_vertex_array_get_vertex_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexArray__1setByte(void) {
    unsigned char *bytes;
    int len = m3g_read_byte_array(5, &bytes);
    int *values = 0;
    int i;
    if (len > 0 && bytes != 0) {
        values = (int *) malloc((size_t) len * sizeof(int));
        if (values != 0) {
            for (i = 0; i < len; ++i) values[i] = (signed char) bytes[i];
            funkey_m3g_vertex_array_set(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                                        KNI_GetParameterAsInt(4), values, len);
            free(values);
        }
        free(bytes);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexArray__1setShort(void) {
    int len = 0;
    jshort *shorts = 0;
    int *values = 0;
    int i;
    KNI_StartHandles(1);
    KNI_DeclareHandle(array);
    KNI_GetParameterAsObject(5, array);
    if (!KNI_IsNullHandle(array)) {
        len = KNI_GetArrayLength(array);
        shorts = (jshort *) malloc((size_t) len * sizeof(jshort));
        values = (int *) malloc((size_t) len * sizeof(int));
        if (shorts != 0 && values != 0) {
            KNI_GetRawArrayRegion(array, 0, len * (int) sizeof(jshort), (jbyte *) shorts);
            for (i = 0; i < len; ++i) values[i] = shorts[i];
            funkey_m3g_vertex_array_set(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3),
                                        KNI_GetParameterAsInt(4), values, len);
        }
    }
    KNI_EndHandles();
    if (shorts != 0) free(shorts);
    if (values != 0) free(values);
    KNI_ReturnVoid();
}


/* VertexBuffer */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_VertexBuffer__1ctor(void) {
    KNI_ReturnLong(funkey_m3g_create_object(FUNKEY_M3G_CLASS_VERTEX_BUFFER));
}

KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_m3g_VertexBuffer__1getArray(void) {
    float scale_bias[4];
    long array = funkey_m3g_vertex_buffer_get_array(M3G_LONG_PARAM(1), KNI_GetParameterAsInt(3), scale_bias, 4);
    m3g_set_float_array_param(4, scale_bias, 4);
    KNI_ReturnLong(array);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_VertexBuffer__1getDefaultColor(void) {
    KNI_ReturnInt((int) funkey_m3g_vertex_buffer_get_default_color(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_m3g_VertexBuffer__1getVertexCount(void) {
    KNI_ReturnInt(funkey_m3g_vertex_buffer_get_vertex_count(M3G_LONG_PARAM(1)));
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexBuffer__1setColors(void) {
    funkey_m3g_vertex_buffer_set_array(M3G_LONG_PARAM(1), 2, M3G_LONG_PARAM(3), 1.0f, 0, 0);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexBuffer__1setDefaultColor(void) {
    funkey_m3g_vertex_buffer_set_default_color(M3G_LONG_PARAM(1),
                                               (unsigned int) KNI_GetParameterAsInt(3));
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexBuffer__1setNormals(void) {
    funkey_m3g_vertex_buffer_set_array(M3G_LONG_PARAM(1), 1, M3G_LONG_PARAM(3), 1.0f, 0, 0);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexBuffer__1setTexCoords(void) {
    float *bias;
    int len = m3g_read_float_array(7, &bias);
    funkey_m3g_vertex_buffer_set_array(M3G_LONG_PARAM(1), 3 + KNI_GetParameterAsInt(3),
                                       M3G_LONG_PARAM(4), KNI_GetParameterAsFloat(6),
                                       bias, len);
    if (bias != 0) free(bias);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_m3g_VertexBuffer__1setVertices(void) {
    float *bias;
    int len = m3g_read_float_array(6, &bias);
    funkey_m3g_vertex_buffer_set_array(M3G_LONG_PARAM(1), 0, M3G_LONG_PARAM(3),
                                       KNI_GetParameterAsFloat(5), bias, len);
    if (bias != 0) free(bias);
    KNI_ReturnVoid();
}
