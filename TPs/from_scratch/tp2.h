#ifndef TP2_H
#define TP2_H

#include "app_camera.h"
#include "application_settings.h"
#include "application_state.h"
#include "application_timer.h"
#include "image_io.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "mesh.h"

#include <string>

struct BoundingBox
{
    Point pMin;
    Point pMax;
};

class TP2 : public AppCamera
{
public:
    struct alignas(16) CullObject
    {
        vec3 min;
        unsigned int vertex_base;
        vec3 max;
        unsigned int vertex_count;
    };

    struct alignas(4) MultiDrawIndirectParam
    {
        unsigned int vertex_count;
        unsigned int instance_count;
        unsigned int vertex_base;
        unsigned int instance_base;
    };

    struct CookTorranceMaterial
    {
        vec3 base_color; // Used only if the object doesn't have a base color texture
        float metalness; // Used only if the object doesn't have a specular texture
        vec3 normal = vec3(-1.0f, -1.0f, -1.0f); // Used only if the object doesn't have a normal texture
        float roughness; // Used only if the object doesn't have a specular texture

        int base_color_texture_id;
        int specular_texture_id;
        int normal_map_texture_id;
    };

    TP2();

	int get_window_width();
	int get_window_height();

    int mesh_groups_count();
    int mesh_groups_drawn();

	int prerender() override;
	int postrender() override;

    GLuint create_opengl_texture(std::string& filepath, int GL_tex_format, float anisotropy = 0.0f);
    std::vector<TP2::CookTorranceMaterial> load_and_create_textures();

	void compute_bounding_boxes_of_groups(std::vector<TriangleGroup>& groups);
    bool rejection_test_bbox_frustum_culling(const CullObject& object, const Transform& mvpMatrix);
    bool rejection_test_bbox_frustum_culling_scene(const CullObject& object, const Transform& inverse_mvp_matrix);

    /**
     * @param object Object to try to cull
     * @param z_buffer_mipmaps The mipmaps of the z-buffer that will be
     * used for the hierarchical occlusion test
     * @return True if the object has been culled and is not visible.
     * False if it is visible
     */
    bool occlusion_cull_cpu(const Transform &mvpv_matrix, CullObject& object, int depth_buffer_width, int depth_buffer_height, const std::vector<std::vector<float>>& z_buffer_mipmaps, const std::vector<std::pair<int, int>>& mipmaps_widths_heights);
    void occlusion_cull_gpu(const Transform& mvp_matrix, const Transform& view_matrix, const Transform& viewport_matrix, GLuint object_ids_to_cull_buffer, int number_of_objects_to_cull);

    std::vector<int> create_material_ids_data();

	// creation des objets de l'application
	int init();

	// destruction des objets de l'application
	int quit();

	void update_recomputed_irradiance_map();

    int resize_hdr_frame();
    void resize_z_buffer_mipmaps();
    int create_hdr_frame();
    void create_z_buffer_mipmaps_textures(int width, int height);
    int create_shadow_map();

	void draw_shadow_map();
    void draw_fullscreen_quad_texture(GLuint texture_to_draw);
    void draw_fullscreen_quad_texture_hdr_exposure(GLuint texture_to_draw);
	void draw_skysphere();

    std::vector<TP2::MultiDrawIndirectParam> generate_draw_params_from_object_ids(std::vector<int> object_ids);

