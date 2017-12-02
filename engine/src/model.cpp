/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <fstream>
#include <math.h>
#include "model.h"
#include "base/geom.h"
#include "base/util.h"
#include "json.h"
#include "3ds.h"

extern int loadTexture(string path, Rect2i rect = Rect2i(0, 0, 0, 0));

static
Action loadSheetAction(json::Value* val, string sheetPath, Size2i cell)
{
  Action r;

  auto action = json::cast<json::Object>(val);
  action->getMember<json::String>("name");
  auto frames = action->getMember<json::Array>("frames");

  for(auto& frame : frames->elements)
  {
    auto const idx = (json::cast<json::Number>(frame.get()))->value;

    auto const col = idx % 16;
    auto const row = idx / 16;
    r.addTexture(sheetPath, Rect2i(col * cell.width, row * cell.height, cell.width, cell.height));
  }

  return r;
}

Model boxModel()
{
  static const Model::Vertex vertices[] =
  {
    { -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 0, 1, },
    { -0.5, +0.5, +0.5, /* uv */ 0, 0, /* N */ 0, 0, 1, },
    { +0.5, +0.5, +0.5, /* uv */ 1, 0, /* N */ 0, 0, 1, },
    { +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 0, 1, },
    { -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, 0, -1, },
    { +0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 0, -1, },
    { +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 0, -1, },
    { -0.5, +0.5, -0.5, /* uv */ 1, 1, /* N */ 0, 0, -1, },
    { -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ -1, 0, 0, },
    { -0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ -1, 0, 0, },
    { -0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ -1, 0, 0, },
    { -0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ -1, 0, 0, },
    { -0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 1, 0, },
    { -0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 1, 0, },
    { +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 1, 0, },
    { +0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 1, 0, },
    { +0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 1, 0, 0, },
    { +0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 1, 0, 0, },
    { +0.5, -0.5, -0.5, /* uv */ 1, 0, /* N */ 1, 0, 0, },
    { +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 1, 0, 0, },
    { -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, -1, 0, },
    { -0.5, -0.5, +0.5, /* uv */ 0, 0, /* N */ 0, -1, 0, },
    { +0.5, -0.5, +0.5, /* uv */ 1, 0, /* N */ 0, -1, 0, },
    { +0.5, -0.5, -0.5, /* uv */ 1, 1, /* N */ 0, -1, 0, },
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

  Model model;

  for(auto idx : faces)
    model.vertices.push_back(vertices[idx]);

  return model;
}

auto magnitude(Vector3f v)
{
  return sqrt(dotProduct(v, v));
}

auto normalize(Vector3f v)
{
  return v * (1.0f / magnitude(v));
}

auto crossProduct(Vector3f a, Vector3f b)
{
  Vector3f r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - b.z * a.x;
  r.z = a.x * b.y - a.y * b.x;
  return r;
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

Model modelFrom3ds(string path3ds)
{
  auto const mesh = tds::load(path3ds);

  Model r;

  auto addVertex =
    [&] (Mesh::Vertex vert, Vector3f N)
    {
      Model::Vertex vt {};

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

  for(auto& face : mesh->faces)
  {
    auto const V1 = mesh->vertices[face.i1];
    auto const V2 = mesh->vertices[face.i2];
    auto const V3 = mesh->vertices[face.i3];

    auto const N = computeNormal(V1, V2, V3);

    addVertex(V1, N);
    addVertex(V2, N);
    addVertex(V3, N);
  }

  return r;
}

Model loadModel(string jsonPath)
{
  Model r;

  auto const path3ds = setExtension(jsonPath, "3ds");

  if(ifstream(path3ds).is_open())
    r = modelFrom3ds(path3ds);
  else
    r = boxModel();

  auto obj = json::load(jsonPath);
  auto dir = dirName(jsonPath);

  auto type = obj->getMember<json::String>("type")->value;
  auto actions = obj->getMember<json::Array>("actions");

  if(type != "sheet")
    throw runtime_error("Unknown model type: '" + type + "'");

  auto sheet = obj->getMember<json::String>("sheet")->value;
  auto width = obj->getMember<json::Number>("width")->value;
  auto height = obj->getMember<json::Number>("height")->value;

  auto cell = Size2i(width, height);

  for(auto& action : actions->elements)
    r.actions.push_back(loadSheetAction(action.get(), dir + "/" + sheet, cell));

  return r;
}

void Action::addTexture(string path, Rect2i rect)
{
  textures.push_back(loadTexture(path, rect));
}

