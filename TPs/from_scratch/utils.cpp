#include "image_hdr.h"
#include "mat.h"
#include "tp.h"
#include "utils.h"
#include "vec.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <omp.h>
#include <vector>

/* The state must be initialized to non-zero */
uint32_t Utils::xorshift32(struct Utils::xorshift32_state* state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

void Utils::precompute_irradiance_map_from_skysphere_and_write(const char* skysphere_path, unsigned int samples, unsigned int downscale_factor, const char* output_irradiance_map_path)
{
    auto start = std::chrono::high_resolution_clock::now();
    Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere(skysphere_path, samples, downscale_factor);
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Writing the precomputed irradiance map to disk..." << std::endl;
    write_image_hdr(irradiance_map, output_irradiance_map_path);

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms --- " << (irradiance_map.width() * irradiance_map.height() * samples) / (float)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) << "samples/ms" << std::endl;
}

void Utils::precompute_irradiance_map_from_skysphere_and_write_gpu(const char* skysphere_path, unsigned int samples, unsigned int downscale_factor, const char* output_irradiance_map_path)
{
    auto start = std::chrono::high_resolution_clock::now();
    GLuint irradiance_map_texture = Utils::precompute_irradiance_map_from_skysphere_gpu(skysphere_path, samples, std::log2(downscale_factor));
    auto stop = std::chrono::high_resolution_clock::now();

    // Determine the size of the texture
    int width, height;
    glBindTexture(GL_TEXTURE_2D, irradiance_map_texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    Image irradiance_map(width, height);

    // Read pixel data from the texture
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, irradiance_map.data());

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Writing the precomputed irradiance map to disk..." << std::endl;
    write_image_hdr(irradiance_map, output_irradiance_map_path);

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms --- " << (irradiance_map.width() * irradiance_map.height() * samples) / (float)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) << "samples/ms" << std::endl;
    std::exit(0);
}

Image Utils::precompute_and_load_associated_irradiance(const char* skysphere_file_path, unsigned int samples, unsigned int downscale_factor)
{
    std::string skysphere_file_string = std::string(skysphere_file_path);
    //Only the name of the jpg (or png, bmp, ...) file without the path in front of it
    std::string skysphere_image_file_name = skysphere_file_string.substr(skysphere_file_string.rfind('/') + 1);

    //Creating the complete (path + image file name) file name of the irradiance map
    std::filesystem::create_directory(TP::IRRADIANCE_MAPS_CACHE_FOLDER);
    std::string irradiance_map_name = skysphere_file_string.substr(0, skysphere_file_string.rfind('/')) + "/irradiance_maps_cache/" + skysphere_image_file_name + "_Irradiance_" + std::to_string(samples) + "x_Down" + std::to_string(downscale_factor) + "x.hdr";

    //Checking whether the irradiance map already exists or not
    std::ifstream input_irradiance(irradiance_map_name);

    if (input_irradiance.is_open())
    {
        std::cout << "An irradiance map has been found!" << std::endl;
        //The irradiance map already exists
        return read_skysphere_image(irradiance_map_name.c_str());
    }
    else
    {
        //No irradiance map was found, precomputing it

        precompute_irradiance_map_from_skysphere_and_write(skysphere_file_path, samples, downscale_factor, irradiance_map_name.c_str());
        return read_skysphere_image(irradiance_map_name.c_str());
    }
}

Image Utils::precompute_and_load_associated_irradiance_gpu(const char* skysphere_file_path, unsigned int samples, unsigned int downscale_factor)
{
    std::string skysphere_file_string = std::string(skysphere_file_path);
    //Only the name of the jpg (or png, bmp, ...) file without the path in front of it
    std::string skysphere_image_file_name = skysphere_file_string.substr(skysphere_file_string.rfind('/') + 1);

    //Creating the complete (path + image file name) file name of the irradiance map
    std::filesystem::create_directory(TP::IRRADIANCE_MAPS_CACHE_FOLDER);
    std::string irradiance_map_name = skysphere_file_string.substr(0, skysphere_file_string.rfind('/')) + "/irradiance_maps_cache/" + skysphere_image_file_name + "_Irradiance_" + std::to_string(samples) + "x_Down" + std::to_string(downscale_factor) + "x.hdr";

    //Checking whether the irradiance map already exists or not
    std::ifstream input_irradiance(irradiance_map_name);

    if (input_irradiance.is_open())
    {
        std::cout << "An irradiance map has been found!" << std::endl;
        //The irradiance map already exists
        return read_skysphere_image(irradiance_map_name.c_str());
    }
    else
    {
        //No irradiance map was found, precomputing it

        precompute_irradiance_map_from_skysphere_and_write_gpu(skysphere_file_path, samples, downscale_factor, irradiance_map_name.c_str());
        return read_skysphere_image(irradiance_map_name.c_str());
    }
}

Image Utils::read_skysphere_image(const char* filename)
{
    return read_image_hdr(filename);
}

GLuint Utils::create_skysphere_texture_hdr(Image& skysphere_image, int texture_unit)
{
    GLuint skysphere;
    glGenTextures(1, &skysphere);
    //Texture 0 is the cubemap so we're incrementing by the texture unit number
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    //Selecting the cubemap as active
    glBindTexture(GL_TEXTURE_2D, skysphere);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, skysphere_image.width(), skysphere_image.height(), 0, GL_RGBA, GL_FLOAT, skysphere_image.data());

    // When scaling the texture down (displaying the texture on a small object), we want to linearly interpolate
    // between mipmaps levels and linearly interpolate the texel within the resulting mipmap level
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Linearly interpolating the texel color when displaying the texture on a big object (the texture is stretched)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamping method if we use texture coordinates that are out of the [0-1] range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return skysphere;
}

