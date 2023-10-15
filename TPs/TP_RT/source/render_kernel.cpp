#include "render_kernel.h"

#include "triangle.h"

void RenderKernel::operator()(const sycl::nd_item<2>& coordinates) const
{
    int x = coordinates.get_global_id(0);
    int y = coordinates.get_global_id(1);

    ray_trace_pixel(x, y);
}

void branchlessONB(const Vector& n, Vector& b1, Vector& b2)
{
    float sign = sycl::copysign(1.0f, n.z);
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    b1 = Vector(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = Vector(b, sign + n.y * n.y * a, -n.y);
}

Vector RenderKernel::random_dir_hemisphere_around_normal(const Vector& normal, xorshift32_generator& random_number_generator) const
{
    Vector tangent, bitangent;
    branchlessONB(normal, tangent, bitangent);

    Transform ONB = Transform(tangent, bitangent, normal, Vector(0, 0, 0));

    float rand_1 = random_number_generator();
    float rand_2 = random_number_generator();

    float phi = 2.0f * M_PI * rand_1;
    float root = sycl::sqrt(1 - rand_2 * rand_2);

    Vector random_dir_world_space(sycl::cos(phi) * root, sycl::sin(phi) * root, rand_2);

    return ONB(random_dir_world_space);
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
    xorshift32_generator random_number_generator(x * y * SAMPLES_PER_KERNEL * (m_kernel_iteration + 1));

    Color final_color = Color(0.0f, 0.0f, 0.0f);
    for (int sample = 0; sample < SAMPLES_PER_KERNEL; sample++)
    {
        //Jittered around the center
        float x_jittered = (x + 0.5f) + random_number_generator() - 1.0f;
        float y_jittered = (y + 0.5f) + random_number_generator() - 1.0f;

        Ray ray = get_camera_ray(x_jittered, y_jittered);

        Color throughput = Color(1.0f, 1.0f, 1.0f);
        Color sample_color = Color(0.0f, 0.0f, 0.0f);
        RayState next_ray_state = RayState::BOUNCE;
        for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
        {
            if (next_ray_state == BOUNCE)
            {
                HitInfo closest_hit_info;
                bool intersection_found = intersect_scene(ray, closest_hit_info);

                if (intersection_found)
                {
                    // Direct lighting only
                    float pdf;
                    int emissive_triangle_index; //Will be used later if we're not in
                    //shadow to get the emission of the sampled emissive triangle
                    Point random_light_point = sample_random_point_on_lights(random_number_generator, pdf, emissive_triangle_index);
                    Point shadow_ray_origin = closest_hit_info.inter_point + closest_hit_info.normal_at_inter * 1.0e-4f;
                    Vector shadow_ray_direction = random_light_point - shadow_ray_origin;
                    float t_max = length(shadow_ray_direction);
                    Vector shadow_ray_direction_normalized = normalize(shadow_ray_direction);

                    Ray shadow_ray(shadow_ray_origin, shadow_ray_direction_normalized);

                    bool in_shadow = evaluate_shadow_ray(shadow_ray, t_max);

                    int triangle_material_index = m_materials_indices_buffer[closest_hit_info.triangle_index];
                    SimpleMaterial triangle_material = m_materials_buffer_access[triangle_material_index];

                    next_ray_state = RayState::TERMINATED;

                    if (in_shadow)
                    {
                        sample_color = Color(0.0f, 0.0f, 0.0f);

                        break;
                    }
                    else
                    {
                        const SimpleMaterial& emissive_triangle_material = m_materials_buffer_access[m_materials_indices_buffer[emissive_triangle_index]];

                        sample_color += emissive_triangle_material.emission;
                        //Lambertian
                        sample_color *= triangle_material.diffuse / M_PI;
                        //Cosine angle
                        sample_color *= sycl::max(dot(closest_hit_info.normal_at_inter, shadow_ray_direction_normalized), 0.0f);
                        //PDF
                        sample_color *= pdf;
                    }




                    //Indirect lighting
//                    int material_index = m_materials_indices_buffer[closest_hit_info.triangle_index];
//                    SimpleMaterial mat = m_materials_buffer_access[material_index];

//                    throughput *= mat.diffuse;
//                    sample_color += throughput * mat.emission;

//                    Vector random_dir = random_dir_hemisphere_around_normal(closest_hit_info.normal_at_inter, random_number_generator);
//                    Point new_ray_origin = closest_hit_info.inter_point + closest_hit_info.normal_at_inter * 1.0e-4f;

//                    ray = Ray(new_ray_origin, normalize(random_dir));
//                    next_ray_state = RayState::BOUNCE;
                }
                else
                    next_ray_state = RayState::MISSED;
            }
            else if (next_ray_state == MISSED)
            {
                //Handle skysphere here
                break;
            }
            else if (next_ray_state == TERMINATED)
                break;
        }

        final_color += sample_color;
    }

    final_color /= SAMPLES_PER_KERNEL;
    final_color.a = 0.0f;
    m_frame_buffer_access[y * m_width + x] += final_color;

    if (m_kernel_iteration == RENDER_KERNEL_ITERATIONS - 1)
        //Last iteration, computing the average
        m_frame_buffer_access[y * m_width + x] /= RENDER_KERNEL_ITERATIONS;
}

bool RenderKernel::intersect_scene(Ray& ray, HitInfo& closest_hit_info) const
{
    float closest_intersection_distance = -1;
    bool intersection_found = false;

    for (int i = 0; i < m_triangle_buffer_access.size(); i++)
    {
        const Triangle triangle = m_triangle_buffer_access[i];

        HitInfo hit_info;
        if (triangle.intersect(ray, hit_info))
        {
            if (hit_info.t < closest_intersection_distance || closest_intersection_distance == -1.0f)
            {
                hit_info.triangle_index = i;
                closest_intersection_distance = hit_info.t;
                closest_hit_info = hit_info;

                intersection_found = true;
            }
        }
    }

    return intersection_found;
}

Point RenderKernel::sample_random_point_on_lights(xorshift32_generator& random_number_generator, float& pdf, int& random_emissive_triangle_index) const
{
    random_emissive_triangle_index = random_number_generator() * m_emissive_triangle_indices_buffer.size();
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

    float triangle_area = length(cross(AB, AC)) / 2.0f;
    float nb_triangles = m_emissive_triangle_indices_buffer.size();
    pdf = 1 / (nb_triangles * triangle_area);

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
