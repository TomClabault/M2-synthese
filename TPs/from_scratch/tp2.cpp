
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.

#include "wavefront.h"
#include "texture.h"

#include "app_camera.h"
#include "draw.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "text.h"
#include "uniforms.h"

#include "application_settings.h"
#include "application_timer.h"
#include "tp2.h"
#include "utils.h"

#include <array>
#include <filesystem>
#include <thread>

// constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
TP2::TP2() : AppCamera(1280, 720, 4, 3, 8)
{
	m_app_timer = ApplicationTimer(this);
}

int TP2::get_window_width() { return window_width(); }
int TP2::get_window_height() { return window_height(); }

int TP2::mesh_groups_count() { return m_mesh_triangles_group.size(); }
int TP2::mesh_groups_drawn() { return m_mesh_groups_drawn; }

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

	if (m_resize_event_fired)
		resize_hdr_frame();

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
	glUseProgram(m_texture_shadow_cook_torrance_shader);

	GLuint use_irradiance_map_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_use_irradiance_map");
	glUniform1i(use_irradiance_map_location, m_application_settings.use_irradiance_map);

	//TODO supprimer ambient color parce qu'on utilise que l'irradiance map, pas de ambient color a deux balles
	GLuint ambient_color_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_ambient_color");
	glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);
}

GLuint TP2::create_opengl_texture(std::string& filepath, int GL_tex_format, float anisotropy)
{
	ImageData texture_data = read_image_data(filepath.c_str());

	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_tex_format,
		texture_data.width, texture_data.height, 0,
		texture_data.channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
		texture_data.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	if (anisotropy > 1.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

	glGenerateMipmap(GL_TEXTURE_2D);

	return texture_id;
}

void TP2::load_mesh_textures_thread_function(const Materials& materials)
{
	m_mesh_base_color_textures.resize(materials.count());
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

			m_mesh_base_color_textures[diffuse_texture_index] = texture_id;
		}
	}
}

void TP2::compute_bounding_boxes_of_groups(std::vector<TriangleGroup>& groups)
{
	vec3 init_min_bbox = vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    vec3 init_max_bbox = vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	m_cull_objects.resize(groups.size());

#pragma omp parallel for schedule(dynamic)
	for (int group_index = 0; group_index < groups.size(); group_index++)
	{
		TriangleGroup& group = groups.at(group_index);

		CullObject cull_object{ init_min_bbox, 0, init_max_bbox, 0 };

		for (int pos = group.first; pos < group.first + group.n; pos += 3)
		{
			vec3 a, b, c;
			a = m_mesh.positions()[pos + 0];
			b = m_mesh.positions()[pos + 1];
			c = m_mesh.positions()[pos + 2];

			cull_object.min = min(cull_object.min, a);
			cull_object.min = min(cull_object.min, b);
			cull_object.min = min(cull_object.min, c);

			cull_object.max = max(cull_object.max, a);
			cull_object.max = max(cull_object.max, b);
			cull_object.max = max(cull_object.max, c);
		}

		cull_object.vertex_base = group.first;
		cull_object.vertex_count = group.n;

		m_cull_objects[group_index] = cull_object;
	}
}

void TP2::compute_bounding_boxes_lines()
{
    std::vector<Point> lines_points;
    lines_points.reserve(m_cull_objects.size() * 12 * 2);

    for (const CullObject& cull_object : m_cull_objects)
    {
        std::vector<Point> bbox_points(8);
        bbox_points[0] = Point(cull_object.min);
        bbox_points[1] = Point(cull_object.max.x, cull_object.min.y, cull_object.min.z);
        bbox_points[2] = Point(cull_object.min.x, cull_object.max.y, cull_object.min.z);
        bbox_points[3] = Point(cull_object.max.x, cull_object.max.y, cull_object.min.z);
        bbox_points[4] = Point(cull_object.min.x, cull_object.min.y, cull_object.max.z);
        bbox_points[5] = Point(cull_object.max.x, cull_object.min.y, cull_object.max.z);
        bbox_points[6] = Point(cull_object.min.x, cull_object.max.y, cull_object.max.z);
        bbox_points[7] = Point(cull_object.max);

        /*
              6--------7
             /|       /|
            / |      / |
           2--------3  |
           |  |     |  |
           |  4-----|--5
           |  /     | /
           | /      |/
           0--------1
        */

        lines_points.push_back(bbox_points[0]);
        lines_points.push_back(bbox_points[1]);

        lines_points.push_back(bbox_points[0]);
        lines_points.push_back(bbox_points[4]);

        lines_points.push_back(bbox_points[1]);
        lines_points.push_back(bbox_points[5]);

        lines_points.push_back(bbox_points[4]);
        lines_points.push_back(bbox_points[5]);

        lines_points.push_back(bbox_points[2]);
        lines_points.push_back(bbox_points[3]);

        lines_points.push_back(bbox_points[2]);
        lines_points.push_back(bbox_points[6]);

        lines_points.push_back(bbox_points[7]);
        lines_points.push_back(bbox_points[3]);

        lines_points.push_back(bbox_points[6]);
        lines_points.push_back(bbox_points[7]);

        lines_points.push_back(bbox_points[0]);
        lines_points.push_back(bbox_points[2]);

        lines_points.push_back(bbox_points[1]);
        lines_points.push_back(bbox_points[3]);

        lines_points.push_back(bbox_points[4]);
        lines_points.push_back(bbox_points[6]);

        lines_points.push_back(bbox_points[5]);
        lines_points.push_back(bbox_points[7]);
    }

    m_bbox_lines = Lines(lines_points);
}

bool TP2::rejection_test_bbox_frustum_culling(const TP2::CullObject& object, const Transform& mvpMatrix)
{
	/*
		  6--------7
		 /|       /|
		/ |      / |
	   2--------3  |
	   |  |     |  |
	   |  4-----|--5
	   |  /     | /
	   | /      |/
	   0--------1
	*/

	BoundingBox bbox = { object.min, object.max };

	std::vector<vec4> bbox_points_projective(8);
	bbox_points_projective[0] = mvpMatrix(vec4(bbox.pMin, 1));
	bbox_points_projective[1] = mvpMatrix(vec4(bbox.pMax.x, bbox.pMin.y, bbox.pMin.z, 1));
	bbox_points_projective[2] = mvpMatrix(vec4(bbox.pMin.x, bbox.pMax.y, bbox.pMin.z, 1));
	bbox_points_projective[3] = mvpMatrix(vec4(bbox.pMax.x, bbox.pMax.y, bbox.pMin.z, 1));
	bbox_points_projective[4] = mvpMatrix(vec4(bbox.pMin.x, bbox.pMin.y, bbox.pMax.z, 1));
	bbox_points_projective[5] = mvpMatrix(vec4(bbox.pMax.x, bbox.pMin.y, bbox.pMax.z, 1));
	bbox_points_projective[6] = mvpMatrix(vec4(bbox.pMin.x, bbox.pMax.y, bbox.pMax.z, 1));
	bbox_points_projective[7] = mvpMatrix(vec4(bbox.pMax, 1));

	for (int coord_index = 0; coord_index < 6; coord_index++)
	{
		bool all_points_outside = true;

		for (int i = 0; i < 8; i++)
		{
			vec4& bbox_point = bbox_points_projective[i];

			bool test_negative_plane = coord_index & 1;

			if (test_negative_plane)
				all_points_outside &= bbox_point(coord_index / 2) < -bbox_point.w;
			else
				all_points_outside &= bbox_point(coord_index / 2) > bbox_point.w;

			if (!all_points_outside)
				break;
		}

		if (all_points_outside)
			return true;
	}

	return false;
}

