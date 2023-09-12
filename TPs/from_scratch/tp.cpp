
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.

#include "wavefront.h"
#include "texture.h"

#include "app_camera.h"        // classe Application a deriver
#include "draw.h"        
#include "orbiter.h"
#include "text.h"
#include "uniforms.h"

#include "application_settings.h"
#include "application_timer.h"
#include "tp.h"
#include "utils.h"

#include <thread>

const vec3 LIGHT_POSITION(2, 0, 11);

// utilitaire. creation d'une grille / repere.
Mesh make_grid(const int n = 10)
{
	Mesh grid = Mesh(GL_LINES);

	// grille
	grid.color(White());
	for (int x = 0; x < n; x++)
	{
		float px = float(x) - float(n) / 2 + .5f;
		grid.vertex(Point(px, 0, -float(n) / 2 + .5f));
		grid.vertex(Point(px, 0, float(n) / 2 - .5f));
	}

	for (int z = 0; z < n; z++)
	{
		float pz = float(z) - float(n) / 2 + .5f;
		grid.vertex(Point(-float(n) / 2 + .5f, 0, pz));
		grid.vertex(Point(float(n) / 2 - .5f, 0, pz));
	}

	// axes XYZ
	grid.color(Red());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(1, .1, 0));

	grid.color(Green());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(0, 1, 0));

	grid.color(Blue());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(0, .1, 1));

	glLineWidth(2);

	return grid;
}

// constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
TP::TP() : AppCamera(1280, 720, 3, 3, 16) 
{
	m_app_timer = ApplicationTimer(this);
}

int TP::get_window_width() { return window_width(); }
int TP::get_window_height() { return window_height(); }

int TP::prerender()
{
	//Timing of the application
	m_app_timer.prerender();





	// recupere les mouvements de la souris
	int mx, my;
	unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	// deplace la camera
	auto& imgui_io = ImGui::GetIO();

	if (!imgui_io.WantCaptureMouse) {
		if (mb & SDL_BUTTON(1))
			m_camera.rotation(mx, my);      // tourne autour de l'objet
		else if (mb & SDL_BUTTON(3))
			m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height()); // deplace le point de rotation
		else if (mb & SDL_BUTTON(2))
			m_camera.move(mx);           // approche / eloigne l'objet

		SDL_MouseWheelEvent wheel = wheel_event();
		if (wheel.y != 0)
		{
			clear_wheel_event();
			m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
		}

	}

	const char* orbiter_filename = "app_orbiter.txt";
	// copy / export / write orbiter
	if (!imgui_io.WantCaptureKeyboard)
	{
		if (key_state('c'))
		{
			clear_key_state('c');
			m_camera.write_orbiter(orbiter_filename);

		}
		// paste / read orbiter
		if (key_state('v'))
		{
			clear_key_state('v');

			Orbiter tmp;
			if (tmp.read_orbiter(orbiter_filename) < 0)
				// ne pas modifer la camera en cas d'erreur de lecture...
				tmp = m_camera;

			m_camera = tmp;
		}

		// screenshot
		if (key_state('f12'))
		{
			static int calls = 1;
			clear_key_state('s');
			screenshot("app", calls++);
		}
	}

	// appelle la fonction update() de la classe derivee
	return update(global_time(), delta_time());
}

int TP::postrender() 
{
	m_app_timer.postrender();
		
	return 0;
}

void TP::update_ambient_uniforms()
{
	glUseProgram(m_custom_shader);

	GLuint use_ambient_location = glGetUniformLocation(m_custom_shader, "u_use_ambient");
	glUniform1i(use_ambient_location, m_application_settings.use_ambient);

	GLuint ambient_color_location = glGetUniformLocation(m_custom_shader, "u_ambient_color");
	glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);
}

void TP::setup_light_position_uniform(const vec3& light_position)
{
	glUseProgram(m_custom_shader);

	GLint light_position_location = glGetUniformLocation(m_custom_shader, "u_light_position");
	glUniform3f(light_position_location, light_position.x, light_position.y, light_position.z);
}

