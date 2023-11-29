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
        groups_drawn = cull_objects.length();

    if (thread_id >= cull_objects.length())
        return;

    //The object will be drawn by default
    output_data[thread_id].instance_count = 1;

    CullObject cull_object = cull_objects[thread_id];

    /*
          6--------7
         /|       /|
        / |      / |
       2--------3  |
       |  |     |  |
       |  4-----|--5
       |  /     | /
       | /      |/
       0--------1
    */
    vec4 object_bbox_vertices[8];
    object_bbox_vertices[0] = vec4(cull_object.min, 1);
    object_bbox_vertices[1] = vec4(cull_object.max.x, cull_object.min.y, cull_object.min.z, 1);
    object_bbox_vertices[2] = vec4(cull_object.min.x, cull_object.max.y, cull_object.min.z, 1);
    object_bbox_vertices[3] = vec4(cull_object.max.x, cull_object.max.y, cull_object.min.z, 1);
    object_bbox_vertices[4] = vec4(cull_object.min.x, cull_object.min.y, cull_object.max.z, 1);
    object_bbox_vertices[5] = vec4(cull_object.max.x, cull_object.min.y, cull_object.max.z, 1);
    object_bbox_vertices[6] = vec4(cull_object.min.x, cull_object.max.y, cull_object.max.z, 1);
    object_bbox_vertices[7] = vec4(cull_object.max, 1);

    vec4 object_bounds_vertices_projective[8];
    for (int i = 0; i < 8; i++)
        object_bounds_vertices_projective[i] = u_mvp_matrix * object_bbox_vertices[i];

    bool all_outside = true;
    for (int i = 0; i < 8; i++)
    {
        vec4 object_bounds_vertex = object_bounds_vertices_projective[i];
        vec4 object_w_bound_min = vec4(-object_bounds_vertex.w);
        vec4 object_w_bound_max = vec4(object_bounds_vertex.w);

        all_outside = all_outside 
            && all(greaterThan(object_bounds_vertex, object_w_bound_max)) 
            && all(lessThan(object_bounds_vertex, object_w_bound_min));
        if (!all_outside)
            break; //We found a vertex of the bounding box that isn't separated from the frustum in projective
            //space so we may have to draw this object, we're going to have to do the second test in world space
    }

    if (all_outside)
    {
        //The two bounding boxes are separated, not the drawing the object

        output_data[thread_id].instance_count = 0;
        return;
    }

    //Testing the bounding box of the object against the non square frustum in world space
    for (int i = 0; i < 8; i++)
    {
        all_outside = all_outside 
            && all(greaterThan(frustum_world_space_vertices[i], cull_object.max))
            && all(lessThan(frustum_world_space_vertices[i], cull_object.min));

        if (!all_outside)
            return; //We found a vertex of the bounding box of the object that is inside the view frustum in world space
            //so we're going to have to draw the object
    }

    //If we arrived here, the object will not be drawn
    atomicAdd(groups_drawn, -1);
    output_data[thread_id].instance_count = 0;
}

#endif