    void draw_by_groups_cpu_frustum_culling(const Transform &vp_matrix, const Transform &mvp_matrix_inverse);
    void draw_multi_draw_indirect_from_ids(const std::vector<int>& object_ids);
    void draw_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse);
    void draw_mdi_occlusion_culling(const Transform &mvp_matrix, const Transform &mvp_matrix_inverse);
    int gpu_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse);
    void cpu_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse);
    void cpu_mdi_selective_frustum_culling(const std::vector<int>& objects_id, const Transform& mvp_matrix, const Transform& mvp_matrix_inverse);

	void draw_general_settings();
    void draw_lighting_window();
	void draw_material_window();
	void draw_imgui();

	// dessiner une nouvelle image
    int render();

    inline static const std::string IRRADIANCE_MAPS_CACHE_FOLDER = "TPs/from_scratch/data/irradiance_maps_cache";

    inline static const int FULLSCREEN_QUAD_TEXTURE_UNIT = 0;
	inline static const int SKYBOX_UNIT = 0;
	inline static const int SKYSPHERE_UNIT = 1;
	inline static const int DIFFUSE_IRRADIANCE_MAP_UNIT = 2;
    inline static const int BASE_COLOR_TEXTURE_ARRAY_UNIT = 3;
    inline static const int SPECULAR_TEXTURE_ARRAY_UNIT = 4;
    inline static const int NORMAL_MAP_TEXTURE_ARRAY_UNIT = 5;
    inline static const int SHADOW_MAP_UNIT = 6;

    inline static const Transform LIGHT_CAMERA_ORTHO_PROJ_BISTRO = Ortho(-60, 90, -80, 110, 50, 190);
    inline static const int SHADOW_MAP_RESOLUTION = 16384;

protected:
	ApplicationTimer m_app_timer;

    int m_frame_number = 0;



	//Mesh m_repere;
	Mesh m_mesh;
    int m_mesh_groups_drawn;

    //Variables used for mesh rendering
    std::vector<TriangleGroup> m_mesh_triangles_group;
    GLuint m_diffuse_texture_array;
    GLuint m_specular_texture_array;
    GLuint m_normal_map_texture_array;
    GLuint m_materials_buffer;
    GLuint m_default_texture;
	GLuint m_cubemap_vao;
    GLuint m_mesh_vao;
    GLuint m_texture_shadow_cook_torrance_shader;
	GLuint m_cubemap_shader;
	GLuint m_cubemap;
	GLuint m_skysphere;
	GLuint m_irradiance_map;
    GLuint m_hdr_shader_output_texture;
    GLuint m_hdr_depth_buffer_texture;
    GLuint m_hdr_framebuffer;
    GLuint m_fullscreen_quad_texture_shader;
    GLuint m_fullscreen_quad_texture_hdr_exposure_shader;
	GLuint m_shadow_map_program;
	GLuint m_shadow_map_framebuffer;
    GLuint m_shadow_map = 0;

    //Variables used for the culling (frustum and occlusion)
    GLuint m_z_buffer_mipmaps_texture;
    int m_z_buffer_mipmaps_count;
    std::vector<int> m_objects_drawn_last_frame;
    std::vector<CullObject> m_cull_objects;
    GLuint m_occlusion_culling_shader;
    GLuint m_frustum_culling_shader;
    GLuint m_mdi_draw_params_buffer;
    GLuint m_culling_objects_id_to_draw;
    GLuint m_culling_passing_ids;
    GLuint m_culling_input_object_buffer;
    GLuint m_culling_nb_objects_passed_buffer;







    Orbiter m_light_camera;
    Vector m_light_pos = Vector(1.0f, 0.0, -1.0f);
	Transform m_lp_light_transform;

    Transform m_debug_current_projection;
    float m_debug_min_x = -90, m_debug_max_x = 110, m_debug_min_y = -35, m_debug_max_y = 50, m_debug_min_z = 0.1, m_debug_max_z = 160;

	//This contains the data of an irradiance map that has just been recomputed
	//We need to use this data to update the OpenGl texture used by the shader
    Image m_recomputed_irradiance_map_data;

	ImGuiIO m_imgui_io;

	//The settings of the application. Those are parameters that can be modified using the
	//ImGui UI
	ApplicationSettings m_application_settings;

	//This structure contains variables relative to the current state of the application
	//such as whether or not we're currently recomputing an irradiance map
	ApplicationState m_application_state;
};

#endif
