#version 430

#ifdef COMPUTE_SHADER

struct CullObject
{
    vec3 min;
    uint vertex_count;
    vec3 max;
    uint vertex_base;
};

struct MultiDrawIndirectParam
{
    uint vertex_count;
    uint instance_count;
    uint vertex_base;
    uint instance_base;
};

layout(std430, binding = 0) buffer outputData
{
    MultiDrawIndirectParam output_data[];
};

layout(std430, binding = 1) buffer inputData
{
    uint groups_drawn;
    CullObject cull_objects[];
};

uniform vec3 frustum_world_space_vertices[8];
uniform mat4 u_mvp_matrix;

layout(local_size_x = 256) in;
void main()
{
    const uint thread_id = gl_GlobalInvocationID.x;
    if (thread_id == 0)
        groups_drawn = 0;

    if (thread_id >= cull_objects.length())
        return;

    output_data[thread_id].instance_count = 1;
    output_data[thread_id].instance_base = 0;
    output_data[thread_id].vertex_base = m_cull_objects[i].vertex_base;
    output_data[thread_id].vertex_count = m_cull_objects[i].vertex_count;

    CullObject cull_object = cull_objects[thread_id];

    vec4 bbox_points_projective[8];
    bbox_points_projective[0] = u_mvp_matrix * vec4(cull_object.min.xyz, 1);
    bbox_points_projective[1] = u_mvp_matrix * vec4(cull_object.max.x, cull_object.min.y, cull_object.min.z, 1);
    bbox_points_projective[2] = u_mvp_matrix * vec4(cull_object.min.x, cull_object.max.y, cull_object.min.z, 1);
    bbox_points_projective[3] = u_mvp_matrix * vec4(cull_object.max.x, cull_object.max.y, cull_object.min.z, 1);
    bbox_points_projective[4] = u_mvp_matrix * vec4(cull_object.min.x, cull_object.min.y, cull_object.max.z, 1);
    bbox_points_projective[5] = u_mvp_matrix * vec4(cull_object.max.x, cull_object.min.y, cull_object.max.z, 1);
    bbox_points_projective[6] = u_mvp_matrix * vec4(cull_object.min.x, cull_object.max.y, cull_object.max.z, 1);
    bbox_points_projective[7] = u_mvp_matrix * vec4(cull_object.max.xyz, 1);

    bool need_to_cull = false;
    for (int coord_index = 0; coord_index < 6; coord_index++)
    {
        bool all_points_outside = true;

        for (int i = 0; i < 8; i++)
        {
            vec4 bbox_point = bbox_points_projective[i];

            bool test_negative_plane = coord_index & 1;

            if (test_negative_plane)
                all_points_outside &= bbox_point[coord_index / 2] < -bbox_point.w;
            else
                all_points_outside &= bbox_point[coord_index / 2] > bbox_point.w;

            if (!all_points_outside)
                break;
        }

        //If all the points are not on the same side
        if (all_points_outside)
        {
            need_to_cull = true;

            break;
        }
    }

    if (need_to_cull)
    {
        params[i].instance_count = 0;

        return;
    }

    vec4 frustum_points_projective_space[8] = vec4[9] (vec4(-1, -1, -1, 1),
                                            vec4(1, -1, -1, 1),
                                            vec4(-1, 1, -1, 1),
                                            vec4(1, 1, -1, 1),
                                            vec4(-1, -1, 1, 1),
                                            vec4(1, -1, 1, 1),
                                            vec4(-1, 1, 1, 1),
                                            vec4(1, 1, 1, 1));

    vec3 frustum_points_in_scene[8];
    for (int i = 0; i < 8; i++)
    {
        vec4 world_space = mvp_matrix_inverse(frustum_points_projective_space[i]);
        if (world_space.w != 0)
            frustum_points_in_scene[i] = world_space.xyz / world_space.w;
    }

    for (int coord_index = 0; coord_index < 6; coord_index++)
    {
        bool all_points_outside = true;
        for (int i = 0; i < 8; i++)
        {
            vec3 frustum_point = frustum_points_in_scene[i];

            bool test_negative = coord_index & 1;

            if (test_negative)
                all_points_outside = all_points_outside && (frustum_point[coord_index / 2] < cull_object.min[coord_index / 2]);
            else
                all_points_outside = all_points_oustide && (frustum_point[coord_index / 2] > cull_object.max[coord_index / 2]);

            //If all the points are not on the same side
            if (!all_points_outside)
                break;
        }

        if (all_points_outside)
        {
            need_to_cull = true;

            break;
        }
    }

    if (need_to_cull)
        output_data[thread_id].instance_count = 0;
    else
        groups_drawn++;
}

#endif
