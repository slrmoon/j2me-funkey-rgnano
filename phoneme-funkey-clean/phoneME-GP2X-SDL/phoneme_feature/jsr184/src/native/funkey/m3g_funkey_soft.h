/*
 * FunKey JSR-184 software render target.
 *
 * This is the phoneME-facing boundary for the M3G renderer. The first native
 * port milestone uses it to bind an LCDUI Graphics target without requiring
 * EGL/GLES. The rasterizer work lands behind this interface.
 */

#ifndef FUNKEY_M3G_SOFT_H
#define FUNKEY_M3G_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FunKeyM3GSurface {
    int width;
    int height;
    int clip_x;
    int clip_y;
    int clip_w;
    int clip_h;
    unsigned int clear_argb;
    unsigned short *pixels;
    int stride;
    float *depth;
    int depth_count;
} FunKeyM3GSurface;

enum {
    FUNKEY_M3G_CLASS_ANIMATION_CONTROLLER = 0x01,
    FUNKEY_M3G_CLASS_ANIMATION_TRACK = 0x02,
    FUNKEY_M3G_CLASS_APPEARANCE = 0x03,
    FUNKEY_M3G_CLASS_BACKGROUND = 0x04,
    FUNKEY_M3G_CLASS_CAMERA = 0x05,
    FUNKEY_M3G_CLASS_COMPOSITING_MODE = 0x06,
    FUNKEY_M3G_CLASS_FOG = 0x07,
    FUNKEY_M3G_CLASS_GROUP = 0x08,
    FUNKEY_M3G_CLASS_IMAGE_2D = 0x09,
    FUNKEY_M3G_CLASS_INDEX_BUFFER = 0x0A,
    FUNKEY_M3G_CLASS_KEYFRAME_SEQUENCE = 0x0B,
    FUNKEY_M3G_CLASS_LIGHT = 0x0C,
    FUNKEY_M3G_CLASS_LOADER = 0x0D,
    FUNKEY_M3G_CLASS_MATERIAL = 0x0E,
    FUNKEY_M3G_CLASS_MESH = 0x0F,
    FUNKEY_M3G_CLASS_MORPHING_MESH = 0x10,
    FUNKEY_M3G_CLASS_POLYGON_MODE = 0x11,
    FUNKEY_M3G_CLASS_RENDER_CONTEXT = 0x12,
    FUNKEY_M3G_CLASS_SKINNED_MESH = 0x13,
    FUNKEY_M3G_CLASS_SPRITE_3D = 0x14,
    FUNKEY_M3G_CLASS_TEXTURE_2D = 0x15,
    FUNKEY_M3G_CLASS_VERTEX_ARRAY = 0x16,
    FUNKEY_M3G_CLASS_VERTEX_BUFFER = 0x17,
    FUNKEY_M3G_CLASS_WORLD = 0x18
};

long funkey_m3g_create_interface(void);
void funkey_m3g_finalize_interface(long handle);
long funkey_m3g_create_object(int class_id);
long funkey_m3g_create_loader(long interface_handle);
void funkey_m3g_add_ref(long handle);
void funkey_m3g_release(long handle);
int funkey_m3g_get_class_id(long handle);
void funkey_m3g_set_user_id(long handle, int user_id);
int funkey_m3g_get_user_id(long handle);
long funkey_m3g_find(long handle, int user_id);
long funkey_m3g_duplicate(long handle, long *pairs, int max_pairs,
                          int *pair_count);
int funkey_m3g_object_get_references(long handle, long *refs, int count);
int funkey_m3g_loader_decode(long loader, int bytes, const unsigned char *data);
int funkey_m3g_loader_get_loaded_objects(long loader, long *objects, int count);
void funkey_m3g_loader_set_external_refs(long loader, const long *refs, int count);
int funkey_m3g_loader_get_objects_with_user_params(long loader, long *objects,
                                                   int count);