bool TP2::rejection_test_bbox_frustum_culling_scene(const CullObject& object, const Transform& inverse_mvp_matrix)
{
	/*
	*
	*     6--------7
		 /|       /|
		/ |      / |
	   2--------3  |
	   |  |     |  |
	   |  4-----|--5
	   |  /     | /
	   | /      |/
	   0--------1
	*/

	BoundingBox bbox = { object.min, object.max };

	std::array<vec4, 8> frustum_points_projective_space
	{
		vec4(-1, -1, -1, 1),
		vec4(1, -1, -1, 1),
		vec4(-1, 1, -1, 1),
		vec4(1, 1, -1, 1),
		vec4(-1, -1, 1, 1),
		vec4(1, -1, 1, 1),
		vec4(-1, 1, 1, 1),
		vec4(1, 1, 1, 1)
	};

	std::vector<Vector> frustum_points_in_scene(8);
	for (int i = 0; i < 8; i++)
	{
		vec4 world_space = inverse_mvp_matrix(frustum_points_projective_space[i]);
		if (world_space.w != 0)
			frustum_points_in_scene[i] = Vector(world_space) / world_space.w;
	}

	for (int coord_index = 0; coord_index < 6; coord_index++)
	{
		bool all_points_outside = true;
		for (int i = 0; i < 8; i++)
		{
			Vector& frustum_point = frustum_points_in_scene[i];

			bool test_negative = coord_index & 1;

			if (test_negative)
				all_points_outside &= frustum_point(coord_index / 2) < bbox.pMin(coord_index / 2);
			else
				all_points_outside &= frustum_point(coord_index / 2) > bbox.pMax(coord_index / 2);

			if (!all_points_outside)
				break;
		}

		if (all_points_outside)
			return true;
	}

	return false;
}

#define TIME(x, message) { auto __start_timer = std::chrono::high_resolution_clock::now(); x; auto __stop_timer = std::chrono::high_resolution_clock::now(); std::cout << message << std::chrono::duration_cast<std::chrono::milliseconds>(__stop_timer - __start_timer).count() << "ms" << std::endl;}