void TP::setup_diffuse_color_uniform()
{
	glUseProgram(m_custom_shader);

	GLuint diffuse_colors_location = glGetUniformLocation(m_custom_shader, "u_diffuse_colors");
	std::vector<Color> diffuse_colors_buffer;
	for (const Material& mat : m_mesh.materials().materials)
		diffuse_colors_buffer.push_back(mat.diffuse);
	glUniform4fv(diffuse_colors_location, m_mesh.materials().materials.size(), (GLfloat*)diffuse_colors_buffer.data());

}

// creation des objets de l'application
int TP::init()
{
	//Setting ImGUI up
	ImGui::CreateContext();

	m_imgui_io = ImGui::GetIO();
	ImGui_ImplSdlGL3_Init(m_window);

	//Positioning the camera to a default state
	m_camera.read_orbiter("TPs/from_scratch/start_camera_minus_z.txt");


	//Creating a basic grid
	//m_repere = make_grid(10);

	//Reading the mesh displayed
	m_mesh = read_mesh("data/robot.obj");

	// etat openGL par defaut
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
	glClearDepth(1.f);                          // profondeur par defaut

	glDepthFunc(GL_LEQUAL);                       // ztest, conserver l'intersection la plus proche de la camera
	glEnable(GL_DEPTH_TEST);                    // activer le ztest

	//Lecture du shader
	m_custom_shader = read_program("TPs/from_scratch/shaders/shader_custom.glsl");
	program_print_errors(m_custom_shader);
	m_cubemap_shader = read_program("TPs/from_scratch/shaders/shader_cubemap.glsl");
	program_print_errors(m_cubemap_shader);

	GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_skysphere);
	//The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
	glUniform1i(skysphere_uniform_location, 1);
		
	setup_light_position_uniform(vec3(2, 0, 10));
	setup_diffuse_color_uniform();

	GLint use_ambient_location = glGetUniformLocation(m_custom_shader, "u_use_ambient");
	glUniform1i(use_ambient_location, m_application_settings.use_ambient);

	GLint ambient_color_location = glGetUniformLocation(m_custom_shader, "u_ambient_color");
	glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);





	//Creating an empty VAO that will be used for the cubemap
	glGenVertexArrays(1, &m_cubemap_vao);





	//Creating the VAO for the mesh that will be displayed
	glGenVertexArrays(1, &m_robot_vao);
	//Selecting the VAO that we're going to configure
	glBindVertexArray(m_robot_vao);

	//Creation du position buffer
	GLuint position_buffer;
	glGenBuffers(1, &position_buffer);
	//On selectionne le position buffer
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	//On remplit le buffer selectionne (le position buffer)
	glBufferData(GL_ARRAY_BUFFER, m_mesh.vertex_buffer_size(), m_mesh.vertex_buffer(), GL_STATIC_DRAW);

	//On recupere l'id de l'attribut position du vertex shader "in vec3 position"
	GLint position_attribute = glGetAttribLocation(m_custom_shader, "position");
	glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(position_attribute);





	//Creation du normal buffer
	GLuint normal_buffer;
	glGenBuffers(1, &normal_buffer);
	//On selectionne le normal buffer
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	//On remplit le buffer selectionne (le normal buffer)
	glBufferData(GL_ARRAY_BUFFER, m_mesh.normal_buffer_size(), m_mesh.normal_buffer(), GL_STATIC_DRAW);

	//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
	GLint normal_attribute = glGetAttribLocation(m_custom_shader, "normal");
	glVertexAttribPointer(normal_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normal_attribute);





	std::vector<unsigned int> material_index_buffer;
	for (unsigned int index : m_mesh.material_indices())
	{
		material_index_buffer.push_back(index);
		material_index_buffer.push_back(index);
		material_index_buffer.push_back(index);
	}

	//Creation du buffer des materiaux
	GLuint material_index_buffer_id;
	glGenBuffers(1, &material_index_buffer_id);
	//On selectionne le normal buffer
	glBindBuffer(GL_ARRAY_BUFFER, material_index_buffer_id);
	//On remplit le buffer selectionne (le material index buffer). On divise par 3 parce qu'on a un material par TRIANGLE, pas par vertex
	//TODO bug ici des fois le vertex_buffer _size() est enorme et donc ca overflow la lecture de .data()
	glBufferData(GL_ARRAY_BUFFER, m_mesh.vertex_buffer_size(), material_index_buffer.data(), GL_STATIC_DRAW);

	//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
	GLint material_index_attribute = glGetAttribLocation(m_custom_shader, "material_index");
	glVertexAttribIPointer(material_index_attribute, 1, GL_UNSIGNED_INT, 0, 0);
	glEnableVertexAttribArray(material_index_attribute);



	//TODO HDR pipeline pour l'irradiance map

	//Reading the faces of the skybox and creating the OpenGL Cubemap
	std::vector<ImageData> cubemap_data;
	ImageData skysphere_data, irradiance_map_data;

	std::thread load_thread_cubemap = std::thread([&] {cubemap_data = Utils::read_cubemap_data("TPs/from_scratch/data/skybox", ".jpg"); });
	std::thread load_thread_skypshere = std::thread([&] {skysphere_data = Utils::read_skysphere_data("TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_Equirect.jpg"); });
	//std::thread load_thread_skypshere = std::thread([&] {skysphere_data = Utils::read_skysphere_data("irradiance_map.png"); });
	//std::thread load_thread_irradiance_map = std::thread([&] {irradiance_map_data = Utils::read_skysphere_data("TPs/from_scratch/data/irradiance_map_pink_384.png"); });
	std::thread load_thread_irradiance_map = std::thread([&] {irradiance_map_data = Utils::read_skysphere_data("TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_Equirect_Irradiance.jpg"); });
	load_thread_cubemap.join();
	load_thread_skypshere.join();
	load_thread_irradiance_map.join();


	m_cubemap = Utils::create_cubemap_from_data(cubemap_data);
	m_skysphere = Utils::create_skysphere_from_data(skysphere_data, 1);
	m_irradiance_map = Utils::create_skysphere_from_data(irradiance_map_data, 2);




	//Cleaning (repositionning the buffers that have been selected to their defaul value)
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return 0;
}