int funkey_m3g_loader_get_num_user_params(long loader, int object_index);
int funkey_m3g_loader_get_user_param(long loader, int object_index, int index,
                                     signed char *data, int count);

void funkey_m3g_group_add_child(long group, long child);
void funkey_m3g_group_remove_child(long group, long child);
int funkey_m3g_group_get_child_count(long group);
long funkey_m3g_group_get_child(long group, int index);
long funkey_m3g_group_pick3d(long group, int mask, float *ray, float *result);
long funkey_m3g_group_pick2d(long group, int mask, float x, float y,
                             long camera, float *result);

long funkey_m3g_node_get_parent(long handle);
int funkey_m3g_node_get_transform_to(long handle, long target, float *matrix);
void funkey_m3g_node_align(long handle, long reference);
void funkey_m3g_node_set_alignment(long handle, long z_ref, int z_target,
                                   long y_ref, int y_target);
void funkey_m3g_node_set_parent(long handle, long parent);
void funkey_m3g_node_set_scope(long handle, int scope);
int funkey_m3g_node_get_scope(long handle);
void funkey_m3g_node_set_alpha(long handle, float alpha);
float funkey_m3g_node_get_alpha(long handle);
void funkey_m3g_node_enable(long handle, int which, int enable);
int funkey_m3g_node_is_enabled(long handle, int which);
long funkey_m3g_node_get_z_ref(long handle);
long funkey_m3g_node_get_y_ref(long handle);
int funkey_m3g_node_get_subtree_size(long handle);
int funkey_m3g_node_get_alignment_target(long handle, int axis);

void funkey_m3g_transform_set_orientation(long handle, float angle,
                                          float ax, float ay, float az,
                                          int absolute);
void funkey_m3g_transform_pre_rotate(long handle, float angle,
                                     float ax, float ay, float az);
void funkey_m3g_transform_get_orientation(long handle, float *values,
                                          int count);
void funkey_m3g_transform_set_scale(long handle, float sx, float sy, float sz,
                                    int absolute);
void funkey_m3g_transform_get_scale(long handle, float *values, int count);
void funkey_m3g_transform_set_translation(long handle,
                                          float tx, float ty, float tz,
                                          int absolute);
void funkey_m3g_transform_get_translation(long handle, float *values,
                                          int count);
void funkey_m3g_transform_set_matrix(long handle, const float *matrix);
void funkey_m3g_transform_get_matrix(long handle, float *matrix);
void funkey_m3g_transform_get_composite(long handle, float *matrix);

void funkey_m3g_anim_set_active_interval(long handle, int start, int end);
int funkey_m3g_anim_get_active_interval(long handle, int which);
void funkey_m3g_anim_set_position(long handle, float position, int world_time);
float funkey_m3g_anim_get_position(long handle, int world_time);
int funkey_m3g_anim_get_ref_world_time(long handle);
void funkey_m3g_anim_set_speed(long handle, float speed, int world_time);
float funkey_m3g_anim_get_speed(long handle);
void funkey_m3g_anim_set_weight(long handle, float weight);
float funkey_m3g_anim_get_weight(long handle);
int funkey_m3g_object_add_animation_track(long object, long track);
void funkey_m3g_object_remove_animation_track(long object, long track);
int funkey_m3g_object_get_animation_track_count(long object);
long funkey_m3g_object_get_animation_track(long object, int index);
int funkey_m3g_object_animate(long object, int world_time);
void funkey_m3g_animation_track_init(long track, long sequence, int property);
void funkey_m3g_animation_track_set_controller(long track, long controller);
long funkey_m3g_animation_track_get_controller(long track);
long funkey_m3g_animation_track_get_sequence(long track);
int funkey_m3g_animation_track_get_property(long track);
int funkey_m3g_keyframe_init(long sequence, int keyframes, int components,
                             int interpolation);
void funkey_m3g_keyframe_set(long sequence, int index, int time,
                             const float *values, int count);
int funkey_m3g_keyframe_get(long sequence, int index, float *values,
                            int count);
