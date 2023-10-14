#include <sycl.hpp>

struct VectorTest
{
    VectorTest(float x, float y, float z) : x(x), y(y), z(z) {}

    SYCL_EXTERNAL VectorTest get_normalized() const;
    float length() const;

    float x, y, z;
};
