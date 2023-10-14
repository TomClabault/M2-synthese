#ifndef HIT_INFO_H
#define HIT_INFO_H

#include "vec.h"

struct HitInfo
{
    Point inter_point;
    Vector normal_at_inter;

    float u, v;
};

#endif
