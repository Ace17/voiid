// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "rendermesh.h"
#include "base/geom.h"
#include "base/util.h" // dirName
#include "misc/json.h"
#include "misc/file.h"
#include "3ds.h"
#include <string.h> // strlen

extern int loadTexture(string path, Rect2i rect = Rect2i(0, 0, 0, 0));

static
Action loadSheetAction(json::Value const& action, string sheetPath, Size2i cell)
{
  Action r;

  (string)action["name"];

  for(auto& frame : action["frames"].elements)
  {
    auto const idx = int(frame);

    auto const col = idx % 16;
    auto const row = idx / 16;
    r.addTexture(sheetPath, Rect2i(col * cell.width, row * cell.height, cell.width, cell.height));
  }

  return r;
}

RenderMesh boxModel()
{
  static const RenderMesh::Vertex vertices[] =
  {
    { -0.5, -0.5, +0.5, /* N */ 0, 0, 1, /* uv */ 0, 0, },
    { -0.5, +0.5, +0.5, /* N */ 0, 0, 1, /* uv */ 0, 1, },
    { +0.5, +0.5, +0.5, /* N */ 0, 0, 1, /* uv */ 1, 1, },
    { +0.5, -0.5, +0.5, /* N */ 0, 0, 1, /* uv */ 1, 0, },
    { -0.5, -0.5, -0.5, /* N */ 0, 0, -1, /* uv */ 0, 0, },
    { +0.5, -0.5, -0.5, /* N */ 0, 0, -1, /* uv */ 0, 1, },
    { +0.5, +0.5, -0.5, /* N */ 0, 0, -1, /* uv */ 1, 1, },
    { -0.5, +0.5, -0.5, /* N */ 0, 0, -1, /* uv */ 1, 0, },
    { -0.5, -0.5, +0.5, /* N */ -1, 0, 0, /* uv */ 0, 0, },
    { -0.5, -0.5, -0.5, /* N */ -1, 0, 0, /* uv */ 0, 1, },
    { -0.5, +0.5, -0.5, /* N */ -1, 0, 0, /* uv */ 1, 1, },
    { -0.5, +0.5, +0.5, /* N */ -1, 0, 0, /* uv */ 1, 0, },
    { -0.5, +0.5, +0.5, /* N */ 0, 1, 0, /* uv */ 0, 0, },
    { -0.5, +0.5, -0.5, /* N */ 0, 1, 0, /* uv */ 0, 1, },
    { +0.5, +0.5, -0.5, /* N */ 0, 1, 0, /* uv */ 1, 1, },
    { +0.5, +0.5, +0.5, /* N */ 0, 1, 0, /* uv */ 1, 0, },
    { +0.5, +0.5, +0.5, /* N */ 1, 0, 0, /* uv */ 0, 0, },
    { +0.5, +0.5, -0.5, /* N */ 1, 0, 0, /* uv */ 0, 1, },
    { +0.5, -0.5, -0.5, /* N */ 1, 0, 0, /* uv */ 1, 1, },
    { +0.5, -0.5, +0.5, /* N */ 1, 0, 0, /* uv */ 1, 0, },
    { -0.5, -0.5, -0.5, /* N */ 0, -1, 0, /* uv */ 0, 0, },
    { -0.5, -0.5, +0.5, /* N */ 0, -1, 0, /* uv */ 0, 1, },
    { +0.5, -0.5, +0.5, /* N */ 0, -1, 0, /* uv */ 1, 1, },
    { +0.5, -0.5, -0.5, /* N */ 0, -1, 0, /* uv */ 1, 0, },
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

Vector3f computeNormal(Mesh::Vertex V1, Mesh::Vertex V2, Mesh::Vertex V3)
{
  auto toVector3f =
    [] (Mesh::Vertex V)
    {
      return Vector3f(V.x, V.y, V.z);
    };

  auto const v1 = toVector3f(V1);
  auto const v2 = toVector3f(V2);
  auto const v3 = toVector3f(V3);

  auto const u = v2 - v1;
  auto const v = v3 - v1;

  auto const N = crossProduct(u, v);

  return normalize(N);
}

static bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

RenderMesh modelFromTxt(string path)
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

    float u, v;
    RenderMesh::Vertex vertex;
    int count = sscanf(line,
                       "%f %f %f - %f %f %f - %f %f - %f %f",
                       &vertex.x, &vertex.y, &vertex.z,
                       &vertex.nx, &vertex.ny, &vertex.nz,
                       &u, &v,
                       &vertex.u, &vertex.v
                       );

    if(count != 10)
      throw runtime_error("Invalid line in mesh file: '" + string(line) + "'");

    mesh.vertices.push_back(vertex);
  }

  return mesh;
}

RenderMesh modelFrom3ds(string path3ds)
{
  auto const meshes = tds::load(path3ds);

  RenderMesh r;

  auto addVertex =
    [&] (Mesh::Vertex vert, Vector3f N)
    {
      RenderMesh::Vertex vt {};

      vt.x = vert.x;
      vt.y = vert.y;
      vt.z = vert.z;

      vt.u = vert.u;
      vt.v = vert.v;

      vt.nx = N.x;
      vt.ny = N.y;
      vt.nz = N.z;

      r.vertices.push_back(vt);
    };

  for(auto& mesh : meshes)
  {
    if(startsWith(mesh.name, "f."))
      continue;

    for(auto& face : mesh.faces)
    {
      auto const V1 = mesh.vertices[face.i1];
      auto const V2 = mesh.vertices[face.i2];
      auto const V3 = mesh.vertices[face.i3];

      auto const N = computeNormal(V1, V2, V3);

      addVertex(V1, N);
      addVertex(V2, N);
      addVertex(V3, N);
    }
  }

  return r;
}

RenderMesh loadModel(string jsonPath)
{
  auto data = read(jsonPath);
  RenderMesh r;

  auto const pathTxtRender = setExtension(jsonPath, "render");
  auto const path3ds = setExtension(jsonPath, "3ds");

  if(exists(pathTxtRender))
    r = modelFromTxt(pathTxtRender);
  else if(exists(path3ds))
    r = modelFrom3ds(path3ds);
  else
    r = boxModel();

  auto obj = json::parse(data.c_str(), data.size());
  auto dir = dirName(jsonPath);

  auto type = string(obj["type"]);

  if(type != "sheet")
    throw runtime_error("Unknown model type: '" + type + "'");

  auto sheet = string(obj["sheet"]);
  auto width = int(obj["width"]);
  auto height = int(obj["height"]);

  auto cell = Size2i(width, height);

  for(auto& action : obj["actions"].elements)
    r.actions.push_back(loadSheetAction(action, dir + "/" + sheet, cell));

  return r;
}

void Action::addTexture(string path, Rect2i rect)
{
  textures.push_back(loadTexture(path, rect));
}