int TP::quit()
{
	return 0;//Error code 0 = no error
}

void TP::draw_general_settings()
{
	if (ImGui::Checkbox("Enable VSync", &m_application_settings.enable_vsync))
	{
		if (!m_application_settings.enable_vsync)
			vsync_off();
		else
			vsync_on();
	}
}

void TP::draw_lighting_window()
{
	if(ImGui::Checkbox("Use ambient", &m_application_settings.use_ambient))
		update_ambient_uniforms();

	if (m_application_settings.use_ambient)
		if(ImGui::ColorPicker4("Ambient color", (float*)(&m_application_settings.ambient_color)))
			update_ambient_uniforms();

	ImGui::Separator();
	ImGui::RadioButton("Use Skybox", &m_application_settings.cubemap_or_skysphere, 1); ImGui::SameLine();
	ImGui::RadioButton("Use Skysphere", &m_application_settings.cubemap_or_skysphere, 0);
}

void TP::draw_materials_window()
{
	static ImGuiTableFlags materials_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
		| ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersInnerH
		| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV
		| ImGuiTableFlags_BordersOuter
		| ImGuiTableFlags_BordersInner;

	if (ImGui::BeginTable("MaterialsTable", /* columns */ 2, materials_table_flags))
	{
		//Headers of the table
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Diffuse color");
		ImGui::TableHeadersRow();

		for (int i = 0; i < m_mesh.materials().materials.size(); i++)
		{
			const Material& mat = m_mesh.materials().materials.at(i);
			std::string& material_name = m_mesh.materials().names.at(i);

			ImGui::TableNextRow();
			for (int column = 0; column < 2; column++)
			{
				ImGui::TableSetColumnIndex(column);

				if (column == 0)
					//Name of the material
					ImGui::TextUnformatted(material_name.c_str());
				else if (column == 1)
				{
					//Diffuse color

					//We're using color button here instead of ColorEdit only to be able to change the size of the color "square"
					//Because we still want to retain the color picker feature of the ColorEdit, we're opening a popup if the ColorButton
					//is clicked
					if (ImGui::ColorButton(std::string("DiffuseColor#" + material_name).c_str(), ImVec4(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b, mat.diffuse.a), 0, ImVec2(32, 32)))
						ImGui::OpenPopup(std::string("DiffuseColor#" + material_name + " picker").c_str());

					if (ImGui::BeginPopup(std::string("DiffuseColor#" + material_name + " picker").c_str()))
					{
						//Adding a color picker to the popup

						if (ImGui::ColorPicker4("##picker", (float*)&mat.diffuse, ImGuiColorEditFlags_None, NULL))
						{
							//If the user interacted with the color of the material
							//We're updating the uniform of the shader
							setup_diffuse_color_uniform();
						}
						ImGui::EndPopup();
					}
				}
			}
		}

		ImGui::EndTable();
	}
}

