#version 330

#ifdef VERTEX_SHADER

in vec3 position;
in vec3 normal;
in uint material_index;

uniform mat4 mvpMatrix;

out vec3 vs_normal;
out vec3 vs_position;
flat out uint vs_material_index;

void main()
{
	gl_Position = mvpMatrix * vec4(position, 1);

	vs_normal = normal;
	vs_position = position;
	vs_material_index = material_index;
}
#endif

#ifdef FRAGMENT_SHADER

uniform vec3 u_light_position;
uniform vec4 u_diffuse_colors[16];
uniform bool u_use_ambient;
uniform vec4 u_ambient_color;

in vec3 vs_normal;
in vec3 vs_position;
flat in uint vs_material_index;

void main()
{
	gl_FragColor = max(0.0f, dot(vs_normal, normalize(u_light_position - vs_position))) * u_diffuse_colors[vs_material_index];

	if (u_use_ambient)
		gl_FragColor = gl_FragColor + u_ambient_color;
}
#endif