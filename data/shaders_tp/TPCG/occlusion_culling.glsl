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

layout(std430, binding = 0) buffer inputObjectIdsToCullBuffer
{
    uint input_cull_object_ids[];
};

layout(std430, binding = 1) buffer inputObjectsToCullBuffer
{
    CullObject input_cull_objects[];
};

layout(std430, binding = 2) buffer outputObjectIdsBuffer
{
    uint passing_object_ids[];
};

layout(std430, binding = 3) buffer outputDrawCommandsBuffer
{
    MultiDrawIndirectParam output_draw_commands[];
};

layout(std430, binding = 4) buffer nbObjectsDrawnBuffer
{
    uint nb_passing_objects;
};

// We have 2 samplers for the hierarchical z-buffer because the mipmap level 0
// is a depth texture, not a color texture as the levels 1, 2,3 , ... are so we
// cannot use them we the same sampler

// Only the very first mipmap level of the hierarchical z-buffer
uniform sampler2D u_z_buffer_mipmap0;
// All the other mipmaps (1, 2, 3, ......) of the hierarchical z-buffer
uniform sampler2D u_z_buffer_mipmaps1;

// How many mipmap levels we have in the hierarchical z buffer
uniform int u_nb_mipmaps;

uniform mat4 u_mvp_matrix;
uniform mat4 u_mvpv_matrix;
uniform mat4 u_view_matrix;

uniform int u_nb_objects_to_cull;

int get_visibility_of_object_from_camera(CullObject object)
{
    vec4 view_space_points_w[8];

    //TODO we only need the z coordinate so the whole matrix-point multiplication isn't needed, too slow
    view_space_points_w[0] = u_view_matrix * vec4(object.min, 1.0f);
    view_space_points_w[1] = u_view_matrix * vec4(object.max.x, object.min.y, object.min.z, 1.0f);
    view_space_points_w[2] = u_view_matrix * vec4(object.min.x, object.max.y, object.min.z, 1.0f);
    view_space_points_w[3] = u_view_matrix * vec4(object.max.x, object.max.y, object.min.z, 1.0f);
    view_space_points_w[4] = u_view_matrix * vec4(object.min.x, object.min.y, object.max.z, 1.0f);
    view_space_points_w[5] = u_view_matrix * vec4(object.max.x, object.min.y, object.max.z, 1.0f);
    view_space_points_w[6] = u_view_matrix * vec4(object.min.x, object.max.y, object.max.z, 1.0f);
    view_space_points_w[7] = u_view_matrix * vec4(object.max, 1.0f);

    vec3 view_space_points[8];
    for (int i = 0; i < 8; i++)
        view_space_points[i] = view_space_points_w[i].xyz / view_space_points_w[i].w;

    int all_behind = 1;
    int all_in_front = 1;
    for (int i = 0; i < 8; i++)
    {
        bool point_behind = view_space_points[i].z > 0;

        all_behind &= int(point_behind);
        all_in_front &= int(!point_behind);
    }

    if (all_behind == 1)
        return 0;
    else if (all_in_front == 1)
        return 1;
    else
        return 2;
}

void get_object_screen_space_bounding_box(CullObject object, out vec3 out_bbox_min, out vec3 out_bbox_max)
{
    vec4 object_screen_space_bbox_points_w[8];
    object_screen_space_bbox_points_w[0] = u_mvpv_matrix * vec4(object.min, 1.0f);
    object_screen_space_bbox_points_w[1] = u_mvpv_matrix * vec4(object.max.x, object.min.y, object.min.z, 1.0f);
    object_screen_space_bbox_points_w[2] = u_mvpv_matrix * vec4(object.min.x, object.max.y, object.min.z, 1.0f);
    object_screen_space_bbox_points_w[3] = u_mvpv_matrix * vec4(object.max.x, object.max.y, object.min.z, 1.0f);
    object_screen_space_bbox_points_w[4] = u_mvpv_matrix * vec4(object.min.x, object.min.y, object.max.z, 1.0f);
    object_screen_space_bbox_points_w[5] = u_mvpv_matrix * vec4(object.max.x, object.min.y, object.max.z, 1.0f);
    object_screen_space_bbox_points_w[6] = u_mvpv_matrix * vec4(object.min.x, object.max.y, object.max.z, 1.0f);
    object_screen_space_bbox_points_w[7] = u_mvpv_matrix * vec4(object.max, 1.0f);

    vec3 object_screen_space_bbox_points[8];
    for (int i = 0; i < 8; i++)
        object_screen_space_bbox_points[i] = object_screen_space_bbox_points_w[i].xyz / object_screen_space_bbox_points_w[i].w;

    out_bbox_min = object_screen_space_bbox_points[0];
    out_bbox_max = object_screen_space_bbox_points[1];
    for (int i = 0; i < 8; i++)
    {
        out_bbox_min = min(out_bbox_min, object_screen_space_bbox_points[i]);
        out_bbox_max = max(out_bbox_max, object_screen_space_bbox_points[i]);
    }
}

