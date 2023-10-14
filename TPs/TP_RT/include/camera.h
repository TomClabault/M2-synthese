
#ifndef CAMERA_H
#define CAMERA_H

#include "mat.h"

struct Camera
{
    Transform view_matrix = Transform(Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, -1), Vector(0, 0, 0));
    Transform perspective_projection = Perspective(45, 16.0f/9.0f, 0, 1000);
};

#endif
