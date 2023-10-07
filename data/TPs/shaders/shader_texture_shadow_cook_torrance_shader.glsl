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

const float M_PI = 3.1415926535897932384626433832795f;

uniform vec3 u_camera_position;
uniform vec3 u_light_position;

uniform bool u_use_irradiance_map;

uniform sampler2D u_irradiance_map;
uniform sampler2D u_mesh_base_color_texture;
uniform sampler2D u_mesh_specular_texture;
uniform sampler2D u_shadow_map;

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
            float shadow_map_depth = texture(shadow_map, texcoords + vec2(i, j) * texel_size).r;
            shadow_sum += scene_depth - bias > shadow_map_depth ? 0.5f : 1.0f;
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

    //Bias with a minimum of 0.005 for perpendicular angles. 0.05 for grazing angles
    float bias = max((1.0f - dot(normal, light_direction)) * 0.005, 0.001);
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

void main()
{
    vec3 ambient_irrandiance_color = vec3(0);
    if (u_use_irradiance_map)
    {
        float u = 0.5 + atan(vs_normal.z, vs_normal.x) / (2.0f * M_PI);
        float v = 0.5 + asin(vs_normal.y) / M_PI;

        ambient_irrandiance_color = texture2D(u_irradiance_map, vec2(u, v)).rgb;
    }

    vec4 base_color = texture2D(u_mesh_base_color_texture, vs_texcoords);
    //base_color = vec4(1.0, 0.71, 0.29, 1);

    //Handling transparency on the texture
    if (base_color.a == 0)
        discard;
    else
    {
        vec3 vs_normal_normalized = normalize(vs_normal);
        vec3 view_direction = normalize(u_camera_position - vs_position);
        vec3 light_direction = normalize(u_light_position - vs_position);
        vec3 halfway_vector = normalize(view_direction + light_direction);

        float NoV = max(0.0f, dot(vs_normal_normalized, view_direction));
        float NoL = max(0.0f, dot(vs_normal_normalized, light_direction));
        float NoH = max(0.0f, dot(vs_normal_normalized, halfway_vector));
        float VoH = max(0.0f, dot(vs_normal, halfway_vector));

        float metalness = texture2D(u_mesh_specular_texture, vs_texcoords).b;
        float roughness = texture2D(u_mesh_specular_texture, vs_texcoords).g;
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

        vec3 light_color = vec3(2.0f);
        gl_FragColor = vec4((diffuse_part + specular_part) * light_color * ambient_irrandiance_color, 1);
        gl_FragColor = gl_FragColor * compute_shadow(vs_position_light_space, normalize(vs_normal), normalize(u_light_position - vs_position));
    }
}

/*
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 * RECIPROCAL_PI / (b * b);
}

float G1_GGX_Schlick(float NoV, float roughness) {
  float alpha = roughness * roughness;
  float k = alpha / 2.0;
  return max(NoV, 0.001) / (NoV * (1.0 - k) + k);
}

float G_Smith(float NoV, float NoL, float roughness) {
  return G1_GGX_Schlick(NoL, roughness) * G1_GGX_Schlick(NoV, roughness);
}

float fresnelSchlick90(float cosTheta, float F0, float F90) {
  return F0 + (F90 - F0) * pow(1.0 - cosTheta, 5.0);
}

float disneyDiffuseFactor(float NoV, float NoL, float VoH, float roughness) {
  float alpha = roughness * roughness;
  float F90 = 0.5 + 2.0 * alpha * VoH * VoH;
  float F_in = fresnelSchlick90(NoL, 1.0, F90);
  float F_out = fresnelSchlick90(NoV, 1.0, F90);
  return F_in * F_out;
}

vec3 brdfMicrofacet(in vec3 L, in vec3 V, in vec3 N,
                        in float metallic, in float roughness, in vec3 baseColor, in float reflectance) {

  vec3 H = normalize(V + L);

  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);

  vec3 f0 = vec3(0.16 * (reflectance * reflectance));
  f0 = mix(f0, baseColor, metallic);

  vec3 F = fresnelSchlick(VoH, f0);
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);

  vec3 spec = (F * D * G) / (4.0 * max(NoV, 0.001) * max(NoL, 0.001));

  vec3 rhoD = baseColor;

  // optionally
  rhoD *= vec3(1.0) - F;
  // rhoD *= disneyDiffuseFactor(NoV, NoL, VoH, roughness);

  rhoD *= (1.0 - metallic);

  vec3 diff = rhoD * RECIPROCAL_PI;

  return diff + spec;


}
*/
#endif
