
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
#include "tp2.h"
#include "utils.h"

#include <filesystem>
#include <thread>

// constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
TP2::TP2() : AppCamera(1280, 720, 3, 3, 16)
{
	m_app_timer = ApplicationTimer(this);
}

int TP2::get_window_width() { return window_width(); }
int TP2::get_window_height() { return window_height(); }

int TP2::prerender()
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
		if (key_state('s'))
		{
			static int calls = 1;
			clear_key_state('s');
			screenshot("app", calls++);
		}
	}

	// appelle la fonction update() de la classe derivee
	return update(global_time(), delta_time());
}

int TP2::postrender()
{
	m_app_timer.postrender();
		
	return 0;
}

void TP2::update_ambient_uniforms()
{
    glUseProgram(m_diffuse_texture_shader);

    GLuint use_irradiance_map_location = glGetUniformLocation(m_diffuse_texture_shader, "u_use_irradiance_map");
	glUniform1i(use_irradiance_map_location, m_application_settings.use_ambient);

	//TODO supprimer ambient color parce qu'on utilise que l'irradiance map, pas de ambient color a deux balles
    GLuint ambient_color_location = glGetUniformLocation(m_diffuse_texture_shader, "u_ambient_color");
	glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);
}

void TP2::setup_light_position_uniform(const vec3& light_position)
{
    glUseProgram(m_diffuse_texture_shader);

    GLint light_position_location = glGetUniformLocation(m_diffuse_texture_shader, "u_light_position");
	glUniform3f(light_position_location, light_position.x, light_position.y, light_position.z);
}

void TP2::load_mesh_textures_thread_function(const Materials& materials)
{
    m_mesh_textures.resize(materials.count());
    for (const Material& mat : materials.materials)
    {
        int diffuse_texture_index = mat.diffuse_texture;
        if (diffuse_texture_index != -1)
        {
            std::string texture_file_path = materials.texture_filenames[diffuse_texture_index];

            std::string texture_file_path_full = texture_file_path;
            ImageData texture_data = read_image_data(texture_file_path_full.c_str());

            GLuint texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         texture_data.width, texture_data.height, 0,
                         texture_data.channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
                         texture_data.data());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

            glGenerateMipmap(GL_TEXTURE_2D);

            m_mesh_textures[diffuse_texture_index] = texture_id;
        }
    }
}

void TP2::compute_bounding_boxes_of_groups(std::vector<TriangleGroup>& groups)
{
	vec3 init_min_bbox = vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec3 init_max_bbox = vec3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

	m_mesh_groups_bounding_boxes.resize(groups.size());

#pragma omp parallel for schedule(dynamic)
	for (int group_index = 0; group_index < groups.size(); group_index++)
	{
		TriangleGroup& group = groups.at(group_index);

		BoundingBox bbox {init_min_bbox, init_max_bbox};

		for (int pos = group.first; pos < group.first + group.n; pos += 3)
		{
			vec3 a, b, c;
			a = m_mesh.positions()[pos + 0];
			b = m_mesh.positions()[pos + 1];
			c = m_mesh.positions()[pos + 2];

			bbox.pMin = min(bbox.pMin, a);
			bbox.pMin = min(bbox.pMin, b);
			bbox.pMin = min(bbox.pMin, c);

			bbox.pMax = max(bbox.pMax, a);
			bbox.pMax = max(bbox.pMax, b);
			bbox.pMax = max(bbox.pMax, c);
		}

		m_mesh_groups_bounding_boxes[group_index] = bbox;
	}
}

bool is_point_in_view_frustum(vec4 point, const Transform& projMat)
{
	vec4 transformed_point = projMat(point);

	return (transformed_point.x >= -transformed_point.w && transformed_point.x <= transformed_point.w)
		&& (transformed_point.y >= -transformed_point.w && transformed_point.y <= transformed_point.w)
		&& (transformed_point.z >= -transformed_point.w && transformed_point.z <= transformed_point.w);
}