// creation des objets de l'application
int TP2::init()
{
	//TP4 intro compute shader
//    //Initialisation des données sur le CPU
//    const int N = 200;
//    int global_counter_init_value = 0;
//    std::vector<int> input_data(N);
//    for (int i= 0; i < N; i++)
//        input_data[i] = i;

//    GLuint input_gpu_buffer, output_gpu_buffer;
//    glGenBuffers(1, &input_gpu_buffer);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, input_gpu_buffer);
//    //sizeof(int) * (N + 1) because we're allocating memory for the global_counter as well as the input data
//    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * (N + 1), NULL, GL_STREAM_READ);
//    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &global_counter_init_value);
//    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 1 * sizeof(int), sizeof(int) * N, input_data.data());

//    glGenBuffers(1, &output_gpu_buffer);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, output_gpu_buffer);
//    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * N, NULL, GL_STREAM_COPY);

//    GLuint compute_shader_program = read_program("data/TPs/shaders/occlusion_culling.glsl");
//    if (program_print_errors(compute_shader_program)) {
//        exit(EXIT_FAILURE);
//    }

//    glUseProgram(compute_shader_program);

//    int nb_groups = N / 256;
//    nb_groups += (N % 256) ? 1 : 0;
//    std::cout << "Nb groups: " << nb_groups << std::endl;

//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, input_gpu_buffer);
//    glUniform1i(glGetUniformLocation(compute_shader_program, "u_operand"), 0);

//    glDispatchCompute(nb_groups, 1, 1);
//    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

//    std::vector<int> result_data(N);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, output_gpu_buffer);
//    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * N, result_data.data());

//    for (int i = 0; i < N; i++)
//        std::cout << result_data[i] << ", ";

//    std::exit(0);

















	//Setting ImGUI up
	ImGui::CreateContext();

	m_imgui_io = ImGui::GetIO();
	ImGui_ImplSdlGL3_Init(m_window);

	//Positioning the camera to a default state
	m_camera.read_orbiter("data/TPs/start_camera_bistro.txt");
	m_light_camera.read_orbiter("data/TPs/light_camera_bistro.txt");
	m_lp_light_transform = TP2::LIGHT_CAMERA_ORTHO_PROJ_BISTRO * m_light_camera.view();

	//Reading the mesh displayed
	//TIME(m_mesh = read_mesh("data/TPs/bistro-small-export/export.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/TPs/bistro-big/exterior.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/sphere_high.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/simple_plane.obj"), "Load OBJ Time: ");
    TIME(m_mesh = read_mesh("data/TPs/cube_occlusion_culling.obj"), "Load OBJ Time: ");
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



	m_fullscreen_quad_texture_shader = read_program("data/TPs/shaders/shader_fullscreen_quad_texture.glsl");
	program_print_errors(m_fullscreen_quad_texture_shader);

	m_fullscreen_quad_texture_hdr_exposure_shader = read_program("data/TPs/shaders/shader_fullscreen_quad_texture_hdr_exposure.glsl");
	program_print_errors(m_fullscreen_quad_texture_hdr_exposure_shader);

	m_texture_shadow_cook_torrance_shader = read_program("data/TPs/shaders/shader_texture_shadow_cook_torrance_shader.glsl");
	program_print_errors(m_texture_shadow_cook_torrance_shader);

	GLint use_irradiance_map_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_use_irradiance_map");
	glUniform1i(use_irradiance_map_location, m_application_settings.use_irradiance_map);

	GLint base_color_texture_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_mesh_base_color_texture");
	glUniform1i(base_color_texture_uniform_location, TP2::TRIANGLE_GROUP_BASE_COLOR_TEXTURE_UNIT);

	GLint specular_texture_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_mesh_specular_texture");
	glUniform1i(specular_texture_uniform_location, TP2::TRIANGLE_GROUP_SPECULAR_TEXTURE_UNIT);

	GLint normal_map_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_mesh_normal_map");
	glUniform1i(normal_map_uniform_location, TP2::TRIANGLE_GROUP_NORMAL_MAP_UNIT);

	m_shadow_map_program = read_program("data/TPs/shaders/shader_shadow_map.glsl");
	program_print_errors(m_shadow_map_program);

    m_cubemap_shader = read_program("data/TPs/shaders/shader_cubemap.glsl");
	program_print_errors(m_cubemap_shader);

	//The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
	GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");
	glUniform1i(skysphere_uniform_location, 1);

    m_occlusion_culling_shader = read_program("data/TPs/shaders/TPCG/occlusion_culling.glsl");
    program_print_errors(m_occlusion_culling_shader);





	//Loading the textures on another thread
	//std::thread texture_thread(&TP2::load_mesh_textures_thread_function, this, std::ref(m_mesh.materials()));

	//TODO sur un thread
	auto start = std::chrono::high_resolution_clock::now();
	m_mesh_triangles_group = m_mesh.groups();
	m_mesh_base_color_textures.resize(m_mesh.materials().filename_count());
	m_mesh_specular_textures.resize(m_mesh.materials().filename_count());
	m_mesh_normal_maps.resize(m_mesh.materials().filename_count());
	for (Material& mat : m_mesh.materials().materials)
	{
		int diffuse_texture_index = mat.diffuse_texture;
		int specular_texture_index = mat.specular_texture;
		int normal_map_index = mat.normal_map;

		if (diffuse_texture_index != -1)
		{
			GLuint texture_id = create_opengl_texture(m_mesh.materials().texture_filenames[diffuse_texture_index], GL_SRGB_ALPHA);
			m_mesh_base_color_textures[diffuse_texture_index] = texture_id;
		}

		if (specular_texture_index != -1)
		{
			GLuint texture_id = create_opengl_texture(m_mesh.materials().texture_filenames[specular_texture_index], GL_RGB);
			m_mesh_specular_textures[specular_texture_index] = texture_id;
		}

		if (normal_map_index != -1)
		{
			GLuint texture_id = create_opengl_texture(m_mesh.materials().texture_filenames[normal_map_index], GL_RGB);
			m_mesh_normal_maps[normal_map_index] = texture_id;
		}
	}
	auto stop = std::chrono::high_resolution_clock::now();
	std::cout << "Texture loading time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

	//Generating the default textures that the triangle groups that don't have texture will use
	unsigned char default_texture_data[3] = { 255, 255, 255 };
	glGenTextures(1, &m_default_texture);
	glBindTexture(GL_TEXTURE_2D, m_default_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, default_texture_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);

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


	glUseProgram(m_texture_shadow_cook_torrance_shader);
	//Setting the id of the attributes (set using layout in the shader)
	GLint position_attribute = 0;//glGetAttribLocation(m_diffuse_texture_shader, "position");
	GLint normal_attribute = 1;//glGetAttribLocation(m_diffuse_texture_shader, "normal");
	GLint texcoord_attribute = 2;//glGetAttribLocation(m_diffuse_texture_shader, "texcoords");

	glVertexAttribPointer(position_attribute, /* size */ 3, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ 0);
	glEnableVertexAttribArray(position_attribute);
	glVertexAttribPointer(normal_attribute, /* size */ 3, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*)position_size);
	glEnableVertexAttribArray(normal_attribute);
	glVertexAttribPointer(texcoord_attribute, /* size */ 2, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*)(position_size + normal_size));
	glEnableVertexAttribArray(texcoord_attribute);

	//Creating an empty VAO that will be used for the cubemap
	glGenVertexArrays(1, &m_cubemap_vao);

	//TODO sur un thread
	compute_bounding_boxes_of_groups(m_mesh_triangles_group);
    //TODO sur un thread
    //compute_bounding_boxes_lines();//TODO lines not drawing

	//Reading the faces of the skybox and creating the OpenGL Cubemap
	std::vector<ImageData> cubemap_data;
	Image skysphere_image, irradiance_map_image;

	std::thread load_thread_cubemap = std::thread([&] {cubemap_data = Utils::read_cubemap_data("data/TPs/skybox", ".jpg"); });
	std::thread load_thread_skypshere = std::thread([&] {skysphere_image = Utils::read_skysphere_image(m_application_settings.irradiance_map_file_path.c_str()); });
	std::thread load_thread_irradiance_map = std::thread([&] {irradiance_map_image = Utils::precompute_and_load_associated_irradiance(m_application_settings.irradiance_map_file_path.c_str(), m_application_settings.irradiance_map_precomputation_samples, m_application_settings.irradiance_map_precomputation_downscale_factor); });
	load_thread_cubemap.join();
	load_thread_skypshere.join();
	load_thread_irradiance_map.join();

	m_cubemap = Utils::create_cubemap_texture_from_data(cubemap_data);
	m_skysphere = Utils::create_skysphere_texture_hdr(skysphere_image, TP2::SKYSPHERE_UNIT);
	m_irradiance_map = Utils::create_skysphere_texture_hdr(irradiance_map_image, TP2::DIFFUSE_IRRADIANCE_MAP_UNIT);

	// ---------- Preparing for multi-draw indirect: ---------- //
    glUseProgram(m_occlusion_culling_shader);

    glGenBuffers(1, &m_occlusion_culling_output_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_output_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TP2::MultiDrawIndirectParam) * m_mesh_triangles_group.size(), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &m_occlusion_culling_drawn_objects_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_drawn_objects_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * m_mesh_triangles_group.size(), nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &m_occlusion_culling_object_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_object_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TP2::CullObject) * m_mesh_triangles_group.size(), nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(TP2::CullObject) * m_cull_objects.size(), m_cull_objects.data());

	glGenBuffers(1, &m_occlusion_culling_parameter_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_parameter_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

    //Setting this parameter for the occlusion culling
    //Because it is initially set to -1, we're going to try to draw
    //every objects on the first frame
    m_nb_objects_drawn_last_frame = -1;
    m_debug_z_buffer = Image(1280, 720);//read_image_pfm("../../gkit2light/zbuffer_viewport.pfm");

	//Cleaning (repositionning the buffers that have been selected to their default value)
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	Point p_min, p_max;
    m_mesh.bounds(p_min, p_max);
    m_camera.lookat(p_min, p_max);
    m_camera.read_orbiter("app_orbiter.txt");

	if (create_shadow_map() == -1)
		return -1;
	draw_shadow_map();

	if (create_hdr_frame() == -1)
		return -1;

	return 0;
}

int TP2::quit()
{
	return 0;//Error code 0 = no error
}

