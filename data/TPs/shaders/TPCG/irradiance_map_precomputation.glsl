#version 430

#ifdef COMPUTE_SHADER

uniform float u_mipmap_level;
uniform int u_iteration;
uniform int u_sample_count;
uniform int u_total_sample_count;//Used to accumulate several passes of the compute shader
uniform sampler2D hdr_skysphere_input;

layout (binding = 1, rgba32f) uniform coherent image2D irradiance_map_output;

#define M_PI 3.1415926535897932384626433832795

void branchlessONB(in vec3 n, out vec3 tangent, out vec3 bitangent)
{
    float nz_sign;
    if (n.z < 0)
        nz_sign = -1.0f;
    else
        nz_sign = 1.0f;

    float a = -1.0f / (nz_sign + n.z);
    float b = n.x * n.y * a;

    tangent = vec3(1.0f + nz_sign * n.x * n.x * a, nz_sign * b, -nz_sign * n.x);
    bitangent = vec3(b, nz_sign + n.y * n.y * a, -n.y);
}

struct random_state
{
    uint seed;
};

//bias: 0.1735335599958158
float hash(inout random_state state)
{
    const uint UINT_MAX = 4294967295u; // Maximum value of a 32-bit unsigned integer

    state.seed ^= state.seed >> 16;
    state.seed *= 0x7feb352dU;
    state.seed ^= state.seed >> 15;
    state.seed *= 0x846ca68bU;
    state.seed ^= state.seed >> 16;

    return state.seed / float(UINT_MAX);
}

layout(local_size_x = 16, local_size_y = 16) in;
void main()
{
    ivec2 thread_id = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    ivec2 image_size = imageSize(irradiance_map_output);
    if (thread_id.x > image_size.x || thread_id.y > image_size.y)
            return;

    vec2 uv = vec2(thread_id) / image_size;

    float theta = M_PI * (1.0f - uv.y);
    float phi = 2.0f * M_PI * (0.5f - uv.x);

    //The main direction we're going to randomly sample the skysphere around
    vec3 normal = normalize(vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)));

    //Calculating the vectors of the basis we're going to use to rotate the randomly generated vector
    //around our main direction
    vec3 tangent, bitangent;

    branchlessONB(normal, tangent, bitangent);
    mat3 ONB = mat3(normalize(tangent),
                    normalize(bitangent),
                    normalize(normal));

    //Init random
    random_state random;
    random.seed = (thread_id.x + thread_id.y * image_size.x) * (u_iteration + 1);

    vec3 sum = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < u_sample_count; i++)
    {
        float rand1 = hash(random);
        float rand2 = hash(random);

        float phi_rand = 2.0f * M_PI * rand1;
        float theta_rand = asin(sqrt(rand2));

        vec3 random_direction_in_canonical_hemisphere = normalize(vec3(cos(phi_rand) * sin(theta_rand), sin(phi_rand) * sin(theta_rand), cos(theta_rand)));

        vec3 random_direction_in_hemisphere_around_normal = ONB * random_direction_in_canonical_hemisphere;
        vec3 random_direction_rotated = random_direction_in_hemisphere_around_normal;

        vec2 uv_random = vec2(0.5f - atan(random_direction_rotated.y, random_direction_rotated.x) / (2.0f * M_PI),
            1.0f - acos(random_direction_rotated.z) / M_PI);

        vec3 sample_color = textureLod(hdr_skysphere_input, uv_random, u_mipmap_level).rgb;
        sum = sum + sample_color;
    }

    vec3 current_color = imageLoad(irradiance_map_output, thread_id).rgb;
    imageStore(irradiance_map_output, thread_id, vec4(sum / vec3(u_total_sample_count) + current_color, 1.0f));
}

#endif