layout(local_size_x = 256) in;
void main()
{
    uint thread_id = gl_GlobalInvocationID.x;
    if (thread_id >= u_nb_objects_to_cull)
        return;

    uint object_id = input_cull_object_ids[thread_id];
    CullObject object = input_cull_objects[object_id];

    vec3 screen_space_bbox_min, screen_space_bbox_max;
    ivec2 z_buffer_mipmap_0_dims = textureSize(u_z_buffer_mipmap0, 0);
    int visibility = get_visibility_of_object_from_camera(object);
    if (visibility == 2) //Partially visible, we're going to assume
        //that the bounding box of the object spans the whole image
    {
        screen_space_bbox_min = vec3(0.0f, 0.0f, 0.0f);
        screen_space_bbox_max = vec3(z_buffer_mipmap_0_dims.x - 1, z_buffer_mipmap_0_dims.y - 1, 0.0f);
    }
    else if (visibility == 1) //Entirely visible
    {
        get_object_screen_space_bounding_box(object, screen_space_bbox_min, screen_space_bbox_max);

        //Clamping the points to the image limits
        screen_space_bbox_min = max(screen_space_bbox_min, vec3(0.0f, 0.0f, -1000000.0f));
        screen_space_bbox_max = min(screen_space_bbox_max, vec3(z_buffer_mipmap_0_dims.x - 1, z_buffer_mipmap_0_dims.y - 1, 1000000.0f));
    }
    else //Not visible
        return;

    //We're going to consider that all the pixels of the object are at the same depth,
    //this depth because the closest one to the camera
    //Because the closest depth is the biggest z, we're querrying the max point of the bbox
    float nearest_depth = screen_space_bbox_min.z;

    //Computing which mipmap level to choose for the depth test so that the
    //screens space bounding rectangle of the object is approximately 4x4
    int mipmap_level = 0;
    //Getting the biggest axis of the screen space bounding rectangle of the object
    float largest_extent = max(screen_space_bbox_max.x - screen_space_bbox_min.x, screen_space_bbox_max.y - screen_space_bbox_min.y);
    if (largest_extent > 4)
        //Computing the factor needed for the largest extent to be 16 pixels
        mipmap_level = int(log2(ceil(largest_extent / 4.0f)));
    else //The extent of the bounding rectangle already is small enough
        ;
    mipmap_level = min(mipmap_level, u_nb_mipmaps - 1);
    int reduction_factor = int(pow(2, mipmap_level));
    float reduction_factor_inverse = 1.0f / reduction_factor;

    ivec2 mipmap_dimensions = mipmap_level == 0 ? textureSize(u_z_buffer_mipmap0, mipmap_level) : textureSize(u_z_buffer_mipmaps1, mipmap_level - 1);
    vec2 half_mipmap_dudv = vec2(0.5f) / mipmap_dimensions;

    int min_y = min(int(floor(screen_space_bbox_min.y * reduction_factor_inverse)), mipmap_dimensions.y - 1);
    int max_y = min(int(ceil(screen_space_bbox_max.y * reduction_factor_inverse)), mipmap_dimensions.y - 1);
    int min_x = min(int(floor(screen_space_bbox_min.x * reduction_factor_inverse)), mipmap_dimensions.x - 1);
    int max_x = min(int(ceil(screen_space_bbox_max.x * reduction_factor_inverse)), mipmap_dimensions.x - 1);

    bool one_pixel_visible = false;
    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            float depth_buffer_depth;
            vec2 pixel_uv = vec2(x, y) / vec2(mipmap_dimensions);

            if (mipmap_level == 0)
                depth_buffer_depth = texelFetch(u_z_buffer_mipmap0, ivec2(x, y), 0).r;
            else
                depth_buffer_depth = texelFetch(u_z_buffer_mipmaps1, ivec2(x, y), mipmap_level - 1).r;

            if (depth_buffer_depth >= nearest_depth)
            {
                //The object needs to be rendered, we can stop here
                one_pixel_visible = true;

                break;
            }
        }

        if (one_pixel_visible)
            break;
    }

    if (one_pixel_visible)
    {
        uint index = atomicAdd(nb_passing_objects, 1);

        passing_object_ids[index] = object_id;
        output_draw_commands[index].vertex_count = object.vertex_count;
        output_draw_commands[index].vertex_base = object.vertex_base;
        output_draw_commands[index].instance_count = 1;
        output_draw_commands[index].instance_base = 0;
    }
}

#endif
