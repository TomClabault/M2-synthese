#include "render_kernel.h"

#include "triangle.h"

void RenderKernel::operator()(const sycl::nd_item<2>& coordinates) const
{
    int x = coordinates.get_global_id(0);
    int y = coordinates.get_global_id(1);

    ray_trace_pixel(x, y);
}

Ray RenderKernel::get_camera_ray(float x, float y) const
{
    float x_ndc_space = x / m_width * 2 - 1;
    x_ndc_space *= (float)m_width / m_height; //Aspect ratio
    float y_ndc_space = y / m_height * 2 - 1;


    Point ray_origin_view_space(0, 0, 0);
    Point ray_origin = m_camera.view_matrix(ray_origin_view_space);

    Point ray_point_direction_ndc_space = Point(x_ndc_space, y_ndc_space, m_camera.fov_dist);
    Point ray_point_direction_world_space = m_camera.view_matrix(ray_point_direction_ndc_space);

    Vector ray_direction = normalize(ray_point_direction_world_space - ray_origin);
    Ray ray(ray_origin, ray_direction);

    return ray;
}

void RenderKernel::ray_trace_pixel(int x, int y) const
{
    xorshift32_generator random_number_generator(x * y * SAMPLES);

    Color final_color = Color(0.0f, 0.0f, 0.0f);
    for (int sample = 0; sample < SAMPLES; sample++)
    {
        //Jittered around the center
        float x_jittered = (x + 0.5f) + random_number_generator() - 1.0f;
        float y_jittered = (y + 0.5f) + random_number_generator() - 1.0f;

        Ray ray = get_camera_ray(x_jittered, y_jittered);

        Color sample_color;
        for (int bounce = 0; bounce < 1; bounce++)
        {
            HitInfo closest_hit_info;
            bool intersection_found = intersect_scene(ray, closest_hit_info);

            if (intersection_found)
            {
                Point random_light_point = sample_random_point_on_lights(random_number_generator);
                Point shadow_ray_origin = closest_hit_info.inter_point + closest_hit_info.normal_at_inter * 1.0e-4f;
                Vector shadow_ray_direction = random_light_point - shadow_ray_origin;
                float t_max = length(shadow_ray_direction);

                Ray shadow_ray(shadow_ray_origin, normalize(shadow_ray_direction));

                bool in_shadow = evaluate_shadow_ray(shadow_ray, t_max);
                sample_color = in_shadow ? Color(0.0f, 0.0f, 0.0f) : Color(1.0f, 1.0f, 1.0f);
            }
        }

        final_color += sample_color;
    }

    final_color /= SAMPLES;
    final_color.a = 1.0f;
    m_frame_buffer_access[y * m_width + x] = final_color;
}

bool RenderKernel::intersect_scene(Ray& ray, HitInfo& closest_hit_info) const
{
    float closest_intersection_distance = -1;
    bool intersection_found = false;

    for (const Triangle triangle : m_triangle_buffer_access)
    {
        HitInfo hit_info;
        if (triangle.intersect(ray, hit_info))
        {
            if (hit_info.t < closest_intersection_distance || closest_intersection_distance == -1.0f)
            {
                closest_intersection_distance = hit_info.t;
                closest_hit_info = hit_info;

                intersection_found = true;
            }
        }
    }

    return intersection_found;
}

Point RenderKernel::sample_random_point_on_lights(xorshift32_generator& random_number_generator) const
{
    int random_emissive_triangle_index = random_number_generator() * m_emissive_triangle_indices_buffer.size();
    random_emissive_triangle_index = m_emissive_triangle_indices_buffer[random_emissive_triangle_index];
    Triangle random_emissive_triangle = m_triangle_buffer_access[random_emissive_triangle_index];

    float rand_1 = random_number_generator();
    float rand_2 = random_number_generator();

    float sqrt_r1 = sycl::sqrt(rand_1);
    float u = 1.0f - sqrt_r1;
    float v = (1.0f - rand_2) * sqrt_r1;

    Vector AB = random_emissive_triangle.m_b - random_emissive_triangle.m_a;
    Vector AC = random_emissive_triangle.m_c - random_emissive_triangle.m_a;

    Point random_point_on_triangle = random_emissive_triangle.m_a + AB * u + AC * v;

    return random_point_on_triangle;
}

bool RenderKernel::evaluate_shadow_ray(Ray& ray, float t_max) const
{
    HitInfo hit_info;
    intersect_scene(ray, hit_info);
    if (hit_info.t + 1.0e-4f < t_max)
    {
        //There is something in between the light and the origin of the ray
        return true;
    }
    else
        return false;

}
