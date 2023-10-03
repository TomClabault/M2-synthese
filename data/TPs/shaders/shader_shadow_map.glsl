#version 330

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;

uniform mat4 mlp_matrix;

void main()
{
	gl_Position = mlp_matrix * vec4(position, 1);
}
#endif
