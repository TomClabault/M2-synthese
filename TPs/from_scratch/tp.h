#include "app_camera.h"
#include "application_settings.h"
#include "application_timer.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "mesh.h"

class TP : public AppCamera
{
public:
	// constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
	TP();

	int get_window_width();
	int get_window_height();

	int prerender() override;
	int postrender() override;

	void update_ambient_uniforms();
	void setup_light_position_uniform(const vec3& light_position);
	void setup_diffuse_color_uniform();

	// creation des objets de l'application
	int init();

	// destruction des objets de l'application
	int quit();

	void draw_general_settings();
	void draw_lighting_window();
	void draw_materials_window();
	void draw_imgui();

	// dessiner une nouvelle image
	int render();

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

	ImGuiIO m_imgui_io;

	ApplicationSettings m_application_settings;
};