bool TP2::is_group_visible_frustum_culling(int group_index, const Transform& mvpMatrix)
{
	BoundingBox group_bbox = m_mesh_groups_bounding_boxes[group_index];

	/*
	* 
	*    6--------7
		/|       /|
	   / |      / |
	  2--------3  |
	  |  |     |  |
	  |  4-----|--5
	  |  /     | /
	  | /      |/
	  0--------1
	*/

	vec4 pBBox_0 = vec4(group_bbox.pMin, 1);
	vec4 pBBox_1 = vec4(group_bbox.pMax.x, group_bbox.pMin.y, group_bbox.pMin.z, 1);
	vec4 pBBox_2 = vec4(group_bbox.pMin.x, group_bbox.pMax.y, group_bbox.pMin.z, 1);
	vec4 pBBox_3 = vec4(group_bbox.pMax.x, group_bbox.pMax.y, group_bbox.pMin.z, 1);
	vec4 pBBox_4 = vec4(group_bbox.pMin.x, group_bbox.pMin.y, group_bbox.pMax.z, 1);
	vec4 pBBox_5 = vec4(group_bbox.pMax.x, group_bbox.pMin.y, group_bbox.pMax.z, 1);
	vec4 pBBox_6 = vec4(group_bbox.pMin.x, group_bbox.pMax.y, group_bbox.pMax.z, 1);
	vec4 pBBox_7 = vec4(group_bbox.pMax, 1);

	//TODO clip aussi si le Z < 0 ?
	if (is_point_in_view_frustum(pBBox_0, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_1, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_2, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_3, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_4, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_5, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_6, mvpMatrix)
		|| is_point_in_view_frustum(pBBox_7, mvpMatrix))
	{
		std::cout << "1 ";
		return true;
	}
	else
	{

		std::cout << "0 ";

		return false;
	}
}

