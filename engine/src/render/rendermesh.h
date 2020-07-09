// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <stdint.h>
#include <string>
#include <vector>
using namespace std;

#include "base/geom.h"

struct SingleRenderMesh
{
  uint32_t buffer = 0;

  // textures
  int diffuse  {};
  int lightmap {};

  // mesh data
  struct Vertex
  {
    float x, y, z; // position
    float nx, ny, nz; // normal
    float diffuse_u, diffuse_v;
    float lightmap_u, lightmap_v;
  };

  vector<Vertex> vertices;
};

struct RenderMesh
{
  vector<SingleRenderMesh> singleMeshes;
};

RenderMesh loadRenderMesh(string path);

