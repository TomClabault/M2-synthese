#ifndef XORSHIFT_H
#define XORSHIFT_H

#include <cstdint>

#include <sycl/sycl.hpp>

struct xorshift32_state {
    uint32_t a;
};

struct xorshift32_generator
{
    xorshift32_generator(uint32_t seed) : m_state({ seed }) {}

    SYCL_EXTERNAL float operator()()
    {
        return xorshift32() / (float)(uint32_t)(-1);
    }

    SYCL_EXTERNAL uint32_t xorshift32()
    {
        /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
        uint32_t x = m_state.a;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return m_state.a = x;
    }

    xorshift32_state m_state;
};

#endif
