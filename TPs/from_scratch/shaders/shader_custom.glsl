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

#define M_PI 3.1415926535897932384626433832795f

uniform vec3 u_camera_position;
uniform vec3 u_light_position;
uniform vec4 u_diffuse_colors[16];
uniform bool u_use_irradiance_map;
uniform vec4 u_ambient_color;

uniform sampler2D u_irradiance_map;

in vec3 vs_normal;
in vec3 vs_position;
flat in uint vs_material_index;

void main()
{
	if (u_use_irradiance_map)
	{
		//This is the diffuse part. Because the diffuse part is based on a Lambertian model, we need to gather the light
		//around the normal of the surface (and not the reflect direction for example)
		float u = 0.5 + atan(vs_normal.z, vs_normal.x) / (2.0f * M_PI);
		float v = 0.5 + asin(vs_normal.y) / M_PI;

		gl_FragColor = u_diffuse_colors[vs_material_index] * texture(u_irradiance_map, vec2(u, v));
		gl_FragColor.a = 1.0f;
	}
	else
	{
		//The diffuse part if not using the irradiance map is the Lambertian approximation:
		//dot(normal, light_position)
		gl_FragColor = max(0.0f, dot(vs_normal, normalize(u_light_position - vs_position))) * u_diffuse_colors[vs_material_index];
	}
}
#endif