int TP2::create_hdr_frame()
{
	glGenTextures(1, &m_hdr_shader_output_texture);
	glBindTexture(GL_TEXTURE_2D, m_hdr_shader_output_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width(), window_height(), 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint render_buffer;
	glGenRenderbuffers(1, &render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width(), window_height());
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &m_hdr_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_hdr_framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdr_shader_output_texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return -1;

	glBindFramebuffer(GL_FRAMEBUFFER, 0); //Cleaning

	return 0;
}

int TP2::resize_hdr_frame()
{
	glDeleteTextures(1, &m_hdr_shader_output_texture);
	glDeleteFramebuffers(1, &m_hdr_framebuffer);

	return create_hdr_frame();
}

int TP2::create_shadow_map()
{
	glGenTextures(1, &m_shadow_map);
	glBindTexture(GL_TEXTURE_2D, m_shadow_map);

	glTexImage2D(GL_TEXTURE_2D, 0,
		GL_DEPTH_COMPONENT32F, TP2::SHADOW_MAP_RESOLUTION, TP2::SHADOW_MAP_RESOLUTION, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

	float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glGenerateMipmap(GL_TEXTURE_2D);

	glGenFramebuffers(1, &m_shadow_map_framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_map_framebuffer);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_DEPTH_ATTACHMENT, m_shadow_map, /* mipmap */ 0);

	if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return -1;

	//Cleaning
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return 0;
}

void TP2::draw_shadow_map()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_map_framebuffer);
	glViewport(0, 0, TP2::SHADOW_MAP_RESOLUTION, TP2::SHADOW_MAP_RESOLUTION);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glCullFace(GL_FRONT);

	glUseProgram(m_shadow_map_program);
	GLint mlp_matrix_uniform_location = glGetUniformLocation(m_shadow_map_program, "mlp_matrix");
	if (m_application_settings.bind_light_camera_to_camera)
		glUniformMatrix4fv(mlp_matrix_uniform_location, 1, GL_TRUE, (m_camera.projection() * m_camera.view()).data());
	else
		glUniformMatrix4fv(mlp_matrix_uniform_location, 1, GL_TRUE, m_lp_light_transform.data());

	glBindVertexArray(m_mesh_vao);
	glDrawArrays(GL_TRIANGLES, 0, m_mesh.triangle_count() * 3);

	//Cleaning
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, window_width(), window_height());
	glBindVertexArray(0);
	glCullFace(GL_BACK);
}

void TP2::draw_by_groups_cpu_frustum_culling(const Transform& vp_matrix, const Transform& mvp_matrix_inverse)
{
	m_mesh_groups_drawn = 0;

	GLint has_normal_map_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_has_normal_map");
	for (TriangleGroup& group : m_mesh_triangles_group)
	{
		if (!rejection_test_bbox_frustum_culling(m_cull_objects[group.index], vp_matrix))
		{
			if (!rejection_test_bbox_frustum_culling_scene(m_cull_objects[group.index], mvp_matrix_inverse))
			{
				int diffuse_texture_index = m_mesh.materials()(group.index).diffuse_texture;
				int specular_texture_index = m_mesh.materials()(group.index).specular_texture;
				int normal_map_index = m_mesh.materials()(group.index).normal_map;

				if (diffuse_texture_index != -1)
				{
					GLuint group_base_color_texture_id = m_mesh_base_color_textures[diffuse_texture_index];
					glActiveTexture(GL_TEXTURE0 + TP2::TRIANGLE_GROUP_BASE_COLOR_TEXTURE_UNIT);
					glBindTexture(GL_TEXTURE_2D, group_base_color_texture_id);
				}
				else
				{
					glActiveTexture(GL_TEXTURE0 + TP2::TRIANGLE_GROUP_BASE_COLOR_TEXTURE_UNIT);
					glBindTexture(GL_TEXTURE_2D, m_default_texture);
				}

				if (specular_texture_index != -1)
				{
					GLuint group_specular_texture_id = m_mesh_specular_textures[specular_texture_index];
					glActiveTexture(GL_TEXTURE0 + TP2::TRIANGLE_GROUP_SPECULAR_TEXTURE_UNIT);
					glBindTexture(GL_TEXTURE_2D, group_specular_texture_id);
				}

				if (normal_map_index != -1)
				{
					GLuint group_normal_map_id = m_mesh_normal_maps[normal_map_index];
					glActiveTexture(GL_TEXTURE0 + TP2::TRIANGLE_GROUP_NORMAL_MAP_UNIT);
					glBindTexture(GL_TEXTURE_2D, group_normal_map_id);

					glUniform1i(has_normal_map_uniform_location, 1);
				}
				else
					glUniform1i(has_normal_map_uniform_location, 0);

				glDrawArrays(GL_TRIANGLES, group.first, group.n);

				m_mesh_groups_drawn++;
			}
		}
	}
}

void TP2::draw_multi_draw_indirect()
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_output_buffer);
	glMultiDrawArraysIndirect(GL_TRIANGLES, 0, m_mesh_triangles_group.size(), 0);
}

bool all(vec3 a)
{
	return a.x && a.y && a.z;
}

vec3 lessThanOrEq(vec4 a, vec4 b)
{
	return vec3(a.x <= b.x, a.y <= b.y, a.z <= b.z);
}

vec3 greaterThanOrEq(vec4 a, vec4 b)
{
	return vec3(a.x >= b.x, a.y >= b.y, a.z >= b.z);
}

void TP2::cpu_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse)
{
    std::vector<TP2::MultiDrawIndirectParam> params;

	std::array<Vector, 8> frustum_world_space_vertices;
	std::array<vec4, 8> frustum_points_projective_space
	{
	    vec4(-1, -1, -1, 1),
	    vec4(1, -1, -1, 1),
	    vec4(-1, 1, -1, 1),
	    vec4(1, 1, -1, 1),
	    vec4(-1, -1, 1, 1),
	    vec4(1, -1, 1, 1),
	    vec4(-1, 1, 1, 1),
	    vec4(1, 1, 1, 1)
	};

	for (int i = 0; i < 8; i++)
	{
	    frustum_points_projective_space[i] = mvp_matrix_inverse(frustum_points_projective_space[i]);

	    if (frustum_points_projective_space[i].w != 0)
	        frustum_world_space_vertices[i] = Vector(frustum_points_projective_space[i]) / frustum_points_projective_space[i].w;
	}

	m_mesh_groups_drawn = 0;
	for (int object_id = 0; object_id < m_cull_objects.size(); object_id++)
	{
	    CullObject cull_object = m_cull_objects[object_id];

	    vec4 bbox_points_projective[8];
	    bbox_points_projective[0] = mvp_matrix(vec4(cull_object.min, 1));
	    bbox_points_projective[1] = mvp_matrix(vec4(cull_object.max.x, cull_object.min.y, cull_object.min.z, 1));
	    bbox_points_projective[2] = mvp_matrix(vec4(cull_object.min.x, cull_object.max.y, cull_object.min.z, 1));
	    bbox_points_projective[3] = mvp_matrix(vec4(cull_object.max.x, cull_object.max.y, cull_object.min.z, 1));
	    bbox_points_projective[4] = mvp_matrix(vec4(cull_object.min.x, cull_object.min.y, cull_object.max.z, 1));
	    bbox_points_projective[5] = mvp_matrix(vec4(cull_object.max.x, cull_object.min.y, cull_object.max.z, 1));
	    bbox_points_projective[6] = mvp_matrix(vec4(cull_object.min.x, cull_object.max.y, cull_object.max.z, 1));
	    bbox_points_projective[7] = mvp_matrix(vec4(cull_object.max, 1));

	    bool next_object = false;
	    for (int coord_index = 0; coord_index < 6; coord_index++)
	    {
	        bool all_points_outside = true;

	        for (int i = 0; i < 8; i++)
	        {
	            vec4 bbox_point = bbox_points_projective[i];

	            int test_negative_plane = coord_index & 1;

	            if (test_negative_plane == 1)
	                all_points_outside = all_points_outside && (bbox_point(coord_index / 2) < -bbox_point.w);
	            else
	                all_points_outside = all_points_outside && (bbox_point(coord_index / 2) > bbox_point.w);

	            if (!all_points_outside)
	                break;
	        }

	        //If all the points are on the same side
	        if (all_points_outside)
	        {
	            next_object = true;

	            break;
	        }
	    }

	    if (next_object)
	        continue;

	    for (int coord_index = 0; coord_index < 6; coord_index++)
	    {
	        bool all_points_outside = true;
	        for (int i = 0; i < 8; i++)
	        {
	            vec3 frustum_point = frustum_world_space_vertices[i];

	            int test_negative = coord_index & 1;

	            if (test_negative == 1)
	                all_points_outside = all_points_outside && (frustum_point(coord_index / 2) < cull_object.min(coord_index / 2));
	            else
	                all_points_outside = all_points_outside && (frustum_point(coord_index / 2) > cull_object.max(coord_index / 2));

	            //If all the points are not on the same side
	            if (!all_points_outside)
	                break;
	        }

	        if (all_points_outside)
	            break;
	    }

		m_mesh_groups_drawn++;

		//The object has not been culled, we're going to push the params
		//for the object to be drawn by the future MDI call
		TP2::MultiDrawIndirectParam object_draw_params;
		object_draw_params.instance_base = 0;
		object_draw_params.instance_count = 1;
		object_draw_params.vertex_base = m_cull_objects[object_id].vertex_base;
		object_draw_params.vertex_count = m_cull_objects[object_id].vertex_count;

        params.push_back(object_draw_params);
        m_objects_drawn_last_frame.push_back(object_id);
	}

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_output_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(TP2::MultiDrawIndirectParam) * m_cull_objects.size(), params.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_drawn_objects_id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int) * m_objects_drawn_last_frame.size(), m_objects_drawn_last_frame.data());
    glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_occlusion_culling_parameter_buffer);
    glBufferSubData(GL_PARAMETER_BUFFER_ARB, 0, sizeof(unsigned int), &m_mesh_groups_drawn);
}

