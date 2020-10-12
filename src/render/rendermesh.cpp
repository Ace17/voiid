// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "base/geom.h"
#include "base/util.h" // setExtension
#include "misc/file.h"
#include "rendermesh.h"
#include <stdexcept>
#include <string.h> // strlen

RenderMesh boxModel()
{
  static const SingleRenderMesh::Vertex vertices[] =
  {
    { -0.5, -0.5, +0.5, /* N */ 0, 0, 1, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { -0.5, +0.5, +0.5, /* N */ 0, 0, 1, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { +0.5, +0.5, +0.5, /* N */ 0, 0, 1, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { +0.5, -0.5, +0.5, /* N */ 0, 0, 1, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
    { -0.5, -0.5, -0.5, /* N */ 0, 0, -1, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { +0.5, -0.5, -0.5, /* N */ 0, 0, -1, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { +0.5, +0.5, -0.5, /* N */ 0, 0, -1, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { -0.5, +0.5, -0.5, /* N */ 0, 0, -1, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
    { -0.5, -0.5, +0.5, /* N */ -1, 0, 0, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { -0.5, -0.5, -0.5, /* N */ -1, 0, 0, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { -0.5, +0.5, -0.5, /* N */ -1, 0, 0, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { -0.5, +0.5, +0.5, /* N */ -1, 0, 0, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
    { -0.5, +0.5, +0.5, /* N */ 0, 1, 0, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { -0.5, +0.5, -0.5, /* N */ 0, 1, 0, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { +0.5, +0.5, -0.5, /* N */ 0, 1, 0, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { +0.5, +0.5, +0.5, /* N */ 0, 1, 0, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
    { +0.5, +0.5, +0.5, /* N */ 1, 0, 0, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { +0.5, +0.5, -0.5, /* N */ 1, 0, 0, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { +0.5, -0.5, -0.5, /* N */ 1, 0, 0, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { +0.5, -0.5, +0.5, /* N */ 1, 0, 0, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
    { -0.5, -0.5, -0.5, /* N */ 0, -1, 0, /* uv diffuse */ 0, 0, /* uv lightmap */ 0, 0, },
    { -0.5, -0.5, +0.5, /* N */ 0, -1, 0, /* uv diffuse */ 0, 1, /* uv lightmap */ 0, 1, },
    { +0.5, -0.5, +0.5, /* N */ 0, -1, 0, /* uv diffuse */ 1, 1, /* uv lightmap */ 1, 1, },
    { +0.5, -0.5, -0.5, /* N */ 0, -1, 0, /* uv diffuse */ 1, 0, /* uv lightmap */ 1, 0, },
  };

  static const short faces[] =
  {
    0, 2, 1,
    0, 3, 2,

    4, 6, 5,
    4, 7, 6,

    8, 10, 9,
    8, 11, 10,

    12, 14, 13,
    12, 15, 14,

    16, 18, 17,
    16, 19, 18,

    20, 22, 21,
    20, 23, 22,
  };

  RenderMesh model;
  model.singleMeshes.resize(1);

  for(auto idx : faces)
    model.singleMeshes[0].vertices.push_back(vertices[idx]);

  return model;
}

static
RenderMesh loadBinaryRenderMesh(string path)
{
  auto fp = fopen(path.c_str(), "rb");

  if(!fp)
    throw runtime_error("Can't open model file: '" + path + "'");

  RenderMesh mesh;

  while(1)
  {
    int vertexCount = 0;
    int n = fread(&vertexCount, 1, 4, fp);

    if(n == 0)
      break;

    SingleRenderMesh single;

    for(int i = 0; i < vertexCount; ++i)
    {
      SingleRenderMesh::Vertex vertex;
      fread(&vertex, 1, sizeof vertex, fp);

      single.vertices.push_back(vertex);
    }

    assert(!single.vertices.empty());

    mesh.singleMeshes.push_back(single);
  }

  fclose(fp);

  return mesh;
}

RenderMesh loadRenderMesh(string renderPath)
{
  if(!File::exists(renderPath))
    return boxModel();

  return loadBinaryRenderMesh(renderPath);
}

