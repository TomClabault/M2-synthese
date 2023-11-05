#version 330

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

uniform mat4 u_model_matrix;
uniform mat4 u_vp_matrix;
uniform mat4 u_lp_matrix;

out mat4 vs_model_matrix;
out vec4 vs_position_light_space;
out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoords;

void main()
{
    gl_Position = u_vp_matrix * u_model_matrix * vec4(position, 1.0f);

    vs_normal = vec3(u_model_matrix * vec4(normal, 0.0f));
    vs_normal = normalize(vec3(transpose(inverse(u_model_matrix)) * vec4(normal, 0.0f)));
    vs_position = vec3(u_model_matrix * vec4(position, 1.0f));
    vs_texcoords = texcoords;

    vs_position_light_space = u_lp_matrix * u_model_matrix * vec4(position, 1);

    vs_model_matrix = u_model_matrix;
}
#endif

#ifdef FRAGMENT_SHADER

const float M_PI = 3.1415926535897932384626433832795f;

uniform vec3 u_camera_position;
uniform vec3 u_light_position;

uniform bool u_use_irradiance_map;
uniform bool u_has_normal_map;

uniform sampler2D u_irradiance_map;
uniform sampler2D u_mesh_base_color_texture;
uniform sampler2D u_mesh_specular_texture;
uniform sampler2D u_mesh_normal_map;
uniform sampler2D u_shadow_map;
uniform float u_shadow_intensity;

uniform bool u_override_material;
uniform float u_metalness;
uniform float u_roughness;

in mat4 vs_model_matrix;
in vec4 vs_position_light_space;
in vec3 vs_normal;
in vec3 vs_position;
in vec2 vs_texcoords;

float percentage_closer_filtering(sampler2D shadow_map, vec2 texcoords, float scene_depth, float bias)
{
    ivec2 texture_size = textureSize(shadow_map, 0);
    vec2 texel_size = 1.0f / texture_size;

    float shadow_sum = 0.0f;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            float shadow_map_depth = texture2D(shadow_map, texcoords + vec2(i, j) * texel_size).r;
            shadow_sum += scene_depth - bias > shadow_map_depth ? u_shadow_intensity : 1.0f;
        }
    }

    return shadow_sum / 9.0f;
}

float compute_shadow(vec4 light_space_fragment_position, vec3 normal, vec3 light_direction)
{

    vec3 projected_point = light_space_fragment_position.xyz / light_space_fragment_position.w;
    projected_point = projected_point * 0.5 + 0.5;

    float scene_projected_depth = projected_point.z;
    if(scene_projected_depth > 1)
        return 1.0f;

    //Bias with a minimum of 0.001 for perpendicular angles. 0.002 for grazing angles
    float bias = max((1.0f - dot(normal, light_direction)) * 0.002, 0.001);
    float shadow_map_depth = percentage_closer_filtering(u_shadow_map, projected_point.xy, scene_projected_depth, bias);

    return shadow_map_depth;
}

vec3 fresnel_schlick(vec3 F0, float NoV)
{
    return F0 + (1.0f - F0) * pow((1.0f - NoV), 5.0f);
}

float GGX_normal_distribution(float alpha, float NoH)
{
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float b = (NoH2 * (alpha2 - 1.0) + 1.0);
    return alpha2 * (1 / M_PI) / (b * b);
}

float G1_schlick_ggx(float k, float dot_prod)
{
    return dot_prod / (dot_prod * (1.0f - k) + k);
}

float GGX_smith_masking_shadowing(float roughness_squared, float NoV, float NoL)
{
    float k = roughness_squared / 2;

    return G1_schlick_ggx(k, NoL) * G1_schlick_ggx(k, NoV);
}

vec3 sample_irradiance_map(vec3 normal)
{
    if (u_use_irradiance_map)
    {
        float u = 0.5 + atan(vs_normal.z, vs_normal.x) / (2.0f * M_PI);
        float v = 0.5 + asin(vs_normal.y) / M_PI;

        return texture2D(u_irradiance_map, vec2(u, v)).rgb;
    }
    else
        return vec3(0.0f);
}