void funkey_m3g_keyframe_set_duration(long sequence, int duration);
void funkey_m3g_keyframe_set_repeat(long sequence, int repeat);
void funkey_m3g_keyframe_set_valid_range(long sequence, int first, int last);
int funkey_m3g_keyframe_get_int(long sequence, int which);

void funkey_m3g_world_set_active_camera(long world, long camera);
long funkey_m3g_world_get_active_camera(long world);
void funkey_m3g_world_set_background(long world, long background);
long funkey_m3g_world_get_background(long world);

void funkey_m3g_camera_set_projection(long camera, int mode,
                                      float a, float b, float c, float d);
void funkey_m3g_camera_set_generic(long camera, const float *matrix);
int funkey_m3g_camera_get_projection(long camera, float *params);
int funkey_m3g_camera_get_projection_matrix(long camera, float *matrix);

void funkey_m3g_background_set_color(long background, unsigned int argb);
unsigned int funkey_m3g_background_get_color(long background);
void funkey_m3g_background_set_image(long background, long image);
long funkey_m3g_background_get_image(long background);
void funkey_m3g_background_set_image_mode(long background, int mode_x,
                                          int mode_y);
int funkey_m3g_background_get_image_mode(long background, int which);
void funkey_m3g_background_enable(long background, int which, int enable);
int funkey_m3g_background_is_enabled(long background, int which);
void funkey_m3g_background_set_crop(long background, int x, int y, int w, int h);
int funkey_m3g_background_get_crop(long background, int which);

int funkey_m3g_vertex_array_init(long handle, int vertices,
                                 int components, int component_size);
void funkey_m3g_vertex_array_set(long handle, int first, int count,
                                 const int *values, int value_count);
void funkey_m3g_vertex_array_get(long handle, int first, int count,
                                 int *values, int value_count);
int funkey_m3g_vertex_array_get_component_count(long handle);
int funkey_m3g_vertex_array_get_component_type(long handle);
int funkey_m3g_vertex_array_get_vertex_count(long handle);
void funkey_m3g_vertex_array_transform(long handle, const float *matrix,
                                       float *out, int out_count, int use_w);

void funkey_m3g_vertex_buffer_set_array(long buffer, int which, long array,
                                        float scale, const float *bias,
                                        int bias_count);
long funkey_m3g_vertex_buffer_get_array(long buffer, int which,
                                        float *scale_bias, int scale_bias_count);
void funkey_m3g_vertex_buffer_set_default_color(long buffer, unsigned int argb);
unsigned int funkey_m3g_vertex_buffer_get_default_color(long buffer);
int funkey_m3g_vertex_buffer_get_vertex_count(long buffer);

int funkey_m3g_index_buffer_init(long handle, const int *indices, int count);
int funkey_m3g_index_buffer_init_strips(long handle, int first_index,
                                        const int *indices, int index_count,
                                        const int *lengths, int length_count);
int funkey_m3g_index_buffer_init_implicit(long handle, int first,
                                          const int *lengths,
                                          int length_count);
int funkey_m3g_index_buffer_get_count(long handle);
void funkey_m3g_index_buffer_get_indices(long handle, int *indices, int count);

int funkey_m3g_mesh_init(long mesh, long vertices, const long *triangles,
                         const long *appearances, int submesh_count);
void funkey_m3g_mesh_set_appearance(long mesh, int index, long appearance);
long funkey_m3g_mesh_get_appearance(long mesh, int index);
long funkey_m3g_mesh_get_index_buffer(long mesh, int index);
long funkey_m3g_mesh_get_vertex_buffer(long mesh);
int funkey_m3g_mesh_get_submesh_count(long mesh);
int funkey_m3g_morphing_mesh_init(long mesh, long vertices,
                                  const long *targets, int target_count,
                                  const long *triangles,
                                  const long *appearances,
                                  int submesh_count);
void funkey_m3g_morphing_mesh_set_weights(long mesh, const float *weights,
                                          int count);
