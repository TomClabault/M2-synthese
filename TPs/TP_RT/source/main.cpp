#include <iostream>
#include <CL/sycl.hpp>
#include <chrono>
#include <cmath>

#include "camera.h"
#include "image_io.h"
#include "triangle.h"

int main(int argc, char* argv[])
{
    const int width = 1280;
    const int height = 720;

    sycl::queue queue;

    std::cout << "Using " << queue.get_device().get_info<sycl::info::device::name>() << std::endl;

    Image image(width, height);

    std::vector<Triangle> triangles_buffer_host;
    triangles_buffer_host.push_back(Triangle(Point(0, 0, -2), Point(1, 0, -2), Point(0.5, 0.5, -2.0)));

    sycl::buffer<Color> image_buffer(image.color_data(), image.width() * image.height());
    sycl::buffer<Triangle> triangle_buffer(triangles_buffer_host.data(), triangles_buffer_host.size());

    Camera camera;

    auto start = std::chrono::high_resolution_clock::now();
    queue.submit([&] (sycl::handler& handler) {
        auto image_buffer_access = image_buffer.get_access<sycl::access::mode::read_write>(handler);
        auto triangle_buffer_access = triangle_buffer.get_access<sycl::access::mode::read>(handler);

        handler.parallel_for(sycl::range<2>(height, width), [camera, image_buffer_access, triangle_buffer_access] (sycl::id<2> coordinates)
        {
            int x = coordinates[1];
            int y = coordinates[0];

            float x_ndc_space = (float)x / width * 2 - 1;
            float y_ndc_space = (float)y / height * 2 - 1;

            Point ray_origin(0, 0, 0);
            Point ray_origin_world_space = camera.view_matrix(ray_origin);

            Point ray_point_direction = Point(x_ndc_space, y_ndc_space, -1);
            Point ray_point_direction_view_space = camera.perspective_projection(ray_point_direction);
            Point ray_point_direction_world_space = camera.view_matrix(ray_point_direction_view_space);

            Ray ray(ray_origin_world_space, ray_point_direction_world_space);
            for (const Triangle& triangle : triangle_buffer_access)
            {
                HitInfo hit_info;
                if (triangle.intersect(ray, hit_info))
                {
                    image_buffer_access[y * width + x] = Color(1.0, 0.0, 0.0);
                }
            }
        });
    }).wait();

    image_buffer.get_access<sycl::access::mode::read>();
    auto stop = std::chrono::high_resolution_clock::now();

    //TODO pour l'histoire des external, tester:
    // - Un cas simple (une seule fonction) pour que ce soit facilement reproduisible
    // - De memoire, quand on avait que queue.parallel_for on pouvait appeler les Operator+ de Vector ?
    // - L'histoire des kernel class
    // TODO essayer de caller partout parce que ça a l'air de marcher

    //TODO demander sur le forum intel pour comment debugguer avec device_host() parce que c'est
    //deprecated (dire que j'ai installe SYCL avec ce tuto etc...)

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

    write_image_png(image, "test.png");

    return 0;
}

/*
#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 16;

int main(){
  queue q;
  std::cout << "Device: " << q.get_device().get_info<info::device::name>() << std::endl;

  std::vector<int> v(N);
  for(int i=0; i<N; i++) v[i] = i;

  buffer<int, 1> buf(v.data(), v.size());
  q.submit([&] (handler &h){
    auto A = buf.get_access<access::mode::read_write>(h);
    h.parallel_for(range<1>(N), [=](id<1> i){
      A[i] *= 2;
    });
  });

  buf.get_access<access::mode::read>(); // <--- Host Accessor to Synchronize Memory

  for(int i=0; i<N; i++) std::cout << v[i] << std::endl;
  return 0;
}
*/

/*
#include <sycl.hpp>

#include "vector_test.h"

int main()
{
    sycl::queue queue;

    VectorTest a(1, 0, 0);
    VectorTest b = a.get_normalized();
    std::cout << b.x;
    //Buffer on the host
    std::vector<VectorTest> vector_buffer_host;
    for (int i = 0; i < 10; i++) vector_buffer_host.push_back(VectorTest(i, i, i));

    //Buffer on the device
    sycl::buffer<VectorTest> vector_buffer_device(vector_buffer_host.data(), vector_buffer_host.size());

    queue.submit([&] (sycl::handler& handler) {
        auto vector_buffer_device_access = vector_buffer_device.get_access<sycl::access::mode::read_write>(handler);

        //Multiplies each vector by 2
        handler.parallel_for(sycl::range<1>(vector_buffer_host.size()), [vector_buffer_device_access] (sycl::id<1> id) {
            vector_buffer_device_access[id] = vector_buffer_device_access[id].get_normalized();
        });
    });

    vector_buffer_device.get_access<sycl::access::mode::read>();

    //Displaying the results
    for (int i = 0; i < vector_buffer_host.size(); i++)
        std::cout << vector_buffer_host[i].x << std::endl;
}
*/
