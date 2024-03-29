// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include "base/geom.h"
#include "base/string.h"

struct IVertexBuffer;
struct ITexture;

struct SingleRenderMesh
{
  // renderer stuff
  std::shared_ptr<IVertexBuffer> vb;
  std::shared_ptr<ITexture> diffuse;
  std::shared_ptr<ITexture> lightmap;
  std::shared_ptr<ITexture> normal;
  bool transparency;

  // mesh data
  struct Vertex
  {
    float x, y, z; // position
    float nx, ny, nz; // normal
    float bx, by, bz; // binormal
    float tx, ty, tz; // tangent
    float diffuse_u, diffuse_v;
    float lightmap_u, lightmap_v;
  };

  std::vector<Vertex> vertices;
};

struct RenderMesh
{
  std::vector<SingleRenderMesh> singleMeshes;
};

RenderMesh loadRenderMesh(String path);

