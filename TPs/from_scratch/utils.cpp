#include "image_hdr.h"
#include "mat.h"
#include "tp2.h"
#include "utils.h"
#include "vec.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <omp.h>

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

Image Utils::precompute_and_load_associated_irradiance(const char* skysphere_file_path, unsigned int samples, unsigned int downscale_factor)
{
    std::string skysphere_file_string = std::string(skysphere_file_path);
    //Only the name of the jpg (or png, bmp, ...) file without the path in front of it
    std::string skysphere_image_file_name = skysphere_file_string.substr(skysphere_file_string.rfind('/') + 1);

    //Creating the complete (path + image file name) file name of the irradiance map
    std::filesystem::create_directory(TP2::IRRADIANCE_MAPS_CACHE_FOLDER);
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