void TP::draw_imgui()
{
	ImGui_ImplSdlGL3_NewFrame(m_window);

	ImGui::ShowDemoWindow();

	ImGui::Begin("General settings");
	draw_general_settings();
	ImGui::End();

	ImGui::Begin("Lighting");
	draw_lighting_window();
	ImGui::End();

	ImGui::Begin("Materials");
	draw_materials_window();
	ImGui::End();


	ImGui::Render();
	ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}

// dessiner une nouvelle image
int TP::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//draw(m_repere, /* model */ Identity(), camera());
		
	//On selectionne notre shader
	glUseProgram(m_custom_shader);

	//On update l'uniform mvpMatrix de notre shader
	Transform mvpMatrix = camera().projection() * camera().view() * Identity();
	GLint mvpMatrixLocation = glGetUniformLocation(m_custom_shader, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLocation, 1, GL_TRUE, mvpMatrix.data());

	//Setting the camera position
	GLint camera_position_uniform_location = glGetUniformLocation(m_custom_shader, "u_camera_position");
	glUniform3f(camera_position_uniform_location, m_camera.position().x, m_camera.position().y, m_camera.position().z);

	//Setting up the irradiance map
	GLint irradiance_map_uniform_location = glGetUniformLocation(m_custom_shader, "u_irradiance_map");
	//The irradiance map is in texture unit 2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_irradiance_map);
	glUniform1i(irradiance_map_uniform_location, 2);

	//On selectionne le vao du robot
	glBindVertexArray(m_robot_vao);
	//On draw le robot
	glDrawArrays(GL_TRIANGLES, 0, m_mesh.vertex_count());

	//TODO la skysphere/skybox s'affiche seulement quand on click sur un radio button et avant on a rien

	//Selecting the empty VAO for the cubemap shader
	glBindVertexArray(m_cubemap_vao);
	glUseProgram(m_cubemap_shader);
	GLint camera_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_camera_position");
	GLint use_cubemap_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_use_cubemap");
	GLint inverse_matrix_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_inverse_matrix");

	glUniform3f(camera_uniform_location, m_camera.position().x, m_camera.position().y, m_camera.position().z);
	glUniformMatrix4fv(inverse_matrix_uniform_location, 1, GL_TRUE, (m_camera.viewport() * m_camera.projection() * m_camera.view() * Identity()).inverse().data());
	glUniform1i(use_cubemap_uniform_location, m_application_settings.cubemap_or_skysphere);

	if (m_application_settings.cubemap_or_skysphere)
	{
		//Cubemap

		GLint cubemap_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_cubemap");
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

		//The cubemap is on texture unit 0 so we're using 0 for the value of the uniform
		glUniform1i(cubemap_uniform_location, 0);
	}
	else
	{
		//Skysphere

		GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_skysphere);

		//The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
		glUniform1i(skysphere_uniform_location, 1);
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);

	////////// ImGUI //////////
	draw_imgui();
	////////// ImGUI //////////

	//Cleaning stuff
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glBindVertexArray(0);

	return 1;
}