// creation des objets de l'application
int TP2::init()
{
	//Setting ImGUI up
	ImGui::CreateContext();

	m_imgui_io = ImGui::GetIO();
	ImGui_ImplSdlGL3_Init(m_window);

	//Positioning the camera to a default state
    m_camera.read_orbiter("TPs/TP2/start_camera.txt");


	//Creating a basic grid
	//m_repere = make_grid(10);

	//Reading the mesh displayed
	//m_mesh = read_mesh("data/TPs/bistro-small-export/export.obj");
	//m_mesh = read_mesh("data/TPs/test_cubes.obj");
	m_mesh = read_mesh("data/cube.obj");
	if (m_mesh.positions().size() == 0)
	{
		std::cout << "The read mesh has 0 positions. Either the mesh file is incorrect or the mesh file wasn't found (incorrect path)" << std::endl;

		exit(-1);
	}



	// etat openGL par defaut
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
	glClearDepth(1.f);                          // profondeur par defaut

	glDepthFunc(GL_LEQUAL);                       // ztest, conserver l'intersection la plus proche de la camera
	glEnable(GL_DEPTH_TEST);                    // activer le ztest


	//Lecture du shader
    m_diffuse_texture_shader = read_program("data/TPs/shaders/shader_diffuse_texture.glsl");
    program_print_errors(m_diffuse_texture_shader);
    m_cubemap_shader = read_program("data/TPs/shaders/shader_cubemap.glsl");
	program_print_errors(m_cubemap_shader);

	GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");
	//The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
	glUniform1i(skysphere_uniform_location, 1);
		
    setup_light_position_uniform(TP2::LIGHT_POSITION);

    GLint use_irradiance_map_location = glGetUniformLocation(m_diffuse_texture_shader, "u_use_irradiance_map");
	glUniform1i(use_irradiance_map_location, m_application_settings.use_ambient);

    GLint diffuse_texture_uniform_location = glGetUniformLocation(m_diffuse_texture_shader, "u_mesh_texture");
    glUniform1i(diffuse_texture_uniform_location, 3);//The mesh texture is on unit 3

    GLint ambient_color_location = glGetUniformLocation(m_diffuse_texture_shader, "u_ambient_color");
	glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);





	//Creating an empty VAO that will be used for the cubemap
	glGenVertexArrays(1, &m_cubemap_vao);





	//Creating the VAO for the mesh that will be displayed
    glGenVertexArrays(1, &m_mesh_vao);
	//Selecting the VAO that we're going to configure
    glBindVertexArray(m_mesh_vao);

	//Creation du position buffer
    GLuint mesh_buffer;
    glGenBuffers(1, &mesh_buffer);
    //On selectionne le position buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh_buffer);
    size_t total_size = m_mesh.normal_buffer_size() + m_mesh.positions().size() * sizeof(vec3) + m_mesh.texcoord_buffer_size();
    //On definit la taille du buffer selectionne (le position buffer)
    glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_STATIC_DRAW);

    //Envoie des positions
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_mesh.positions().size() * sizeof(vec3), m_mesh.positions().data());
    size_t position_size = m_mesh.positions().size() * sizeof(vec3);

    //Envoie des normales
    glBufferSubData(GL_ARRAY_BUFFER, position_size, m_mesh.normal_buffer_size(), m_mesh.normal_buffer());
    size_t normal_size = m_mesh.normal_buffer_size();

    //Envoie des texcoords
    glBufferSubData(GL_ARRAY_BUFFER, position_size + normal_size, m_mesh.texcoord_buffer_size(), m_mesh.texcoord_buffer());


    glUseProgram(m_diffuse_texture_shader);
    //Setting the id of the attributes (set using layout in the shader)
    GLint position_attribute = 0;//glGetAttribLocation(m_diffuse_texture_shader, "position");
    GLint normal_attribute = 1;//glGetAttribLocation(m_diffuse_texture_shader, "normal");
    GLint texcoord_attribute = 2;//glGetAttribLocation(m_diffuse_texture_shader, "texcoords");

    glVertexAttribPointer(position_attribute, /* size */ 3, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ 0);
    glEnableVertexAttribArray(position_attribute);
    glVertexAttribPointer(normal_attribute, /* size */ 3, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*) position_size);
    glEnableVertexAttribArray(normal_attribute);
    glVertexAttribPointer(texcoord_attribute, /* size */ 2, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*) (position_size + normal_size));
    glEnableVertexAttribArray(texcoord_attribute);

    //Loading the textures on another thread
    //std::thread texture_thread(&TP2::load_mesh_textures_thread_function, this, std::ref(m_mesh.materials()));

	/*m_mesh_triangles_group = m_mesh.groups();

    Materials& materials = m_mesh.materials();
    m_mesh_textures.resize(materials.count());
    for (const Material& mat : materials.materials)
    {
        int diffuse_texture_index = mat.diffuse_texture;
        if (diffuse_texture_index != -1)
        {
            std::string texture_file_path = materials.texture_filenames[diffuse_texture_index];

            std::string texture_file_path_full = texture_file_path;
            ImageData texture_data = read_image_data(texture_file_path_full.c_str());

            GLuint texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         texture_data.width, texture_data.height, 0,
                         texture_data.channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
                         texture_data.data());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

            glGenerateMipmap(GL_TEXTURE_2D);

            m_mesh_textures[diffuse_texture_index] = texture_id;
        }
    }*/

	//TODO sur un thread
	m_mesh_triangles_group = m_mesh.groups();
	m_mesh_textures.resize(m_mesh.materials().filename_count());
	for (Material& mat : m_mesh.materials().materials)
	{
		int diffuse_texture_index = mat.diffuse_texture;
		if (diffuse_texture_index != -1)
		{
			std::string texture_file_path = m_mesh.materials().texture_filenames[diffuse_texture_index];
			ImageData texture_data = read_image_data(texture_file_path.c_str());

			GLuint texture_id;
			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
						 texture_data.width, texture_data.height, 0,
						 texture_data.channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
						 texture_data.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

			glGenerateMipmap(GL_TEXTURE_2D);

			m_mesh_textures[diffuse_texture_index] = texture_id;
		}
	}

	//TODO sur un thread
	compute_bounding_boxes_of_groups(m_mesh_triangles_group);

	//TODO HDR pipeline pour l'irradiance map

	//Reading the faces of the skybox and creating the OpenGL Cubemap
	std::vector<ImageData> cubemap_data;
	ImageData skysphere_data, irradiance_map_data;

    std::thread load_thread_cubemap = std::thread([&] {cubemap_data = Utils::read_cubemap_data("data/TPs/skybox", ".jpg"); });
	std::thread load_thread_skypshere = std::thread([&] {skysphere_data = Utils::read_skysphere_data(m_application_settings.irradiance_map_file_path.c_str()); });
	std::thread load_thread_irradiance_map = std::thread([&] {irradiance_map_data = Utils::precompute_and_load_associated_irradiance(m_application_settings.irradiance_map_file_path.c_str(), m_application_settings.irradiance_map_precomputation_samples); });
	load_thread_cubemap.join();
	load_thread_skypshere.join();
	load_thread_irradiance_map.join();


	m_cubemap = Utils::create_cubemap_texture_from_data(cubemap_data);
	m_skysphere = Utils::create_skysphere_texture_from_data(skysphere_data, 1);
    m_irradiance_map = Utils::create_skysphere_texture_from_data(irradiance_map_data, 2);

	//Cleaning (repositionning the buffers that have been selected to their default value)
	glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Point p_min, p_max;
    m_mesh.bounds(p_min, p_max);
    m_camera.lookat(p_min, p_max);

	return 0;
}

int TP2::quit()
{
	return 0;//Error code 0 = no error
}

void TP2::draw_general_settings()
{
	if (ImGui::Checkbox("Enable VSync", &m_application_settings.enable_vsync))
	{
		if (!m_application_settings.enable_vsync)
			vsync_off();
		else
			vsync_on();
	}
}

void TP2::update_recomputed_irradiance_map()
{
	if (m_application_state.irradiance_map_freshly_recomputed)
	{
		//An irradiance has just been recomputed, we're going to update the texture used by the shader
		GLuint new_irradiance_map_texture = Utils::create_skysphere_texture_from_data(m_recomputed_irradiance_map_data, 2);

		//Deleting the old texture
		glDeleteTextures(1, &m_irradiance_map);
		m_irradiance_map = new_irradiance_map_texture;

		m_application_state.irradiance_map_freshly_recomputed = false;
	}
}

