#version 330

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

uniform mat4 mvpMatrix;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoords;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);

    vs_normal = normal;
    vs_position = position;
    vs_texcoords = texcoords;
}
#endif

#ifdef FRAGMENT_SHADER

#define M_PI 3.1415926535897932384626433832795f

uniform vec3 u_camera_position;
uniform vec3 u_light_position;

uniform bool u_use_irradiance_map;
uniform vec4 u_ambient_color;

uniform sampler2D u_irradiance_map;
uniform sampler2D u_mesh_texture;

in vec3 vs_normal;
in vec3 vs_position;
in vec2 vs_texcoords;

void main()
{
    //	if (u_use_irradiance_map)
    //	{
    //		//This is the diffuse part. Because the diffuse part is based on a Lambertian model, we need to gather the light
    //		//around the normal of the surface (and not the reflect direction for example)
    //		float u = 0.5 + atan(vs_normal.z, vs_normal.x) / (2.0f * M_PI);
    //		float v = 0.5 + asin(vs_normal.y) / M_PI;

    //		gl_FragColor = u_diffuse_colors[vs_material_index] * texture(u_irradiance_map, vec2(u, v));
    //		gl_FragColor.a = 1.0f;
    //	}

    vec4 texture_color = texture(u_mesh_texture, vs_texcoords);
    if (texture_color.a == 0)
        discard;
    else
        gl_FragColor = texture_color;
    //gl_FragColor = vec4((vs_normal + 1) * 0.5, 1);
}
#endif
