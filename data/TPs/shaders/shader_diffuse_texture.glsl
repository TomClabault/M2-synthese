#version 330

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

uniform mat4 u_mvp_matrix;
uniform mat4 u_mlp_matrix;

out vec4 vs_position_light_space;
out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoords;

void main()
{
    gl_Position = u_mvp_matrix * vec4(position, 1);

    vs_normal = normal;
    vs_position = position;//Model transformation omitted
    vs_position_light_space = u_mlp_matrix * vec4(position, 1);
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
uniform sampler2D u_shadow_map;

in vec4 vs_position_light_space;
in vec3 vs_normal;
in vec3 vs_position;
in vec2 vs_texcoords;

float compute_shadow(vec4 light_space_fragment_position, vec3 normal, vec3 light_direction)
{

    vec3 projected_point = light_space_fragment_position.xyz / light_space_fragment_position.w;
    projected_point = projected_point * 0.5 + 0.5;

    float shadow_map_depth = texture(u_shadow_map, projected_point.xy).r;
    float scene_projected_depth = projected_point.z;
    if(scene_projected_depth > 1)
        return 1.0f;

    //Bias with a minimum of 0.005 for perpendicular angles. 0.05 for grazing angles
    float bias = max((1.0f - dot(normal, light_direction)) * 0.005, 0.001);

    return scene_projected_depth - bias > shadow_map_depth ? 0.0f : 1.0f;
}

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
    {
        gl_FragColor = vec4(1.0f);

        gl_FragColor = gl_FragColor * compute_shadow(vs_position_light_space, normalize(vs_normal), normalize(u_light_position - vs_position));
    }
}
#endif
