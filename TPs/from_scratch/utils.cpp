#include "utils.h"

#include <string>

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

ImageData Utils::read_skysphere_data(const char* filename)
{
	return read_image_data(filename);
}

GLuint Utils::create_skysphere_from_data(ImageData& skysphere_image_data)
{
	GLuint skysphere;
	glGenTextures(1, &skysphere);
	//Texture 1 because 0 is the cubemap
	glActiveTexture(GL_TEXTURE1);
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

GLuint Utils::create_skysphere_from_path(const char* filename)
{
	ImageData skysphere_image = read_skysphere_data(filename);

	return create_skysphere_from_data(skysphere_image);
}