void TP2::gpu_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse)
{
	glUseProgram(m_occlusion_culling_shader);

	GLint mvp_matrix_uniform_location = glGetUniformLocation(m_occlusion_culling_shader, "u_mvp_matrix");
	glUniformMatrix4fv(mvp_matrix_uniform_location, 1, GL_TRUE, mvp_matrix.data());

	std::array<Vector, 8> frustum_points_world_space;
	std::array<vec4, 8> frustum_points_projective_space
	{
		vec4(-1, -1, -1, 1),
		vec4(1, -1, -1, 1),
		vec4(-1, 1, -1, 1),
		vec4(1, 1, -1, 1),
		vec4(-1, -1, 1, 1),
		vec4(1, -1, 1, 1),
		vec4(-1, 1, 1, 1),
		vec4(1, 1, 1, 1)
	};

	for (int i = 0; i < 8; i++)
	{
		frustum_points_projective_space[i] = mvp_matrix_inverse(frustum_points_projective_space[i]);

		if (frustum_points_projective_space[i].w != 0)
			frustum_points_world_space[i] = Vector(frustum_points_projective_space[i]) / frustum_points_projective_space[i].w;
	}

	GLint frustum_world_space_vertices_uniform_location = glGetUniformLocation(m_occlusion_culling_shader, "frustum_world_space_vertices");
	glUniform3fv(frustum_world_space_vertices_uniform_location, 8, (float*)frustum_points_world_space.data());

    //TODO besoin d'utiliser base ?
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_occlusion_culling_output_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_occlusion_culling_drawn_objects_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_occlusion_culling_object_buffer);

	unsigned int zero = 0;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_occlusion_culling_parameter_buffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &zero);

	int nb_groups = m_mesh_triangles_group.size() / 256 + (m_mesh_triangles_group.size() % 256 > 0);
	glDispatchCompute(nb_groups, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    //Getting the number of groups drawn for displaying with ImGui
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_parameter_buffer);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &m_mesh_groups_drawn);
}

void TP2::draw_mdi_frustum_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse)
{
    if (m_application_settings.gpu_frustum_culling)
        gpu_mdi_frustum_culling(mvp_matrix, mvp_matrix_inverse);
    else
        cpu_mdi_frustum_culling(mvp_matrix, mvp_matrix_inverse);

	glUseProgram(m_texture_shadow_cook_torrance_shader);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_output_buffer);
    glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_occlusion_culling_parameter_buffer);
    glMultiDrawArraysIndirectCountARB(GL_TRIANGLES, 0, 0, m_cull_objects.size(), 0);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void get_object_screen_space_bounding_box(const Transform& mvp_matrix, const Transform& viewport_matrix, const TP2::CullObject& object, Point& out_bbox_min, Point& out_bbox_max)
{
    Point object_screen_space_bbox_points[8];
    object_screen_space_bbox_points[0] = viewport_matrix(mvp_matrix(Point(object.min)));
    object_screen_space_bbox_points[1] = viewport_matrix(mvp_matrix(Point(object.max.x, object.min.y, object.min.z)));
    object_screen_space_bbox_points[2] = viewport_matrix(mvp_matrix(Point(object.min.x, object.max.y, object.min.z)));
    object_screen_space_bbox_points[3] = viewport_matrix(mvp_matrix(Point(object.max.x, object.max.y, object.min.z)));
    object_screen_space_bbox_points[4] = viewport_matrix(mvp_matrix(Point(object.min.x, object.min.y, object.max.z)));
    object_screen_space_bbox_points[5] = viewport_matrix(mvp_matrix(Point(object.max.x, object.min.y, object.max.z)));
    object_screen_space_bbox_points[6] = viewport_matrix(mvp_matrix(Point(object.min.x, object.max.y, object.max.z)));
    object_screen_space_bbox_points[7] = viewport_matrix(mvp_matrix(Point(object.max)));

    out_bbox_min = Point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    out_bbox_max = Point(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i = 0; i < 8; i++)
    {
        out_bbox_min = min(out_bbox_min, object_screen_space_bbox_points[i]);
        out_bbox_max = max(out_bbox_max, object_screen_space_bbox_points[i]);
    }
}

int get_visibility_of_object_from_camera(const Transform& view_matrix, const TP2::CullObject& object)
{
    Point view_space_points[8];

    //TODO we only need the z coordinate so the whole matrix-point multiplication isn't needed, too slow
    view_space_points[0] = view_matrix(Point(object.min));
    view_space_points[1] = view_matrix(Point(object.max.x, object.min.y, object.min.z));
    view_space_points[2] = view_matrix(Point(object.min.x, object.max.y, object.min.z));
    view_space_points[3] = view_matrix(Point(object.max.x, object.max.y, object.min.z));
    view_space_points[4] = view_matrix(Point(object.min.x, object.min.y, object.max.z));
    view_space_points[5] = view_matrix(Point(object.max.x, object.min.y, object.max.z));
    view_space_points[6] = view_matrix(Point(object.min.x, object.max.y, object.max.z));
    view_space_points[7] = view_matrix(Point(object.max));

    bool all_behind = true;
    bool all_in_front = true;
    for (int i = 0; i < 8; i++)
    {
        bool point_behind = view_space_points[i].z > 0;

        all_behind &= point_behind;
        all_in_front &= !point_behind;
    }

    if (all_behind)
        return 0;
    else if (all_in_front)
        return 1;
    else
        return 2;
}

