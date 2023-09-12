#include "mat.h"
#include "utils.h"
#include "vec.h"

#include <atomic>
#include <chrono>
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

void Utils::precompute_irradiance_map_from_skysphere_and_write(const char* skysphere_path, unsigned int samples, const char* output_irradiance_map_path)
{
	auto start = std::chrono::high_resolution_clock::now();
	Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere(skysphere_path, samples);
	auto stop = std::chrono::high_resolution_clock::now();

	write_image(irradiance_map, output_irradiance_map_path);

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms --- " << (irradiance_map.width() * irradiance_map.height() * samples) / (float)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) * 1000 << "samples/s" << std::endl;
}

Image Utils::precompute_irradiance_map_from_skysphere(const char* skysphere_path, unsigned int samples)
{
	Image skysphere_image = read_image(skysphere_path);
	std::cout << "Skysphere loaded" << std::endl;

	Image irradiance_map(skysphere_image.width(), skysphere_image.height());

	std::cout << "Precomputing the irradiance map..." << std::endl;
	float phi_step = (2 * M_PI) / (irradiance_map.width());
	float theta_step = M_PI / (irradiance_map.height());

	Utils::xorshift32_state state;
	state.a = rand();

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

#pragma omp parallel
	{
		int thread_id = omp_get_thread_num();

#pragma omp for
		for (int y = 0; y < irradiance_map.height(); y++)
		{
			float theta = M_PI - y * theta_step;
			//for (int x = irradiance_map.width() / 2; x < irradiance_map.width(); x++)
			for (int x = 0; x < irradiance_map.width(); x++)
			{
				float phi = x * phi_step;

				//The main direction we're going to randomly sample the skysphere around
				/*Vector normal = normalize(Vector(std::cos(phi) * std::sin(theta),
										  std::cos(theta),
										  -std::sin(phi) * std::sin(theta)));*/
				Vector normal = normalize(Vector(std::cos(phi) * std::sin(theta),
										  std::sin(phi) * std::sin(theta),
										  std::cos(theta)));
				//Spherical coordinates follow the convention that +Z is the up vector and +Y is the forward vector
				//We want +Y as the up vector and -Z as the forward vector
				
				
				
				
				
				normal = RotationX(-90)(normal);




				Vector arbitrary_vector = Vector(1, 0, 0);
				//To avoid issues when the main direction is colinear with the arbitrary vector
				if (1 - std::abs(dot(normal, arbitrary_vector)) < 1e-6f)
					arbitrary_vector = Vector(0, 1, 0);

				//Calculating the vectors of the basis we're going to use to rotate the randomly generated vector
				//around our main direction
				Vector tangent = normalize(cross(normal, arbitrary_vector));
				Vector bitangent = cross(tangent, normal);

				/*
				float tb = dot(tangent, bitangent)
				float bn = dot(bitangent, normal);
				float nt = dot(tangent, normal);
				if (tb != 0 || bn != 0 || nt != 0)
					std::cout << "Non orthongonaux" << std::endl;

				Vector tXb = cross(tangnt, normal);
				Vector bXn = cross(bitangent, normal);
				Vector nXt = cross(normal, tangent);

				if (tXb != normal)
					std::cout << "tangent != normal" << std::endl;
				*/


				//Transform obn(bitangent, normal, tangent, Vector(0, 0, 0));
				Transform obn(tangent, bitangent, normal, Vector(0, 0, 0));

				Color sum = Color(0, 0, 0);
				for (unsigned int i = 0; i < samples; i++)
				{
					float rand1 = Utils::xorshift32(&states.at(thread_id)) / (float)std::numeric_limits<unsigned int>::max();
					float rand2 = Utils::xorshift32(&states.at(thread_id)) / (float)std::numeric_limits<unsigned int>::max();

					float root = std::sqrt(1 - rand1 * rand1);
					float angle = 2 * M_PI * rand2;
					Vector random_direction_in_canonical_hemisphere(std::cos(angle) * root,
																	std::sin(angle) * root,
																	rand1);
						
					Vector random_direction_in_hemisphere_around_normal = obn(random_direction_in_canonical_hemisphere);
					Vector random_direction_rotated = random_direction_in_hemisphere_around_normal;

					//Vector random_direction_rotated;
					//do
					//{
					//	Vector random_direction = normalize(Vector(Utils::xorshift32(&state) / (float)std::numeric_limits<unsigned int>::max() * 2 - 1, 
					//									 Utils::xorshift32(&state) / (float)std::numeric_limits<unsigned int>::max() * 2 - 1,
					//									 Utils::xorshift32(&state) / (float)std::numeric_limits<unsigned int>::max() * 2 - 1));

					//	random_direction_rotated = obn(random_direction);

					//	//We're checking that the generated direction is in the upper hemisphere (and not below)
					//} while (dot(random_direction_rotated, normal) < 0.0f);

					vec2 uv = vec2(0.5 + std::atan2(random_direction_rotated.z, random_direction_rotated.x) / (2.0f * M_PI), 0.5 + std::asin(random_direction_rotated.y) / M_PI);

					sum = sum + skysphere_image(uv.x * skysphere_image.width(), uv.y * skysphere_image.height());
				}

				irradiance_map(x, y) = sum / (float)samples;
			}

			completed_lines++;

			if (thread_id == 0)
			{
				if (completed_lines % 40)
					printf("[%d*%d, %dx] - %.3f%% completed\n", skysphere_image.width(), skysphere_image.height(), samples, completed_lines / (float)skysphere_image.height() * 100);
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

GLuint Utils::create_cubemap_from_data(std::vector<ImageData>& faces_data)
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
GLuint Utils::create_cubemap_from_path(const char* folder_name, const char* face_extension)
{
	std::vector<ImageData> faces_data = read_cubemap_data(folder_name, face_extension);

	return create_cubemap_from_data(faces_data);
}

ImageData Utils::precompute_and_load_associated_irradiance(const char* skysphere_file_path, unsigned int samples)
{
	std::string skysphere_file_string = std::string(skysphere_file_path);
	//Creating the file name of the irradiance map
	std::string irradiance_map_name = skysphere_file_string.substr(0, skysphere_file_string.rfind('.')) + ".png";
	std::cout << "irradiance map name: " << irradiance_map_name << std::endl;

	//Checking whether the irradiance map already exists or not
	std::ifstream input_irradiance(irradiance_map_name);
	if (input_irradiance.is_open())
	{
		std::cout << "An irradiance map has been found!" << std::endl;
		//The irradiance map already exists
		return read_skysphere_data(irradiance_map_name.c_str());
	}
	else
	{
		//No irradiance map was found, precomputing it

		precompute_irradiance_map_from_skysphere_and_write(skysphere_file_path, samples, irradiance_map_name.c_str());
	}
}

ImageData Utils::read_skysphere_data(const char* filename)
{
	return read_image_data(filename);
}

GLuint Utils::create_skysphere_from_data(ImageData& skysphere_image_data, int texture_unit)
{
	GLuint skysphere;
	glGenTextures(1, &skysphere);
	//Texture 0 is the cubemap so we're incrementing by the texture unit number
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	//Selecting the cubemap as active
	glBindTexture(GL_TEXTURE_2D, skysphere);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, skysphere_image_data.width, skysphere_image_data.height, 0, skysphere_image_data.channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, skysphere_image_data.data());

	// When scaling the texture down (displaying the texture on a small object), we want to linearly interpolate
	// between mipmaps levels and linearly interpolate the texel within the resulting mipmap level
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// Linearly interpolating the texel color when displaying the texture on a big object (the texture is stretched)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Clamping method if we use texture coordinates that are out of the [0-1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Generating the MIPMAP levels for the cubemap
	//glGenerateMipmap(GL_TEXTURE_2D);

	return skysphere;
}

GLuint Utils::create_skysphere_from_path(const char* filename, int texture_unit)
{
	ImageData skysphere_image = read_skysphere_data(filename);

	return create_skysphere_from_data(skysphere_image, texture_unit);
}