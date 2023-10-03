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

	int prerender() override;
	int postrender() override;

	void update_ambient_uniforms();
	void setup_light_position_uniform(const vec3& light_position);
	void setup_diffuse_color_uniform();
	void setup_roughness_uniform(const float roughness);

    void load_mesh_textures_thread_function(const Materials& materials);

	void compute_bounding_boxes_of_groups(std::vector<TriangleGroup>& groups);
    bool rejection_test_bbox_frustum_culling(const BoundingBox& bbox, const Transform& mvpMatrix);
    bool rejection_test_bbox_frustum_culling_scene(const BoundingBox& bbox, const Transform& inverse_mvp_matrix);

	// creation des objets de l'application
	int init();

	// destruction des objets de l'application
	int quit();

	void update_recomputed_irradiance_map();

	void draw_shadow_map();
	void draw_skysphere();

	void draw_general_settings();
    void draw_lighting_window();
	void draw_imgui();

	// dessiner une nouvelle image
    int render();

    inline static const Vector LIGHT_POSITION = Vector(2, 0, -10);
    inline static const std::string IRRADIANCE_MAPS_CACHE_FOLDER = "TPs/from_scratch/data/irradiance_maps_cache";
	inline static const int SKYBOX_UNIT = 0;
	inline static const int SKYSPHERE_UNIT = 1;
	inline static const int DIFFUSE_IRRADIANCE_MAP_UNIT = 2;
	inline static const int TRIANGLE_GROUP_TEXTURE_UNIT = 3;
	inline static const int SHADOW_MAP_UNIT = 4;

	inline static const int SHADOW_MAP_RESOLUTION = 2048;

protected:
	ApplicationTimer m_app_timer;





	//Mesh m_repere;
	Mesh m_mesh;
	std::vector<BoundingBox> m_mesh_groups_bounding_boxes;
    std::vector<TriangleGroup> m_mesh_triangles_group;
	std::vector<GLuint> m_mesh_textures;

	GLuint m_cubemap_vao;
    GLuint m_mesh_vao;
    GLuint m_diffuse_texture_shader;

	GLuint m_cubemap_shader;
	GLuint m_cubemap;
	GLuint m_skysphere;
	GLuint m_irradiance_map;

	GLuint m_wholescreen_texture_shader;//Debug shadow map shader
	GLuint m_shadow_map_program;
	GLuint m_shadow_map_framebuffer;
	GLuint m_shadow_map;
	Transform m_mlp_light_transform;

	//This contains the data of an irradiance map that has just been recomputed
	//We need to use this data to update the OpenGl texture used by the shader
	ImageData m_recomputed_irradiance_map_data;

	ImGuiIO m_imgui_io;

	//The settings of the application. Those are parameters that can be modified using the
	//ImGui UI
	ApplicationSettings m_application_settings;

	//This structure contains variables relative to the current state of the application
	//such as whether or not we're currently recomputing an irradiance map
	ApplicationState m_application_state;
};