std::vector<Image> compute_mipmaps(const Image& input_image)
{
    std::vector<Image> mipmaps;
    mipmaps.push_back(input_image);

    int width = input_image.width();
    int height = input_image.height();

    int level = 0;
    while (width > 4 && height > 4)//Stop at a 4*4 mipmap
    {
        int new_width = std::max(1, width / 2);
        int new_height = std::max(1, height / 2);

        mipmaps.push_back(Image(new_width, new_height));

        const Image& previous_level = mipmaps[level];
        Image& mipmap = mipmaps[level + 1];
        for (int y = 0; y < new_height; y++)
            for (int x = 0; x < new_width; x++)
                mipmap(x, y) = Color(std::max(previous_level(x * 2, y * 2).r, std::max(previous_level(x * 2 + 1, y * 2).r, std::max(previous_level(x * 2, y * 2 + 1).r, previous_level(x * 2 + 1, y * 2 + 1).r))));

//        for (int y = 0; y < height; y += 2)
//            for (int x = 0; x < width; x += 2)
//                mipmap(x / 2, y / 2) = Color(std::max(previous_level(x, y).r, std::max(previous_level(x + 1, y).r, std::max(previous_level(x, y  + 1).r, previous_level(x + 1, y + 1).r))));

        width = new_width;
        height = new_height;
        level++;
    }

    return mipmaps;
}

void TP2::draw_mdi_occlusion_culling(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse)
{
    Image debug_bboxes_image(window_width(), window_height());
    Image debug_bboxes_mipmap_image;
    Image debug_bboxes_zbuffer_mipmap_image;
    Image debug_zbuffer_mipmap_image;
    std::vector<Image> debug_mipmaps_with_bboxes;

    std::vector<Image> mipmaps;

    static int debug_counter = 0;
    if(m_nb_objects_drawn_last_frame == -1)
    {
        //We're going to try to draw every objects
        cpu_mdi_frustum_culling(mvp_matrix, mvp_matrix_inverse);
        m_nb_objects_drawn_last_frame = m_mesh_groups_drawn;
    }
    else
    {
        std::vector<int> drawn_objects_id(m_nb_objects_drawn_last_frame);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_occlusion_culling_drawn_objects_id);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int) * m_nb_objects_drawn_last_frame, drawn_objects_id.data());

        mipmaps = compute_mipmaps(m_debug_z_buffer);

//        //TODO remove debug
//        debug_mipmaps_with_bboxes = mipmaps;

        for (int i = 0; i < m_nb_objects_drawn_last_frame; i++)
        {
            CullObject object = m_cull_objects[drawn_objects_id[i]];

            Point screen_space_bbox_min, screen_space_bbox_max;
            float nearest_depth;

            int visibility = get_visibility_of_object_from_camera(m_camera.view(), object);
            if (visibility == 2) //Partially visible, we're going to assume
            //that the bounding box of the object
            //spans the whole image
            {
                screen_space_bbox_min = Point(0, 0, 0);
                screen_space_bbox_max = Point(window_width() - 1, window_height() - 1, 0);
            }
            else if (visibility == 1) //Entirely visible
            {
                get_object_screen_space_bounding_box(mvp_matrix, Viewport(window_width(), window_height()), object, screen_space_bbox_min, screen_space_bbox_max);

                //Clamping the points to the image limits
                screen_space_bbox_min = max(screen_space_bbox_min, Point(0, 0, -std::numeric_limits<float>::max()));
                screen_space_bbox_max = min(screen_space_bbox_max, Point(window_width() - 1, window_height() - 1, std::numeric_limits<float>::max()));
            }
            else //Not visible
                continue;

            //We're going to consider that all the pixels of the object are at the same depth,
            //this depth because the closest one to the camera
            //Because the closest depth is the biggest z, we're querrying the max point of the bbox
            nearest_depth = screen_space_bbox_min.z;

            //Computing which mipmap level to choose for the depth test so that the
            //screens space bounding rectangle of the object is approximately 4x4
            int mipmap_level = 0;
            //Getting the biggest axis of the screen space bounding rectangle of the object
            int largest_extent = std::max(screen_space_bbox_max.x - screen_space_bbox_min.x, screen_space_bbox_max.y - screen_space_bbox_min.y);
            if (largest_extent > 4)
                //Computing the factor needed for the largest extent to be 16 pixels
                mipmap_level = std::log2(std::ceil(largest_extent / 4.0f));
            else //The extent of the bounding rectangle already is small enough
                ;
            int reduction_factor = std::pow(2, mipmap_level);
            float reduction_factor_inverse = 1.0f / reduction_factor;

            const Image& mipmap = mipmaps[mipmap_level];

            bool one_pixel_visible = false;
            int min_y = std::floor(screen_space_bbox_min.y * reduction_factor_inverse);
            int max_y = std::ceil(screen_space_bbox_max.y * reduction_factor_inverse);
            int min_x = std::floor(screen_space_bbox_min.x * reduction_factor_inverse);
            int max_x = std::ceil(screen_space_bbox_max.x * reduction_factor_inverse);

            for (int y = min_y; y <= max_y; y++)
            {
                for (int x = min_x; x <= max_x; x++)
                {
                    float depth_buffer_depth = mipmap(x, y).r;
                    if (depth_buffer_depth >= nearest_depth)
                    {
                        //The object needs to be rendered, we can stop here
                        one_pixel_visible = true;

                        break;
                    }
                }

                if (one_pixel_visible)
                    break;
            }

//            //TODO remove, debug only
//            if (debug_counter % 4 == 0)
//            {
//                if (one_pixel_visible)
//                    std::cout << Point(object.min) << ", " << Point(object.max) << ", " << "visible --- near depth: " << nearest_depth << std::endl;
//                else
//                    std::cout << Point(object.min) << ", " << Point(object.max) << ", " << "hidden --- near depth: " << nearest_depth << std::endl;
//            }

//            //TODO remove, debug only
//            {
//                if (i == 0)
//                {
//                    debug_zbuffer_mipmap_image = mipmap;
//                    debug_bboxes_zbuffer_mipmap_image = mipmap;//Bboxes drawn on top of zbuffer
//                    debug_bboxes_mipmap_image = Image(mipmap.width(), mipmap.height());

//                    for (int x = min_x; x <= max_x; x++)
//                    {
//                        //debug_bboxes_mipmap_image(x, min_y) = Color(1.0f, 0, 0);
//                        //debug_bboxes_mipmap_image(x, max_y) = Color(1.0f, 0, 0);

//                        debug_bboxes_zbuffer_mipmap_image(x, min_y) = Color(1.0f, 0, 0);
//                        debug_bboxes_zbuffer_mipmap_image(x, max_y) = Color(1.0f, 0, 0);
//                    }

//                    for (int y = min_y; y <= max_y; y++)
//                    {
//                        //debug_bboxes_mipmap_image(min_x, y) = Color(1.0f, 0, 0);
//                        //debug_bboxes_mipmap_image(max_x, y) = Color(1.0f, 0, 0);

//                        debug_bboxes_zbuffer_mipmap_image(min_x, y) = Color(1.0f, 0, 0);
//                        debug_bboxes_zbuffer_mipmap_image(max_x, y) = Color(1.0f, 0, 0);
//                    }

//                    for (int i = 0; i < mipmaps.size(); i++)
//                    {
//                        int reduction_factor = std::pow(2, i);
//                        float reduction_factor_inverse = 1.0f  / reduction_factor;
//                        int min_y = std::floor(screen_space_bbox_min.y * reduction_factor_inverse);
//                        int max_y = std::ceil(screen_space_bbox_max.y * reduction_factor_inverse);
//                        int min_x = std::floor(screen_space_bbox_min.x * reduction_factor_inverse);
//                        int max_x = std::ceil(screen_space_bbox_max.x * reduction_factor_inverse);

//                        for (int x = min_x; x <= max_x; x++)
//                        {
//                            debug_mipmaps_with_bboxes[i](x, min_y) = Color(1.0f, 0, 0);
//                            debug_mipmaps_with_bboxes[i](x, max_y) = Color(1.0f, 0, 0);
//                        }

//                        for (int y = min_y; y <= max_y; y++)
//                        {
//                            debug_mipmaps_with_bboxes[i](min_x, y) = Color(1.0f, 0, 0);
//                            debug_mipmaps_with_bboxes[i](max_x, y) = Color(1.0f, 0, 0);
//                        }
//                    }
//                }

//                for (int x = screen_space_bbox_min.x; x <= screen_space_bbox_max.x; x++)
//                {
//                    debug_bboxes_image(x, screen_space_bbox_min.y) = Color(1.0f, 0, 0);
//                    debug_bboxes_image(x, screen_space_bbox_max.y) = Color(1.0f, 0, 0);
//                }

//                for (int y = screen_space_bbox_min.y; y <= screen_space_bbox_max.y; y++)
//                {
//                    debug_bboxes_image(screen_space_bbox_min.x, y) = Color(1.0f, 0, 0);
//                    debug_bboxes_image(screen_space_bbox_max.x, y) = Color(1.0f, 0, 0);
//                }
//            }
        }

