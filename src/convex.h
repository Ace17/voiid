#pragma once
#include <vector>
#include "trace.h"

struct Convex
{
  std::vector<Plane> planes;
  Trace trace(Vector A, Vector B, float radius) const;
};

