#include <iostream>
#include <sycl/sycl.hpp>
#include <chrono>
#include <cmath>

#include <rapidobj/rapidobj.hpp>

#include "camera.h"
#include "image_io.h"
#include "render_kernel.h"
#include "tests.h"
#include "triangle.h"

int main(int argc, char* argv[])
{
    regression_tests();

    const int width = 1280;
    const int height = 720;

    sycl::queue queue;
    std::cout << "Using " << queue.get_device().get_info<sycl::info::device::name>() << std::endl;

    Image image(width, height);

    //rapidobj::Result parsed_obj = rapidobj::ParseFile("../../../data/simple_plane.obj", rapidobj::MaterialLibrary::Default());
    rapidobj::Result parsed_obj = rapidobj::ParseFile("../../../data/cornell.obj", rapidobj::MaterialLibrary::Default());
    if (parsed_obj.error)
    {
        std::cout << "There was an error loading the OBJ file: " << parsed_obj.error.code.message() << std::endl;

        return -1;
    }
    rapidobj::Triangulate(parsed_obj);

    const rapidobj::Array<float>& positions = parsed_obj.attributes.positions;
    std::vector<Triangle> triangle_host_buffer;
    std::vector<Triangle> emissive_triangle_host_buffer;
    for (rapidobj::Shape& shape : parsed_obj.shapes)
    {
        rapidobj::Mesh& mesh = shape.mesh;
        for (int i = 0; i < mesh.indices.size(); i += 3)
        {
            int index_0 = mesh.indices[i + 0].position_index;
            int index_1 = mesh.indices[i + 1].position_index;
            int index_2 = mesh.indices[i + 2].position_index;

            Point A = Point(positions[index_0 * 3 + 0], positions[index_0 * 3 + 1], positions[index_0 * 3 + 2]);
            Point B = Point(positions[index_1 * 3 + 0], positions[index_1 * 3 + 1], positions[index_1 * 3 + 2]);
            Point C = Point(positions[index_2 * 3 + 0], positions[index_2 * 3 + 1], positions[index_2 * 3 + 2]);

            Triangle triangle(A, B, C);
            triangle_host_buffer.push_back(triangle);

            int mesh_triangle_index = i / 3;
            int material_index = mesh.material_ids[mesh_triangle_index];
            rapidobj::Float3 emission = parsed_obj.materials[material_index].emission;
            if (emission[0] > 0 || emission[1] > 0 || emission[2] > 0)
            {
                //This is an emissive triangle
                emissive_triangle_host_buffer.push_back(triangle);
            }
        }
    }

    sycl::buffer<Color> image_buffer(image.color_data(), image.width() * image.height());
    sycl::buffer<Triangle> triangle_buffer(triangle_host_buffer.data(), triangle_host_buffer.size());
    sycl::buffer<Triangle> emissive_triangle_buffer(emissive_triangle_host_buffer.data(), emissive_triangle_host_buffer.size());

    auto start = std::chrono::high_resolution_clock::now();
    queue.submit([&] (sycl::handler& handler) {
        auto image_buffer_access = image_buffer.get_access<sycl::access::mode::write, sycl::access::target::device>(handler);
        auto triangle_buffer_access = triangle_buffer.get_access<sycl::access::mode::read>(handler);
        auto emissive_triangle_buffer_access = emissive_triangle_buffer.get_access<sycl::access::mode::read>(handler);

        const auto global_range = sycl::range<2>(width, height);
        const auto local_range = sycl::range<2>(TILE_SIZE_X, TILE_SIZE_Y);
        const auto coordinates_indices = sycl::nd_range<2>(global_range, local_range);

        sycl::stream debug_out_stream(16384, 128, handler);

        auto render_kernel = RenderKernel(width, height,
                                          image_buffer_access,
                                          triangle_buffer_access,
                                          emissive_triangle_buffer_access,
                                          debug_out_stream);
        render_kernel.set_camera(Camera(45, Translation(0, 1, 3.5)));

        handler.parallel_for(coordinates_indices, render_kernel);
    }).wait();
    image_buffer.get_access<sycl::access::mode::read>();
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

    write_image_png(image, "../output.png");

    return 0;
}