void TP2::draw_lighting_window()
{
	/*if(ImGui::Checkbox("Use ambient", &m_application_settings.use_ambient))
		update_ambient_uniforms();

	if (m_application_settings.use_ambient)
		if(ImGui::ColorPicker4("Ambient color", (float*)(&m_application_settings.ambient_color)))
			update_ambient_uniforms();*/

	ImGui::Separator();
	ImGui::Text("Sky & Irradiance");
	if(ImGui::Checkbox("Use Irradiance Map", &m_application_settings.use_ambient))
		update_ambient_uniforms();
	ImGui::RadioButton("Use Skybox", &m_application_settings.cubemap_or_skysphere, 1); ImGui::SameLine();
	ImGui::RadioButton("Use Skysphere", &m_application_settings.cubemap_or_skysphere, 0);
    ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Irradiance map");
	ImGui::DragInt("Irradiance Map Precomputation Samples", &m_application_settings.irradiance_map_precomputation_samples, 1.0f, 1, 2048);
	ImGui::DragInt("Irradiance Map Downscale Factor", &m_application_settings.irradiance_map_precomputation_downscale_factor, 1.0f, 1, 8);
	if (ImGui::Button("Recompute"))
	{
		//If we are not already recomputing an irradiance map
		if (!m_application_state.currently_recomputing_irradiance)
		{
			m_application_state.currently_recomputing_irradiance = true;

			//Recomputing the irradiance map in a thread to avoid freezing the application
			std::thread recompute_thread([&] {
				m_recomputed_irradiance_map_data = Utils::precompute_and_load_associated_irradiance(m_application_settings.irradiance_map_file_path.c_str(), 
																									m_application_settings.irradiance_map_precomputation_samples, 
																									m_application_settings.irradiance_map_precomputation_downscale_factor);

				m_application_state.currently_recomputing_irradiance = false;
				m_application_state.irradiance_map_freshly_recomputed = true;
			});

			recompute_thread.detach();
		}
	}
	update_recomputed_irradiance_map();

	if (ImGui::Button("Clear Irradiance Maps Cache"))
	{
		//Removing all the files from the 
        for (const auto& entry : std::filesystem::directory_iterator(TP2::IRRADIANCE_MAPS_CACHE_FOLDER))
			std::filesystem::remove_all(entry.path());
	}
}

void TP2::draw_imgui()
{
	ImGui_ImplSdlGL3_NewFrame(m_window);

	ImGui::ShowDemoWindow();

	ImGui::Begin("General settings");
	draw_general_settings();
	ImGui::End();

	ImGui::Begin("Lighting");
	draw_lighting_window();
	ImGui::End();

    //ImGui::Begin("Materials");
    //draw_materials_window();
    //ImGui::End();


	ImGui::Render();
	ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}

// dessiner une nouvelle image
int TP2::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		

	//On selectionne notre shader
    glUseProgram(m_diffuse_texture_shader);

	//On update l'uniform mvpMatrix de notre shader
    Transform mvpMatrix = m_camera.projection() * m_camera.view() * Identity();
    GLint mvpMatrixLocation = glGetUniformLocation(m_diffuse_texture_shader, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLocation, 1, GL_TRUE, mvpMatrix.data());

	//Setting the camera position
    GLint camera_position_uniform_location = glGetUniformLocation(m_diffuse_texture_shader, "u_camera_position");
	glUniform3f(camera_position_uniform_location, m_camera.position().x, m_camera.position().y, m_camera.position().z);

	//Setting up the irradiance map
    GLint irradiance_map_uniform_location = glGetUniformLocation(m_diffuse_texture_shader, "u_irradiance_map");
	//The irradiance map is in texture unit 2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_irradiance_map);
	glUniform1i(irradiance_map_uniform_location, 2);

    //Selecting the VAO of the mesh
    glBindVertexArray(m_mesh_vao);

    //Drawing the mesh group by group
	for (TriangleGroup& group : m_mesh_triangles_group)
	{
		if (is_group_visible_frustum_culling(group.index, mvpMatrix))
		{
			int diffuse_texture_index = m_mesh.materials()(group.index).diffuse_texture;

			if (diffuse_texture_index != -1)
			{
				GLuint group_texture_id = m_mesh_textures[diffuse_texture_index];
				glActiveTexture(GL_TEXTURE3); //The textures of the mesh are on unit 3
				glBindTexture(GL_TEXTURE_2D, group_texture_id);
			}
			else
				//TODO afficher le material plutot que la texture --> changer de shader
				;

			glDrawArrays(GL_TRIANGLES, group.first, group.n);
		}
    }

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
