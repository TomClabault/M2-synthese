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
    TP2();

	int get_window_width();
	int get_window_height();

    int mesh_groups_count();
    int mesh_groups_drawn();

	int prerender() override;
	int postrender() override;

	void update_ambient_uniforms();
	void setup_diffuse_color_uniform();
	void setup_roughness_uniform(const float roughness);

    GLuint create_opengl_texture(std::string& filepath);
    void load_mesh_textures_thread_function(const Materials& materials);

	void compute_bounding_boxes_of_groups(std::vector<TriangleGroup>& groups);
    bool rejection_test_bbox_frustum_culling(const BoundingBox& bbox, const Transform& mvpMatrix);
    bool rejection_test_bbox_frustum_culling_scene(const BoundingBox& bbox, const Transform& inverse_mvp_matrix);

	// creation des objets de l'application
	int init();

	// destruction des objets de l'application
	int quit();

	void update_recomputed_irradiance_map();

    int resize_hdr_frame();
    int create_hdr_frame();
    int create_shadow_map();
	void draw_shadow_map();
    void draw_light_camera_frustum();
    void draw_fullscreen_quad_texture(GLuint texture_to_draw);
    void draw_fullscreen_quad_texture_hdr_exposure(GLuint texture_to_draw);
	void draw_skysphere();

	void draw_general_settings();
    void draw_lighting_window();
	void draw_imgui();

	// dessiner une nouvelle image
    int render();

    inline static const std::string IRRADIANCE_MAPS_CACHE_FOLDER = "TPs/from_scratch/data/irradiance_maps_cache";

    inline static const int FULLSCREEN_QUAD_TEXTURE_UNIT = 0;
	inline static const int SKYBOX_UNIT = 0;
	inline static const int SKYSPHERE_UNIT = 1;
	inline static const int DIFFUSE_IRRADIANCE_MAP_UNIT = 2;
    inline static const int TRIANGLE_GROUP_BASE_COLOR_TEXTURE_UNIT = 3;
    inline static const int TRIANGLE_GROUP_SPECULAR_TEXTURE_UNIT = 4;
    inline static const int SHADOW_MAP_UNIT = 5;

    inline static const Transform LIGHT_CAMERA_ORTHO_PROJ_BISTRO = Ortho(-60, 90, -80, 110, 50, 190);
    inline static const int SHADOW_MAP_RESOLUTION = 16384;

protected:
	ApplicationTimer m_app_timer;





	//Mesh m_repere;
	Mesh m_mesh;
    int m_mesh_groups_drawn;
	std::vector<BoundingBox> m_mesh_groups_bounding_boxes;
    std::vector<TriangleGroup> m_mesh_triangles_group;
    std::vector<GLuint> m_mesh_base_color_textures;
    std::vector<GLuint> m_mesh_specular_textures;
    GLuint m_default_texture;

	GLuint m_cubemap_vao;
    GLuint m_mesh_vao;
    GLuint m_texture_shadow_cook_torrance_shader;

	GLuint m_cubemap_shader;
	GLuint m_cubemap;
	GLuint m_skysphere;
	GLuint m_irradiance_map;

    GLuint m_hdr_shader_output_texture;
    GLuint m_hdr_framebuffer;

    GLuint m_fullscreen_quad_texture_shader;
    GLuint m_fullscreen_quad_texture_hdr_exposure_shader;
	GLuint m_shadow_map_program;
	GLuint m_shadow_map_framebuffer;
    GLuint m_shadow_map = 0;

    Orbiter m_light_camera;
	Transform m_mlp_light_transform;

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
