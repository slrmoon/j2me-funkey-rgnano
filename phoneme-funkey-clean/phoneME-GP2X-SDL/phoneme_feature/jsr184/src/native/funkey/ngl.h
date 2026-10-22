/*
 * Minimal NGL compatibility layer for building m3gcore without EGL/GLES.
 *
 * The FunKey target has SDL but no OpenGL ES development headers. m3gcore's
 * NGL mode expects an OpenGL ES-like API surface, so this header provides the
 * required fixed-function types, constants and entry points. The companion
 * software backend keeps enough fixed-function state for m3gcore to be the
 * source of truth for scene traversal, transforms, texture binding and draw
 * submission on devices without GLES.
 */
#ifndef FUNKEY_M3G_NGL_H
#define FUNKEY_M3G_NGL_H

#include <stddef.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef unsigned char GLubyte;
typedef short GLshort;
typedef unsigned short GLushort;
typedef float GLfloat;
typedef int GLfixed;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NO_ERROR 0
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FIXED 0x140C

#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_PALETTE8_RGB8_OES 0x8B95
#define GL_PALETTE8_RGBA8_OES 0x8B96

/*
 * m3gcore keeps NGL textures in compact or padded internal formats which
 * vanilla GLES does not describe.  Keep those formats explicit across the
 * m3gcore/backend boundary instead of guessing a byte layout in the backend.
 */
#define GL_M3G_LUMINANCE_ALPHA4 0x7000
#define GL_M3G_RGB565 0x7001
#define GL_M3G_RGB8_32 0x7002
#define GL_M3G_BGR8_32 0x7003
#define GL_M3G_BGRA8 0x7004
#define GL_M3G_ARGB8 0x7005
#define GL_M3G_PALETTE8_RGB8_32 0x7006

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE 0x1702
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TEXTURE_ENV_COLOR 0x2201
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_GENERATE_MIPMAP 0x8191
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901
#define GL_REPLACE 0x1E01
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
#define GL_ADD 0x0104

#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TRIANGLE_STRIP 0x0005

#define GL_DEPTH_TEST 0x0B71
#define GL_ALPHA_TEST 0x0BC0
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44
#define GL_LIGHTING 0x0B50
#define GL_LIGHT0 0x4000
#define GL_LIGHT7 0x4007
#define GL_NORMALIZE 0x0BA1
#define GL_SCISSOR_TEST 0x0C11
#define GL_MULTISAMPLE 0x809D
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_COLOR_MATERIAL 0x0B57
#define GL_FRONT_AND_BACK 0x0408
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50

#define GL_ALWAYS 0x0207
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ZERO 0
#define GL_ONE 1
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_SRC_ALPHA_SATURATE 0x0308

#define GL_FOG 0x0B60
#define GL_FOG_MODE 0x0B65
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_COLOR 0x0B66
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
#define GL_LINEAR_FOG 0x2601

#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_EMISSION 0x1600
#define GL_SHININESS 0x1601
#define GL_POSITION 0x1203
#define GL_SPOT_DIRECTION 0x1204
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_LIGHT_MODEL_TWO_SIDE 0x0B52

#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_RENDERER 0x1F01
#define GL_UNPACK_ALIGNMENT 0x0CF5

#define NGL_MIN_WORKING_MEMORY_BYTES (256 * 1024)
#define NGL_TEXTURE_STRUCT_SIZE 1024
#define NGL_ES_BIT 1
#define NGL_ES2_BIT_KHR 2

#include "ngl_software.h"

#endif
