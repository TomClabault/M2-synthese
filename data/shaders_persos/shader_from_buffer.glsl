#version 330

#ifdef VERTEX_SHADER

in vec3 position;

uniform mat4 mvpMatrix;

void main()
{
	gl_Position = mvpMatrix * vec4(position, 1);
}
#endif

#ifdef FRAGMENT_SHADER
void main()
{
	gl_FragColor = vec4(1, 1, 1, 1);
}
#endif