// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// A convex shape represented by its planes.
// (Quake games call these "brushes")

#pragma once
#include "trace.h"
#include <vector>

struct Convex
{
  std::vector<Plane> planes;
  Trace trace(Vector A, Vector B, Vector boxSize = {}) const;
};

struct Triangle
{
  Vec3f vertices[3];
  Vec3f normal;
  Vec3f edgeDirs[3]; // normalized
};

Trace raycastBoxVsTriangle(Vec3f A, Vec3f B, Vec3f boxHalfSize, const Triangle& t);

