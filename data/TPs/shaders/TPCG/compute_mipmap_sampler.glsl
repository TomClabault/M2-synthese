#version 430

#ifdef COMPUTE_SHADER

uniform sampler2D input_mipmap;

layout (binding = 1, r32f) uniform writeonly image2D output_mipmap;

layout(local_size_x = 16, local_size_y = 16) in;
void main()
{
    ivec2 thread_id = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    ivec2 output_mipmap_size = imageSize(output_mipmap);
    ivec2 input_mipmap_size = textureSize(input_mipmap, 0);
    if (thread_id.x >= output_mipmap_size.x || thread_id.y >= output_mipmap_size.y)
            return;

    vec2 dudv = vec2(1.0f) / input_mipmap_size;
    vec2 du = vec2(dudv.x, 0);
    vec2 dv = vec2(0, dudv.y);
    vec2 uv = vec2(thread_id) / input_mipmap_size;

    vec2 centering = (du + dv) / 2.0f;
    float first_texel_depth = texture(input_mipmap, uv * 2 + centering).r;
    float second_texel_depth = texture(input_mipmap, uv * 2 + du + centering).r;
    float third_texel_depth = texture(input_mipmap, uv * 2 + dv + centering).r;
    float fourth_texel_depth = texture(input_mipmap, uv * 2 + dudv + centering).r;
    float computed_mipmap_depth = max(max(max(first_texel_depth, second_texel_depth), third_texel_depth), fourth_texel_depth);

    imageStore(output_mipmap, thread_id, vec4(vec3(computed_mipmap_depth), 1.0f));
}

#endif