//        //TODO remove debug
//        if (debug_counter % 4 == 0)
//            std::cout << std::endl;

        cpu_mdi_frustum_culling(mvp_matrix, mvp_matrix_inverse);
        m_nb_objects_drawn_last_frame = m_mesh_groups_drawn;
    }

    debug_counter++;

    glUseProgram(m_texture_shadow_cook_torrance_shader);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_output_buffer);
    glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_occlusion_culling_parameter_buffer);
    glMultiDrawArraysIndirectCountARB(GL_TRIANGLES, 0, 0, m_cull_objects.size(), 0);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::vector<float> tmp(m_debug_z_buffer.width() * m_debug_z_buffer.height());

    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, m_debug_z_buffer.width(), m_debug_z_buffer.height(), GL_DEPTH_COMPONENT, GL_FLOAT, tmp.data());

    // conversion en image
    for(unsigned i= 0; i < m_debug_z_buffer.size(); i++)
        m_debug_z_buffer(i)= Color(tmp[i]);

    //TODO remove debug
    {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard)
            if (key_state('d'))
            {
                write_image(debug_bboxes_image, "debug_bboxes.png");
                write_image(debug_bboxes_mipmap_image, "debug_bboxes_mipmap.png");
                write_image(debug_zbuffer_mipmap_image, "debug_zbuffer_mipmap.png");
                write_image(debug_bboxes_zbuffer_mipmap_image, "debug_zbuffer_bboxes_mipmap.png");

                write_image(m_debug_z_buffer, "debug_z_buffer.png");

                for (int i = 0; i < mipmaps.size(); i++)
                    write_image(mipmaps[i], std::string(std::string("debug_z_buffer_mipmap") + std::to_string(i) + std::string(".png")).c_str());

                for (int i = 0; i < mipmaps.size(); i++)
                    write_image(debug_mipmaps_with_bboxes[i], std::string(std::string("debug_z_buffer_bboxes_mipmap") + std::to_string(i) + std::string(".png")).c_str());
            }
    }
}

void TP2::draw_skysphere()
{
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

		glActiveTexture(GL_TEXTURE0 + TP2::SKYBOX_UNIT);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

		//The cubemap is on texture unit 0 so we're using 0 for the value of the uniform
		glUniform1i(cubemap_uniform_location, 0);
	}
	else
	{
		//Skysphere

		GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");

		glActiveTexture(GL_TEXTURE0 + TP2::SKYSPHERE_UNIT);
		glBindTexture(GL_TEXTURE_2D, m_skysphere);

		//The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
		glUniform1i(skysphere_uniform_location, 1);
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

Mesh make_frustum()
{
    glLineWidth(2);
    Mesh camera = Mesh(GL_LINES);

    camera.color(Yellow());
    // face avant
    camera.vertex(-1, -1, -1);
    camera.vertex(-1, 1, -1);
    camera.vertex(-1, 1, -1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, -1, -1);
    camera.vertex(1, -1, -1);
    camera.vertex(-1, -1, -1);

    // face arriere
    camera.vertex(-1, -1, 1);
    camera.vertex(-1, 1, 1);
    camera.vertex(-1, 1, 1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, -1, 1);
    camera.vertex(1, -1, 1);
    camera.vertex(-1, -1, 1);

    // aretes
    camera.vertex(-1, -1, -1);
    camera.vertex(-1, -1, 1);
    camera.vertex(-1, 1, -1);
    camera.vertex(-1, 1, 1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, -1, -1);
    camera.vertex(1, -1, 1);

    return camera;
}

void TP2::draw_light_camera_frustum()
{
	if (m_application_settings.draw_light_camera_frustum)
	{
		// affiche le frustum de la camera
		Mesh frustum_mesh = make_frustum();
		draw(frustum_mesh, m_lp_light_transform.inverse(), camera());
	}
}

void TP2::draw_fullscreen_quad_texture(GLuint texture_to_draw)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(m_fullscreen_quad_texture_shader);

	GLint texture_sampler_location = glGetUniformLocation(m_fullscreen_quad_texture_shader, "u_texture");
	glActiveTexture(GL_TEXTURE0 + TP2::FULLSCREEN_QUAD_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, texture_to_draw);
	glUniform1i(texture_sampler_location, TP2::FULLSCREEN_QUAD_TEXTURE_UNIT);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void TP2::draw_fullscreen_quad_texture_hdr_exposure(GLuint texture_to_draw)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(m_fullscreen_quad_texture_hdr_exposure_shader);

	GLint u_exposure_uniform_location = glGetUniformLocation(m_fullscreen_quad_texture_hdr_exposure_shader, "u_exposure");
	glUniform1f(u_exposure_uniform_location, m_application_settings.hdr_exposure);

	GLint texture_sampler_location = glGetUniformLocation(m_fullscreen_quad_texture_shader, "u_texture");
	glActiveTexture(GL_TEXTURE0 + TP2::FULLSCREEN_QUAD_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, texture_to_draw);
	glUniform1i(texture_sampler_location, TP2::FULLSCREEN_QUAD_TEXTURE_UNIT);

	glDrawArrays(GL_TRIANGLES, 0, 6);
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

    ImGui::Separator();
    ImGui::Text("Frustum Culling");
    ImGui::RadioButton("CPU Frustum Culling", &m_application_settings.gpu_frustum_culling, 0); ImGui::SameLine();
    ImGui::RadioButton("GPU Frustum Culling", &m_application_settings.gpu_frustum_culling, 1);
}

