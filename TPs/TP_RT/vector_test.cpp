#include "vector_test.h"

#include <cmath>

VectorTest VectorTest::get_normalized() const
{
    float length = this->length();

    return VectorTest(x / length, y / length, z / length);
}

float VectorTest::length() const
{
    return std::sqrt(x*x + y*y + z*z);
}
