// Copyright (C) 2018 - Sebastien Alaiwan
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
  Trace trace(Vector A, Vector B, Vector boxSize) const;
};

