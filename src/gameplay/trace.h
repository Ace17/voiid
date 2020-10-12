#pragma once

#include "vec.h"

struct Plane
{
  Vector N; // normal
  float D; // distance to origin

  float dist(Vector pos) const
  {
    return dotProduct(pos, N) - D;
  }
};

struct Trace
{
  float fraction;
  Plane plane;
};