void funkey_m3g_morphing_mesh_get_weights(long mesh, float *weights,
                                          int count);
long funkey_m3g_morphing_mesh_get_target(long mesh, int index);
int funkey_m3g_morphing_mesh_get_target_count(long mesh);
int funkey_m3g_skinned_mesh_init(long mesh, long vertices,
                                 const long *triangles,
                                 const long *appearances,
                                 int submesh_count, long skeleton);
void funkey_m3g_skinned_mesh_add_transform(long mesh, long bone, int weight,
                                           int first_vertex, int num_vertices);
long funkey_m3g_skinned_mesh_get_skeleton(long mesh);
void funkey_m3g_skinned_mesh_get_bone_transform(long mesh, long bone,
                                                float *matrix);
int funkey_m3g_skinned_mesh_get_bone_vertices(long mesh, long bone,
                                              int *indices, float *weights);
int funkey_m3g_sprite_init(long sprite, int scaled, long image,
                           long appearance);
int funkey_m3g_sprite_is_scaled(long sprite);
void funkey_m3g_sprite_set_appearance(long sprite, long appearance);
long funkey_m3g_sprite_get_appearance(long sprite);
void funkey_m3g_sprite_set_image(long sprite, long image);
long funkey_m3g_sprite_get_image(long sprite);
void funkey_m3g_sprite_set_crop(long sprite, int x, int y, int w, int h);
int funkey_m3g_sprite_get_crop(long sprite, int which);

int funkey_m3g_image_init(long image, int format, int width, int height,
                          const unsigned char *pixels, int pixel_count,
                          int mutable_image);
int funkey_m3g_image_init_palette(long image, int format, int width, int height,
                                  const unsigned char *pixels, int pixel_count,
                                  const unsigned char *palette,
                                  int palette_count);
void funkey_m3g_image_set(long image, int x, int y, int width, int height,
                          const unsigned char *pixels, int pixel_count);
int funkey_m3g_image_get_format(long image);
int funkey_m3g_image_get_width(long image);
int funkey_m3g_image_get_height(long image);
int funkey_m3g_image_is_mutable(long image);

void funkey_m3g_texture_set_image(long texture, long image);
long funkey_m3g_texture_get_image(long texture);
void funkey_m3g_texture_set_filtering(long texture, int level, int image);
void funkey_m3g_texture_set_wrapping(long texture, int s, int t);
int funkey_m3g_texture_get_wrapping_s(long texture);
int funkey_m3g_texture_get_wrapping_t(long texture);
void funkey_m3g_texture_set_blending(long texture, int func);
int funkey_m3g_texture_get_blending(long texture);
void funkey_m3g_texture_set_blend_color(long texture, unsigned int rgb);
unsigned int funkey_m3g_texture_get_blend_color(long texture);
int funkey_m3g_texture_get_image_filter(long texture);
int funkey_m3g_texture_get_level_filter(long texture);

void funkey_m3g_appearance_set(long appearance, int slot, long value);
long funkey_m3g_appearance_get(long appearance, int slot);
void funkey_m3g_appearance_set_texture(long appearance, int unit, long texture);
long funkey_m3g_appearance_get_texture(long appearance, int unit);
void funkey_m3g_appearance_set_layer(long appearance, int layer);
int funkey_m3g_appearance_get_layer(long appearance);

void funkey_m3g_material_set_color(long material, int target,
                                   unsigned int argb);
unsigned int funkey_m3g_material_get_color(long material, int target);
void funkey_m3g_material_set_shininess(long material, float shininess);
float funkey_m3g_material_get_shininess(long material);
void funkey_m3g_material_set_vertex_color_tracking(long material, int enable);
int funkey_m3g_material_get_vertex_color_tracking(long material);

