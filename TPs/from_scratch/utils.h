#ifndef UTILS_H
#define UTILS_H

#include "GL/glew.h"

#include "image_io.h"

#include <vector>

class Utils
{
public:
	struct xorshift32_state {
		uint32_t a;
	};

	static uint32_t xorshift32(struct xorshift32_state* state);

	static void precompute_irradiance_map_from_skysphere_and_write(const char* skysphere_path, unsigned int samples, unsigned int downscale_factor, const char* output_irradiance_map_path);
	static Image precompute_irradiance_map_from_skysphere(const char* skysphere_path, unsigned int samples, unsigned int downscale_factor = 1);
    static Image precompute_and_load_associated_irradiance(const char* skysphere_file_path, unsigned int samples = 20, unsigned int downscale_factor = 1);

    static std::vector<ImageData> read_cubemap_data(const char* folder_name, const char* face_extension);
	static GLuint create_cubemap_texture_from_data(std::vector<ImageData>& faces_data);
	/*
	 * @param folder_name The folder containing the faces of the cubemap. The folder path should NOT contain a trailing '/'
	 * @param face_extension ".jpg", ".png", ....
	 * The faces of the cubemap should be named "right", "left", "top", "bottom", "front" and "back"
	 */
	static GLuint create_cubemap_texture_from_path(const char* folder_name, const char* face_extension);

	static void downscale_image(const Image& input_image, Image& downscaled_output, const int factor);

    static Image read_skysphere_image(const char* filename);
    static GLuint create_skysphere_texture_hdr(Image& skysphere_image, int texture_unit);
	static GLuint create_skysphere_texture_from_path(const char* filename, int texture_unit);
};

#endif
