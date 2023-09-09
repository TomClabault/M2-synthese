#ifndef UTILS_H
#define UTILS_H

#include "GL/glew.h"

#include "image_io.h"

#include <vector>

class Utils
{
public:

	static std::vector<ImageData> read_cubemap_data(const char* folder_name, const char* face_extension);
	static GLuint create_cubemap_from_data(std::vector<ImageData>& faces_data);
	/*
	 * @param folder_name The folder containing the faces of the cubemap. The folder path should NOT contain a trailing '/'
	 * @param face_extension ".jpg", ".png", ....
	 * The faces of the cubemap should be named "right", "left", "top", "bottom", "front" and "back"
	 */
	static GLuint create_cubemap_from_path(const char* folder_name, const char* face_extension);

	static ImageData read_skysphere_data(const char* filename);
	static GLuint create_skysphere_from_data(ImageData& skysphere_image_data);
	static GLuint create_skysphere_from_path(const char* filename);
};

#endif