void funkey_m3g_compositing_set_blending(long mode, int blending);
int funkey_m3g_compositing_get_blending(long mode);
void funkey_m3g_compositing_set_alpha_threshold(long mode, float threshold);
float funkey_m3g_compositing_get_alpha_threshold(long mode);
void funkey_m3g_compositing_set_enable(long mode, int slot, int enable);
int funkey_m3g_compositing_get_enable(long mode, int slot);
void funkey_m3g_compositing_set_depth_offset(long mode, float factor,
                                             float units);
float funkey_m3g_compositing_get_depth_offset(long mode, int slot);

void funkey_m3g_polygon_set_mode(long polygon, int slot, int mode);
int funkey_m3g_polygon_get_mode(long polygon, int slot);
void funkey_m3g_polygon_set_enable(long polygon, int slot, int enable);
int funkey_m3g_polygon_get_enable(long polygon, int slot);

void funkey_m3g_fog_set_mode(long fog, int mode);
int funkey_m3g_fog_get_mode(long fog);
void funkey_m3g_fog_set_linear(long fog, float near_distance,
                               float far_distance);
float funkey_m3g_fog_get_distance(long fog, int which);
void funkey_m3g_fog_set_density(long fog, float density);
float funkey_m3g_fog_get_density(long fog);
void funkey_m3g_fog_set_color(long fog, unsigned int rgb);
unsigned int funkey_m3g_fog_get_color(long fog);

void funkey_m3g_light_set_intensity(long light, float intensity);
float funkey_m3g_light_get_intensity(long light);
void funkey_m3g_light_set_color(long light, unsigned int rgb);
unsigned int funkey_m3g_light_get_color(long light);
void funkey_m3g_light_set_mode(long light, int mode);
int funkey_m3g_light_get_mode(long light);
void funkey_m3g_light_set_spot(long light, int slot, float value);
float funkey_m3g_light_get_spot(long light, int slot);
void funkey_m3g_light_set_attenuation(long light, float constant,
                                      float linear, float quadratic);
float funkey_m3g_light_get_attenuation(long light, int slot);

void funkey_m3g_surface_init(FunKeyM3GSurface *surface,
                             int width, int height,
                             int clip_x, int clip_y,
                             int clip_w, int clip_h);
void funkey_m3g_surface_bind_pixels(FunKeyM3GSurface *surface,
                                    unsigned short *pixels, int stride);
void funkey_m3g_surface_clear(FunKeyM3GSurface *surface, unsigned int argb);
void funkey_m3g_surface_render_mesh(FunKeyM3GSurface *surface, long mesh);
void funkey_m3g_surface_render_node(FunKeyM3GSurface *surface, long node);
void funkey_m3g_surface_render_world(FunKeyM3GSurface *surface, long world);
void funkey_m3g_context_bind_surface(long context, FunKeyM3GSurface *surface);
void funkey_m3g_context_release_target(long context);
void funkey_m3g_context_clear(long context, long background);
void funkey_m3g_context_render(long context, long vertices, long indices,
                               long appearance, const float *transform,
                               int scope);
void funkey_m3g_context_render_node(long context, long node,
                                    const float *transform);
void funkey_m3g_context_render_world(long context, long world);
int funkey_m3g_context_add_light(long context, long light,
                                 const float *transform);
void funkey_m3g_context_set_camera(long context, long camera,
                                   const float *transform);
void funkey_m3g_context_set_viewport(long context, int x, int y,
                                     int width, int height);
void funkey_m3g_context_set_light(long context, int index, long light,
                                  const float *transform);
void funkey_m3g_context_set_depth_range(long context, float near_value,
                                        float far_value);
void funkey_m3g_context_get_view_transform(long context, float *matrix);
long funkey_m3g_context_get_camera(long context);
long funkey_m3g_context_get_light_transform(long context, int index,
                                            float *matrix);
int funkey_m3g_context_get_light_count(long context);
void funkey_m3g_context_get_depth_range(long context, float *near_value,
                                        float *far_value);
void funkey_m3g_context_get_viewport(long context, int *x, int *y,
                                     int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
