#version 430

#ifdef COMPUTE_SHADER

layout (binding = 0, r32f) uniform readonly image2D input_mipmap;
layout (binding = 1, r32f) uniform writeonly image2D output_mipmap;

layout(local_size_x = 16, local_size_y = 16) in;
void main()
{
    ivec2 thread_id = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    ivec2 output_mipmap_size = imageSize(output_mipmap);
    if (thread_id.x >= output_mipmap_size.x || thread_id.y >= output_mipmap_size.y)
            return;

    float first_texel_depth = imageLoad(input_mipmap, thread_id * 2).r;
    float second_texel_depth = imageLoad(input_mipmap, thread_id * 2 + ivec2(1, 0)).r;
    float third_texel_depth = imageLoad(input_mipmap, thread_id * 2 + ivec2(0, 1)).r;
    float fourth_texel_depth = imageLoad(input_mipmap, thread_id * 2 + ivec2(1, 1)).r;
    float computed_mipmap_depth = max(max(max(first_texel_depth, second_texel_depth), third_texel_depth), fourth_texel_depth);

    imageStore(output_mipmap, thread_id, vec4(vec3(computed_mipmap_depth), 1.0f));
}

#endif
