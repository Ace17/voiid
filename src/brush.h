#pragma once
#include <vector>
#include "trace.h"

struct Brush
{
  std::vector<Plane> planes;
  Trace trace(Vector A, Vector B, float radius) const;
};

