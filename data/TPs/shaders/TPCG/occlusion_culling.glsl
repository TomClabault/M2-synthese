#version 430

#ifdef COMPUTE_SHADER

struct CullObject
{
    vec3 min;
    uint vertex_base;
    vec3 max;
    uint vertex_count;
};

struct MultiDrawIndirectParam
{
    uint vertex_count;
    uint instance_count;
    uint vertex_base;
    uint instance_base;
};

layout(std430, binding = 0) buffer outputDrawParams
{
    MultiDrawIndirectParam output_draw_params[];
};

layout(std430, binding = 1) buffer outputDrawnObjects
{
    uint output_objects_drawn_id[];
};

layout(std430, binding = 2) buffer inputData
{
    CullObject cull_objects[];
};

layout(std430, binding = 3) buffer parameters
{
    uint groups_drawn;
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

    for (int coord_index = 0; coord_index < 6; coord_index++)
    {
        bool all_points_outside = true;

        for (int i = 0; i < 8; i++)
        {
            vec4 bbox_point = bbox_points_projective[i];

            int test_negative_plane = coord_index & 1;

            if (test_negative_plane == 1)
                all_points_outside = all_points_outside && (bbox_point[coord_index / 2] < -bbox_point.w);
            else
                all_points_outside = all_points_outside && (bbox_point[coord_index / 2] > bbox_point.w);

            if (!all_points_outside)
                break;
        }

        //If all the points are on the same side
        if (all_points_outside)
            return;
    }

    for (int coord_index = 0; coord_index < 6; coord_index++)
    {
        bool all_points_outside = true;
        for (int i = 0; i < 8; i++)
        {
            vec3 frustum_point = frustum_world_space_vertices[i];

            int test_negative = coord_index & 1;

            if (test_negative == 1)
                all_points_outside = all_points_outside && (frustum_point[coord_index / 2] < cull_object.min[coord_index / 2]);
            else
                all_points_outside = all_points_outside && (frustum_point[coord_index / 2] > cull_object.max[coord_index / 2]);

            //If all the points are not on the same side
            if (!all_points_outside)
                break;
        }

        if (all_points_outside)
            return;
    }

    uint index = atomicAdd(groups_drawn, 1);

    output_draw_params[index].vertex_count = cull_objects[thread_id].vertex_count;
    output_draw_params[index].vertex_base = cull_objects[thread_id].vertex_base;
    output_draw_params[index].instance_count = 1;
    output_draw_params[index].instance_base = 0;

    output_objects_drawn_id[index] = thread_id;
}

#endif
