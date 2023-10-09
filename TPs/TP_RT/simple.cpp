#include <iostream>
#include <CL/sycl.hpp>
#include <chrono>
#include <cmath>

class Vector
{
public:
    Vector(float x, float y, float z) : x(x), y(y), z(z) {}

    friend Vector operator+(const Vector& u, const Vector& v);

    float x, y, z;
};

Vector operator+(const Vector& u, const Vector& v)
{
    return Vector(u.x + v.x, u.y + v.y, u.z + v.z);
}

int main(int argc, char* argv[])
{
    sycl::queue queue;

    std::cout << "Using "
        << queue.get_device().get_info<sycl::info::device::name>()
        << std::endl;

    // Compute the first n_items values in a well known sequence
    constexpr int n_items = 500000000;

    float *items = sycl::malloc_shared<float>(n_items, queue);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 40; i++)
    {
        queue.parallel_for(sycl::range<1>(n_items), [items] (sycl::id<1> i) {
            Vector a(i, i, i);
            Vector b(i / 2, i / 2, i / 2);

            Vector c = a + b;

            float x1 = powf(i, (1.0 + sqrtf(5.0))/2);
            float x2 = powf(i + 1, (1.0 + sqrtf(5.0))/2);
            items[i] = round((x1 - x2)/sqrtf(5)) + c.x;
        }).wait();
    }
    auto stop = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10; i++)
        std::cout << items[i] << ", ";
    std::cout << std::endl;

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

    free(items, queue);

    return 0;
}