GLuint Utils::create_skysphere_texture_from_path(const char* filename, int texture_unit)
{
    Image skysphere_image = read_skysphere_image(filename);

    return create_skysphere_texture_hdr(skysphere_image, texture_unit);
}

void branchlessONB(const Vector& n, Vector& b1, Vector& b2)
{
	float sign = std::copysign(1.0f, n.z);
	const float a = -1.0f / (sign + n.z);
	const float b = n.x * n.y * a;
	b1 = Vector(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
	b2 = Vector(b, sign + n.y * n.y * a, -n.y);
}


Image Utils::precompute_irradiance_map_from_skysphere(const char* skysphere_path, unsigned int samples, unsigned int downscale_factor)
{
    Image skysphere_image = read_image_hdr(skysphere_path);

	if (downscale_factor > 1)
	{
		Image skysphere_image_downscaled;

		downscale_image(skysphere_image, skysphere_image_downscaled, downscale_factor);
		skysphere_image = skysphere_image_downscaled;
	}
	std::cout << "Skysphere loaded" << std::endl;

	Image irradiance_map(skysphere_image.width(), skysphere_image.height());

	std::cout << "Precomputing the irradiance map..." << std::endl;

	//Generating one random number generator for each thread
	std::vector<Utils::xorshift32_state> states;
	for (int i = 0; i < omp_get_max_threads(); i++)
	{
		Utils::xorshift32_state state;
		state.a = rand();

		states.push_back(state);
	}

	//Each thread is going to increment this variable after one line of the irradiance map has been computed
	//Because all thread increment this variable, it is atomic
	//This variable is then used to print a completion purcentage on stdout
	std::atomic<int> completed_lines(0);

#pragma omp parallel for schedule(dynamic)
		for (int y = 0; y < irradiance_map.height(); y++)
		{
            float theta = M_PI * (1.0f - (float)y / irradiance_map.height());
			for (int x = 0; x < irradiance_map.width(); x++)
			{
                float phi = 2.0f * M_PI * (0.5f - (float)x / irradiance_map.width());

				//The main direction we're going to randomly sample the skysphere around
                Vector normal = normalize(Vector(std::cos(phi) * std::sin(theta),
										  std::sin(phi) * std::sin(theta),
										  std::cos(theta)));
				
				//Calculating the vectors of the basis we're going to use to rotate the randomly generated vector
				//around our main direction
				Vector tangent, bitangent;

				branchlessONB(normal, tangent, bitangent);
				Transform onb(tangent, bitangent, normal, Vector(0, 0, 0));

				Color sum = Color(0, 0, 0);
				for (unsigned int i = 0; i < samples; i++)
				{
                    float rand1 = Utils::xorshift32(&states[omp_get_thread_num()]) / (float)std::numeric_limits<unsigned int>::max();
                    float rand2 = Utils::xorshift32(&states[omp_get_thread_num()]) / (float)std::numeric_limits<unsigned int>::max();

                    float phi_rand = 2.0f * M_PI * rand1;
                    float theta_rand = std::asin(std::sqrt(rand2));

                    Vector random_direction_in_canonical_hemisphere = normalize(Vector(std::cos(phi_rand) * std::sin(theta_rand),
                                                                                       std::sin(phi_rand) * std::sin(theta_rand),
                                                                                       std::cos(theta_rand)));
						
                    Vector random_direction_in_hemisphere_around_normal = onb(random_direction_in_canonical_hemisphere);
					Vector random_direction_rotated = random_direction_in_hemisphere_around_normal;

                    vec2 uv = vec2(0.5 - std::atan2(random_direction_rotated.y, random_direction_rotated.x) / (2.0 * M_PI),
                                   1.0 - std::acos(random_direction_rotated.z) / M_PI);

					Color sample_color = skysphere_image(uv.x * skysphere_image.width(), uv.y * skysphere_image.height());
					sum = sum + sample_color;
				}

				irradiance_map(x, y) = sum / (float)samples;
			}

			completed_lines++;

			if (omp_get_thread_num() == 0)
			{
				if (completed_lines % 20)
                {
                    printf("[%d*%d, %dx] - %.3f%% completed", skysphere_image.width(), skysphere_image.height(), samples, completed_lines / (float)skysphere_image.height() * 100);
                    std::cout << std::endl;
                }
			}
		}

	return irradiance_map;
}

#include "texture.h"

GLuint Utils::precompute_irradiance_map_from_skysphere_gpu(const char* skysphere_path, unsigned int samples, float mipmap_level)
{
    constexpr int SAMPLES_PER_ITERATION = 64;

    Image skysphere_image = read_image_hdr(skysphere_path);

    std::cout << "Skysphere loaded" << std::endl;

    GLuint irradiance_map_precomputation_shader = read_program("data/TPs/shaders/TPCG/irradiance_map_precomputation.glsl");
    program_print_errors(irradiance_map_precomputation_shader);
    glUseProgram(irradiance_map_precomputation_shader);

    //Creating the input texture for the skysphere
    GLuint skysphere_texture;
    glGenTextures(1, &skysphere_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skysphere_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, skysphere_image.width(), skysphere_image.height(), 0, GL_RGBA, GL_FLOAT, skysphere_image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    GLint skysphere_input_uniform_location = glGetUniformLocation(irradiance_map_precomputation_shader, "hdr_skysphere_input");
    GLint u_iteration_uniform_location = glGetUniformLocation(irradiance_map_precomputation_shader, "u_iteration");
    GLint u_sample_count_uniform_location = glGetUniformLocation(irradiance_map_precomputation_shader, "u_sample_count");
    GLint u_total_sample_count_uniform_location = glGetUniformLocation(irradiance_map_precomputation_shader, "u_total_sample_count");
    GLint u_mipmap_level_location = glGetUniformLocation(irradiance_map_precomputation_shader, "u_mipmap_level");

    int nb_iterations = std::ceil(samples / (float)SAMPLES_PER_ITERATION);
    glUniform1i(skysphere_input_uniform_location, 0);
    glUniform1i(u_sample_count_uniform_location, SAMPLES_PER_ITERATION);
    glUniform1i(u_total_sample_count_uniform_location, SAMPLES_PER_ITERATION * nb_iterations);
    glUniform1f(u_mipmap_level_location, mipmap_level);

    //Creating the input accumulation irradiance map texture
    GLuint irradiance_map_texture_output;
    glGenTextures(1, &irradiance_map_texture_output);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, irradiance_map_texture_output);
    {
        Image black_image(skysphere_image.width(), skysphere_image.height(), Color(0.0f, 0.0f, 0.0f));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, skysphere_image.width(), skysphere_image.height(), 0, GL_RGBA, GL_FLOAT, black_image.data());
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //No mipmaps
    glBindImageTexture(1, irradiance_map_texture_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);



    GLint threads[3];
    glGetProgramiv(irradiance_map_precomputation_shader, GL_COMPUTE_WORK_GROUP_SIZE, threads);

    int nb_groups_x = std::ceil(skysphere_image.width() / (float)threads[0]);
    int nb_groups_y = std::ceil(skysphere_image.height() / (float)threads[1]);
    glUseProgram(irradiance_map_precomputation_shader);

    //To avoid the compute shader timeout, we're going to do several iterations
    //of 64 samples
    for (int i = 0; i < nb_iterations; i++)
    {
        glUniform1i(u_iteration_uniform_location, i);
        glDispatchCompute(nb_groups_x, nb_groups_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glFinish();
        if (i % 16 == 0)
            std::cout << i << " --- " << (float)i / nb_iterations * 100.0f << "%" << std::endl;
    }

    glDeleteTextures(1, &skysphere_texture);
    glDeleteProgram(irradiance_map_precomputation_shader);

    return irradiance_map_texture_output;
}

std::vector<ImageData> Utils::read_cubemap_data(const char* folder_name, const char* face_extension)
{
	std::string face_names_std_string[6] = { "right", "left", "top", "bottom", "front", "back" };
	for (std::string& face_name : face_names_std_string)
		face_name = folder_name + std::string("/") + face_name + face_extension;

	std::vector<ImageData> faces_data;
	for (int i = 0; i < 6; i++)
		faces_data.push_back(flipY(read_image_data(face_names_std_string[i].c_str())));

	return faces_data;
}

GLuint Utils::create_cubemap_texture_from_data(std::vector<ImageData>& faces_data)
{
	//Creating the cubemap
	GLuint cubemap = 0;
	glGenTextures(1, &cubemap);
	glActiveTexture(GL_TEXTURE0);
	//Selecting the cubemap as active
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	for (int i = 0; i < 6; i++)
	{
		ImageData& face = faces_data[i];

		GLenum data_format;
		if (face.channels == 3)
			data_format = GL_RGB;
		else // Default
			data_format = GL_RGBA;

		glTexImage2D(/* what texture we're generating. This could be GL_TEXTURE_2D for a regular 2D
					 texture but because we're generating a cubemap, we have to precise which face of the
					 cubemap we're currently generating. This is done by getting the X+ OpenGL constant
					 (the first face of a cubemap is X+) and adding i so that we will cycle through all
					 the faces of the cubemap as i goes from 0 to 5 */ GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					 /* mipmap levels, 0 is the default */ 0,
					 /* the format we want the texture to be stored in the GPU as */ GL_RGBA,
					 /* width and height of one face */ face.width, face.height,
					 0,
					 /* pixel format of the face, RGB or RGBA typically */ data_format,
					 /* how the pixels are formatted in the data buffer. unsigned chars here */ GL_UNSIGNED_BYTE,
					 /* data of the face */ face.data());
	}

	// When scaling the texture down (displaying the texture on a small object), we want to linearly interpolate
	// between mipmaps levels and linearly interpolate the texel within the resulting mipmap level
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// Linearly interpolating the texel color when displaying the texture on a big object (the texture is stretched)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Clamping method if we use texture coordinates that are out of the [0-1] range
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Generating the MIPMAP levels for the cubemap
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Filtering on the boundary between two faces
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	return cubemap;
}

/*
 * @param folder_name The folder containing the faces of the cubemap. The folder path should NOT contain a trailing '/'
 * @param face_extension ".jpg", ".png", ....
 * The faces of the cubemap should be named "right", "left", "top", "bottom", "front" and "back"
 */
GLuint Utils::create_cubemap_texture_from_path(const char* folder_name, const char* face_extension)
{
	std::vector<ImageData> faces_data = read_cubemap_data(folder_name, face_extension);

	return create_cubemap_texture_from_data(faces_data);
}

void Utils::downscale_image(const Image& input_image, Image& downscaled_output, const int factor)
{
    if (input_image.width() % factor != 0)
    {
        std::cerr << "Image width isn't divisible by the factor";

        return;
    }

    if (input_image.height() % factor != 0)
    {
        std::cerr << "Image height isn't divisible by the factor";

        return;
    }


    int downscaled_width = input_image.width() / factor;
    int downscaled_height = input_image.height() / factor;
    downscaled_output = Image(downscaled_width, downscaled_height);

#pragma omp parallel for
    for (int y = 0; y < downscaled_height; y++)
    {
        for (int x = 0; x < downscaled_width; x++)
        {
            Color average;

            for (int i = 0; i < factor; i++)
                for (int j = 0; j < factor; j++)
                    average = average + input_image(x * factor + j, y * factor + i);

            average = average / (factor * factor);

            downscaled_output(x, y) = average;
        }
    }
}

std::vector<std::vector<float>> Utils::compute_mipmaps(const std::vector<float>& input_image, int width, int height, std::vector<std::pair<int, int>>& mipmaps_widths_heights)
{
    std::vector<std::vector<float>> mipmaps;
    mipmaps.push_back(input_image);
    mipmaps_widths_heights.push_back(std::make_pair(width, height));

    int level = 0;
    while (width > 4 && height > 4)//Stop at a 4*4 mipmap
    {
        int new_width = std::max(1, width / 2);
        int new_height = std::max(1, height / 2);

        mipmaps.push_back(std::vector<float>(new_width * new_height));
        mipmaps_widths_heights.push_back(std::make_pair(new_width, new_height));

        const std::vector<float>& previous_level = mipmaps[level];
        std::vector<float>& mipmap = mipmaps[level + 1];
        for (int y = 0; y < new_height; y++)
            for (int x = 0; x < new_width; x++)
                mipmap[y * new_width + x] = std::max(previous_level[x * 2 + y * 2 * width], std::max(previous_level[x * 2 + 1 + y * 2 * width], std::max(previous_level[x * 2 + (y * 2 + 1) * width], previous_level[x * 2 + 1 + (y * 2 + 1) * width])));

        width = new_width;
        height = new_height;
        level++;
    }

    return mipmaps;
}

std::vector<GLuint> Utils::compute_mipmaps_gpu(GLuint input_image, int width, int height, std::vector<std::pair<int, int>>& mipmaps_widths_heights)
{
    static GLuint compute_mipmap_shader_sampler = read_program("data/TPs/shaders/TPCG/compute_mipmap_sampler.glsl");
    program_print_errors(compute_mipmap_shader_sampler);
    static GLuint compute_mipmap_shader_image_unit = read_program("data/TPs/shaders/TPCG/compute_mipmap_image_unit.glsl");
    program_print_errors(compute_mipmap_shader_image_unit);

    std::vector<GLuint> mipmaps_texture_indices;
    mipmaps_texture_indices.push_back(input_image);
    mipmaps_widths_heights.push_back(std::make_pair(width, height));

    //TODO ne creer les texutres de mipmap que une seule fois plutot que a chaque fois
    int level = 0;
    while (width > 4 && height > 4)//Stop at a 4*4 mipmap
    {
        int new_width = std::max(1, width / 2);
        int new_height = std::max(1, height / 2);

        //For the first level, the input image is the depth buffer
        //We cannot use a depth texture directly in an image unit, we need to use
        //a sampler (and we don't want to blit the depth texture to a regular
        //texture because that's expensive)
        if (level == 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, input_image);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            //For the other levels, the input mipmap is a regular texture that can be used
            //in an image unit
            GLuint previous_level_texture = mipmaps_texture_indices[level];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, previous_level_texture);
            glBindImageTexture(1, previous_level_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
        }

        //Creating the texture for the mipmap that we're computing and binding to image
        //unit 1
        GLuint mipmap_texture;
        glGenTextures(1, &mipmap_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mipmap_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, new_width, new_height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //No mipmaps
        glBindImageTexture(1, mipmap_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        GLuint compute_shader;
        if (level == 0)
            //Using the program that has a sampler for the unit 0 because
            //the very first bitmap is not an image unit, it's a texture unit
            compute_shader = compute_mipmap_shader_sampler;
        else
            compute_shader = compute_mipmap_shader_image_unit;

        glUseProgram(compute_shader);
        glUniform1i(glGetUniformLocation(compute_shader, "input_mipmap"), 0);
        glUniform1i(glGetUniformLocation(compute_shader, "output_mipmap"), 1);


        int threads[3];
        glGetProgramiv(compute_shader, GL_COMPUTE_WORK_GROUP_SIZE, threads);
        int nb_groups_x = std::ceil((float)new_width / threads[0]);
        int nb_groups_y = std::ceil((float)new_height / threads[1]);

        glDispatchCompute(nb_groups_x, nb_groups_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        mipmaps_texture_indices.push_back(mipmap_texture);
        mipmaps_widths_heights.push_back(std::make_pair(new_width, new_height));

        width = new_width;
        height = new_height;
        level++;
    }

    return mipmaps_texture_indices;
}

std::vector<float> Utils::get_z_buffer(int window_width, int window_height, GLuint framebuffer)
{
    int previous_framebuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_framebuffer);

    //We want to read the depth buffer from the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    std::vector<float> tmp(window_width * window_height);

    //glReadBuffer(GL_BACK);
    glReadPixels(0, 0, window_width, window_height, GL_DEPTH_COMPONENT, GL_FLOAT, tmp.data());

    glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer);

    return tmp;
}

void Utils::get_object_screen_space_bounding_box(const Transform& mvp_matrix, const Transform& viewport_matrix, const TP2::CullObject& object, Point& out_bbox_min, Point& out_bbox_max)
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

int Utils::get_visibility_of_object_from_camera(const Transform& view_matrix, const TP2::CullObject& object)
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
