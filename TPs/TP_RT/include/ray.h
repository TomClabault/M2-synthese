#ifndef RAY_H
#define RAY_H

#include "vec.h"

struct Ray
{
    Ray(Point origin, Point direction) : origin(origin), direction(direction) {}

    Point origin;
    Vector direction;
};

#endif
