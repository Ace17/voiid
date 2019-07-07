// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "rendermesh.h"
#include "base/geom.h"
#include "base/util.h" // setExtension
#include "misc/file.h"
#include "3ds.h"
#include <string.h> // strlen

extern int loadTexture(string path, Rect2i rect = Rect2i(0, 0, 0, 0));

RenderMesh boxModel()
{
  static const RenderMesh::Vertex vertices[] =
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

  for(auto idx : faces)
    model.vertices.push_back(vertices[idx]);

  return model;
}

static
RenderMesh loadTxtRenderMesh(string path)
{
  auto fp = fopen(path.c_str(), "rb");

  if(!fp)
    throw runtime_error("Can't open model file: '" + path + "'");

  RenderMesh mesh;
  char line[256];

  while(fgets(line, sizeof line, fp))
  {
    auto n = strlen(line);

    if(n > 0 && line[n - 1] == '\n')
      line[n - 1] = 0;

    if(line[0] == 0 || line[0] == '#')
      continue;

    RenderMesh::Vertex vertex;
    int count = sscanf(line,
                       "%f %f %f - %f %f %f - %f %f - %f %f",
                       &vertex.x, &vertex.y, &vertex.z,
                       &vertex.nx, &vertex.ny, &vertex.nz,
                       &vertex.diffuse_u, &vertex.diffuse_v,
                       &vertex.lightmap_u, &vertex.lightmap_v
                       );

    if(count != 10)
      throw runtime_error("Invalid line in mesh file: '" + string(line) + "'");

    mesh.vertices.push_back(vertex);
  }

  fclose(fp);

  return mesh;
}

RenderMesh loadModel(string renderPath)
{
  RenderMesh r;

  if(exists(renderPath))
    r = loadTxtRenderMesh(renderPath);
  else
    r = boxModel();

  r.diffuse = loadTexture(setExtension(renderPath, "diffuse.png"));
  r.lightmap = loadTexture(setExtension(renderPath, "png"));
  return r;
}

