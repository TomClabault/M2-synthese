
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

    if(m_resize_event_fired)
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
    vec3 init_max_bbox = vec3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

    m_cull_objects.resize(groups.size());

#pragma omp parallel for schedule(dynamic)
    for (int group_index = 0; group_index < groups.size(); group_index++)
    {
        TriangleGroup& group = groups.at(group_index);

        CullObject cull_object { init_min_bbox, 0, init_max_bbox, 0 };

        for (int pos = group.first; pos < group.first + group.n; pos += 3)
        {
            vec3 a, b, c;
            a = m_mesh.positions()[pos + 0];
            b = m_mesh.positions()[pos + 1];
            c = m_mesh.positions()[pos + 2];

            cull_object.pMin = min(cull_object.pMin, a);
            cull_object.pMin = min(cull_object.pMin, b);
            cull_object.pMin = min(cull_object.pMin, c);

            cull_object.pMax = max(cull_object.pMax, a);
            cull_object.pMax = max(cull_object.pMax, b);
            cull_object.pMax = max(cull_object.pMax, c);
        }

        cull_object.vertex_base = group.first;
        cull_object.vertex_count = group.n;

        m_cull_objects[group_index] = cull_object;
    }
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

    BoundingBox bbox = { object.pMin, object.pMax };

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

    BoundingBox bbox = { object.pMin, object.pMax };

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
    TIME(m_mesh = read_mesh("data/TPs/bistro-big/exterior.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/cube_plane_touching.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/sphere_high.obj"), "Load OBJ Time: ");
    //TIME(m_mesh = read_mesh("data/simple_plane.obj"), "Load OBJ Time: ");
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


    m_occlusion_culling_shader = read_program("data/TPs/shaders/occlusion_culling.glsl");
    program_print_errors(m_occlusion_culling_shader);

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

    GLint skysphere_uniform_location = glGetUniformLocation(m_cubemap_shader, "u_skysphere");
    //The skysphere is on texture unit 1 so we're using 1 for the value of the uniform
    glUniform1i(skysphere_uniform_location, 1);





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
    unsigned char default_texture_data[3] = {255, 255, 255};
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
    glVertexAttribPointer(normal_attribute, /* size */ 3, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*) position_size);
    glEnableVertexAttribArray(normal_attribute);
    glVertexAttribPointer(texcoord_attribute, /* size */ 2, /* type */ GL_FLOAT, GL_FALSE, /* stride */ 0, /* offset */ (GLvoid*) (position_size + normal_size));
    glEnableVertexAttribArray(texcoord_attribute);

    //Creating an empty VAO that will be used for the cubemap
    glGenVertexArrays(1, &m_cubemap_vao);

    //TODO sur un thread
    compute_bounding_boxes_of_groups(m_mesh_triangles_group);

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
    m_occlusion_culling_shader = read_program("data/TPs/shaders/occlusion_culling.glsl");
    program_print_errors(m_occlusion_culling_shader);

    glGenBuffers(1, &m_occlusion_culling_object_buffer);
    glGenBuffers(1, &m_occlusion_culling_indirect_param_buffer);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_object_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(TP2::CullObject) * m_mesh_triangles_group.size(), m_cull_objects.data(), GL_STATIC_DRAW);

    std::vector<TP2::MultiDrawIndirectParam> indirect_params(m_mesh_triangles_group.size());
    int index = 0;
    for (TP2::MultiDrawIndirectParam& param : indirect_params)
    {
        param.instance_base = 0;
        param.instance_count = 0;
        param.vertex_base = m_mesh_triangles_group[index].first;
        param.vertex_count = m_mesh_triangles_group[index].n;

        index++;
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_indirect_param_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(TP2::MultiDrawIndirectParam) * m_mesh_triangles_group.size(), indirect_params.data(), GL_DYNAMIC_DRAW);

    //Cleaning (repositionning the buffers that have been selected to their default value)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    //Point p_min, p_max;
    //TODO ça recalcule tous les bounds alors qu'on les a deja calculees
    //m_mesh.bounds(p_min, p_max);
    //m_camera.lookat(p_min, p_max);

    if(create_shadow_map() == -1)
        return -1;
    draw_shadow_map();

    if(create_hdr_frame() == -1)
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

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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
                    glActiveTexture(GL_TEXTURE0 + TP2::TRIANGLE_GROUP_BASE_COLOR_TEXTURE_UNIT); //The textures of the mesh are on unit 3
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
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_indirect_param_buffer);
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, m_mesh_triangles_group.size(), 0);
}

void TP2::draw_multi_draw_indirect_gpu_frustum(const Transform& mvp_matrix, const Transform& mvp_matrix_inverse)
{
    glUseProgram(m_occlusion_culling_shader);

    GLint mvp_matrix_uniform_location = glGetUniformLocation(m_occlusion_culling_shader, "u_mvp_matrix");
    glUniformMatrix4fv(mvp_matrix_uniform_location, 1, GL_FALSE, mvp_matrix.data());

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
        frustum_points_projective_space[i] = mvp_matrix_inverse(frustum_points_projective_space[i]);

    GLint frustum_world_space_vertices_uniform_location = glGetUniformLocation(m_occlusion_culling_shader, "frustum_world_space_vertices");
    glUniform3fv(frustum_world_space_vertices_uniform_location, 8, (float*)&frustum_points_projective_space[0]);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_occlusion_culling_indirect_param_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_occlusion_culling_object_buffer);

    int nb_groups = m_mesh_triangles_group.size() / 256 + (m_mesh_triangles_group.size() % 256 > 0);
    glDispatchCompute(nb_groups, 1, 1);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(m_texture_shadow_cook_torrance_shader);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_occlusion_culling_indirect_param_buffer);
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, m_mesh_triangles_group.size(), 0);
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

Mesh make_frustum( )
{
    glLineWidth(2);
    Mesh camera= Mesh(GL_LINES);

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

    m_mesh_groups_drawn = 0;

    glBindFramebuffer(GL_FRAMEBUFFER,  m_hdr_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //On selectionne notre shader
    glUseProgram(m_texture_shadow_cook_torrance_shader);

    //On update l'uniform mvpMatrix de notre shader
    Transform model_matrix;// = RotationX(90);
    Transform vp_matrix = m_camera.projection() * m_camera.view();
    Transform mvpMatrixInverse = vp_matrix.inverse();
    GLint model_matrix_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_model_matrix");
    glUniformMatrix4fv(model_matrix_uniform_location, 1, GL_TRUE, model_matrix.data());

    GLint mvp_matrix_uniform_location = glGetUniformLocation(m_texture_shadow_cook_torrance_shader, "u_vp_matrix");
    glUniformMatrix4fv(mvp_matrix_uniform_location, 1, GL_TRUE, vp_matrix.data());

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
    draw_multi_draw_indirect_gpu_frustum(vp_matrix, mvpMatrixInverse);

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
