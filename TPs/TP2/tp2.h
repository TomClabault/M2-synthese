#include "app_camera.h"
#include "application_settings.h"
#include "application_state.h"
#include "application_timer.h"
#include "image_io.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "mesh.h"

#include <string>

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

	// creation des objets de l'application
	int init();

	// destruction des objets de l'application
	int quit();

	void update_recomputed_irradiance_map();

	void draw_general_settings();
	void draw_lighting_window();
	void draw_materials_window();
	void draw_imgui();

	// dessiner une nouvelle image
        int render();

        inline static const Vector LIGHT_POSITION = Vector(2, 0, -10);

protected:
	ApplicationTimer m_app_timer;








	//Mesh m_repere;
	Mesh m_mesh;

	GLuint m_cubemap_vao;
	GLuint m_robot_vao;
	GLuint m_custom_shader;

	GLuint m_cubemap_shader;
	GLuint m_cubemap;
	GLuint m_skysphere;
	GLuint m_irradiance_map;

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