void TP2::update_recomputed_irradiance_map()
{
	if (m_application_state.irradiance_map_freshly_recomputed)
	{
		//An irradiance has just been recomputed, we're going to update the texture used by the shader
		GLuint new_irradiance_map_texture = Utils::create_skysphere_texture_hdr(m_recomputed_irradiance_map_data, 2);

		//Deleting the old texture
		glDeleteTextures(1, &m_irradiance_map);
		m_irradiance_map = new_irradiance_map_texture;

		m_application_state.irradiance_map_freshly_recomputed = false;
	}
}

void TP2::draw_lighting_window()
{
	ImGui::Separator();
	ImGui::Text("Sky & Irradiance");
	if (ImGui::Checkbox("Use Irradiance Map", &m_application_settings.use_irradiance_map))
		update_ambient_uniforms();
	ImGui::RadioButton("Use Skybox", &m_application_settings.cubemap_or_skysphere, 1); ImGui::SameLine();
	ImGui::RadioButton("Use Skysphere", &m_application_settings.cubemap_or_skysphere, 0);
	ImGui::Separator();
	ImGui::Checkbox("Draw Shadow Map", &m_application_settings.draw_shadow_map);
	ImGui::Checkbox("Bind Light Camera to Camera", &m_application_settings.bind_light_camera_to_camera);
	ImGui::Checkbox("Show Light Camera Frustum", &m_application_settings.draw_light_camera_frustum);
	ImGui::Separator();
	ImGui::SliderFloat3("Light Position", (float*)&m_light_pos, -100.0f, 100.0f);
	ImGui::PushItemWidth(256);
	ImGui::SliderFloat("Shadows Intensity", &m_application_settings.shadow_intensity, 0.0f, 1.0f);
	ImGui::SliderFloat("HDR Tone Mapping Exposure", &m_application_settings.hdr_exposure, 0.0f, 10.0f);
	ImGui::PopItemWidth();
	ImGui::Separator();
	ImGui::Text("Irradiance map");
	ImGui::PushItemWidth(128);
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

			m_application_state.irradiance_map_freshly_recomputed = true;
            m_application_state.currently_recomputing_irradiance = false;
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

void TP2::draw_material_window()
{
	ImGui::Checkbox("Override material", &m_application_settings.override_material);
	ImGui::SliderFloat("Roughness", &m_application_settings.mesh_roughness, 0, 1);
	ImGui::SliderFloat("Metalness", &m_application_settings.mesh_metalness, 0, 1);
}

void TP2::draw_imgui()
{
	ImGui_ImplSdlGL3_NewFrame(m_window);

	//ImGui::ShowDemoWindow();

	ImGui::Begin("General settings");
	draw_general_settings();
	ImGui::End();

	ImGui::Begin("Lighting");
	draw_lighting_window();
	ImGui::End();

	ImGui::Begin("Material");
	draw_material_window();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}

// dessiner une nouvelle image
int TP2::render()
{
	if (m_application_settings.draw_shadow_map)
	{
		draw_shadow_map();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_fullscreen_quad_texture_shader);
		GLint texture_sampler_location = glGetUniformLocation(m_fullscreen_quad_texture_shader, "u_texture");
		glActiveTexture(GL_TEXTURE0 + TP2::SHADOW_MAP_UNIT);
		glBindTexture(GL_TEXTURE_2D, m_shadow_map);
		glUniform1i(texture_sampler_location, TP2::SHADOW_MAP_UNIT);

		glBindVertexArray(m_cubemap_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		////////// ImGUI //////////
		draw_imgui();
		////////// ImGUI //////////

		return 1;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_hdr_framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//On selectionne notre shader
	glUseProgram(m_texture_shadow_cook_torrance_shader);

	//On update l'uniform mvpMatrix de notre shader
	Transform model_matrix;// = RotationX(90);
	Transform mvp_matrix = m_camera.projection() * m_camera.view() * model_matrix;
    Transform mvp_matrix_inverse = mvp_matrix.inverse();
	GLint model_matrix_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_model_matrix");
	glUniformMatrix4fv(model_matrix_uniform_location, 1, GL_TRUE, model_matrix.data());

	GLint mvp_matrix_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_vp_matrix");
	glUniformMatrix4fv(mvp_matrix_uniform_location, 1, GL_TRUE, mvp_matrix.data());

	GLint mlp_matrix_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_lp_matrix");
	glUniformMatrix4fv(mlp_matrix_uniform_location, 1, GL_TRUE, m_lp_light_transform.data());

	//Setting the camera position
	GLint camera_position_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_camera_position");
	glUniform3f(camera_position_uniform_location, m_camera.position().x, m_camera.position().y, m_camera.position().z);

	GLint light_position_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_light_position");
	glUniform3f(light_position_uniform_location, m_light_pos.x, m_light_pos.y, m_light_pos.z);

	GLint override_material_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_override_material");
	glUniform1i(override_material_uniform_location, m_application_settings.override_material);

	GLint metalness_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_metalness");
	glUniform1f(metalness_uniform_location, m_application_settings.mesh_metalness);

	GLint roughness_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_roughness");
	glUniform1f(roughness_uniform_location, m_application_settings.mesh_roughness);

	//Setting up the irradiance map
	GLint irradiance_map_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_irradiance_map");
	//The irradiance map is in texture unit 2
	glActiveTexture(GL_TEXTURE0 + TP2::DIFFUSE_IRRADIANCE_MAP_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_irradiance_map);
	glUniform1i(irradiance_map_uniform_location, TP2::DIFFUSE_IRRADIANCE_MAP_UNIT);

	GLint shadow_map_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_shadow_map");
	glActiveTexture(GL_TEXTURE0 + TP2::SHADOW_MAP_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_shadow_map);
	glUniform1i(shadow_map_uniform_location, TP2::SHADOW_MAP_UNIT);

	GLint shadow_intensity_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_shadow_intensity");
	glUniform1f(shadow_intensity_uniform_location, m_application_settings.shadow_intensity);

	//Selecting the VAO of the mesh
	glBindVertexArray(m_mesh_vao);

//    //Drawing the mesh group by group
//    draw_by_groups_cpu_frustum_culling(vp_matrix, mvpMatrixInverse);
//    draw_multi_draw_indirect();
//    draw_mdi_frustum_culling(mvp_matrix, mvp_matrix_inverse);
    draw_mdi_occlusion_culling(mvp_matrix, mvp_matrix_inverse);

    draw_skysphere();
    draw_light_camera_frustum();
    draw_fullscreen_quad_texture_hdr_exposure(m_hdr_shader_output_texture);

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