void branchlessONB(in vec3 n, out vec3 tangent, out vec3 bitangent)
{
    float nz_sign;
    if (n.z < 0)
        nz_sign = -1.0f;
    else
        nz_sign = 1.0f;
    //float nz_sign = sign(n.z);
    float a = -1.0f / (nz_sign + n.z);
    float b = n.x * n.y * a;

    tangent = vec3(1.0f + nz_sign * n.x * n.x * a, nz_sign * b, -nz_sign * n.x);
    bitangent = vec3(b, nz_sign + n.y * n.y * a, -n.y);
}

vec3 normal_mapping(vec2 normal_map_uv)
{
    //Building the ONB around the surface normal
    vec3 tangent, bitangent;

    branchlessONB(vs_normal, tangent, bitangent);

    mat3 ONB = mat3(normalize(tangent),
                    normalize(bitangent),
                    normalize(vs_normal));

    vec3 texture_normal = normalize(texture2D(u_mesh_normal_map, normal_map_uv).rgb * 2.0f - 1.0f);
    return ONB * texture_normal;
}

void main()
{
    vec4 base_color = texture2D(u_mesh_base_color_texture, vs_texcoords);
    vec3 irradiance_map_color;
    //base_color = vec4(1.0, 0.71, 0.29, 1); //Hardcoded gold color

    vec3 surface_normal = vs_normal;
    if (u_has_normal_map)
        surface_normal = normal_mapping(vs_texcoords);
    gl_FragColor = vec4(surface_normal, 0.0f);
    return;

    //Handling transparency on the texture
    if (base_color.a < 0.5)
        discard;
    else
    {
        vec3 surface_normal_normalized = normalize(surface_normal);
        vec3 view_direction = normalize(u_camera_position - vs_position);
        vec3 light_direction = normalize(u_light_position - vs_position);
        vec3 halfway_vector = normalize(view_direction + light_direction);

        float NoV = max(0.0f, dot(surface_normal_normalized, view_direction));
        float NoL = max(0.0f, dot(surface_normal_normalized, light_direction));
        float NoH = max(0.0f, dot(surface_normal_normalized, halfway_vector));
        float VoH = max(0.0f, dot(halfway_vector, view_direction));

        if (NoV > 0 && NoL > 0 && NoH > 0)
        {
            float metalness = texture2D(u_mesh_specular_texture, vs_texcoords).b;
            float roughness = texture2D(u_mesh_specular_texture, vs_texcoords).g;

            if (u_override_material)
            {
                metalness = u_metalness;
                roughness = u_roughness;
            }

            float alpha = roughness * roughness;

            ////////// Cook Torrance BRDF //////////
            vec3 F;
            float D, G;

            //F0 = 0.04 for dielectrics, 1.0 for metals (approximation)
            vec3 F0 = 0.04f * (1.0f - metalness) + metalness * base_color.rgb;

            //GGX Distribution function
            F = fresnel_schlick(F0, VoH);
            D = GGX_normal_distribution(alpha, NoH);
            G = GGX_smith_masking_shadowing(alpha, NoV, NoL);

            vec3 kD = vec3(1.0f - metalness); //Metals do not have a diffuse part
            kD *= 1.0f - F;//Only the transmitted light is diffused

            vec3 diffuse_part = kD * base_color.rgb / M_PI;
            vec3 specular_part = (F * D * G) / (4.0f * NoV * NoL);

            gl_FragColor = vec4(diffuse_part + specular_part, 1);

            // Sampling the irradiance map with the microfacet normal
            irradiance_map_color = sample_irradiance_map(halfway_vector);
        }
        else
        {
            gl_FragColor = vec4(0, 0, 0, 1);
            irradiance_map_color = sample_irradiance_map(surface_normal);
        }
    }

    gl_FragColor += vec4(base_color.rgb * irradiance_map_color, 0);// Ambient lighting
    gl_FragColor *= compute_shadow(vs_position_light_space, normalize(surface_normal), normalize(u_light_position - vs_position));
    //gl_FragColor.a = 1.0f;
}

#endif
