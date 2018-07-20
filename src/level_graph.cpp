/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Loader for rooms (levels)

#include "room.h"
#include "base/mesh.h"
#include <fstream>

static Vector3f toVector3f(Mesh::Vertex v)
{
  return Vector3f(v.x, v.y, v.z);
}

static bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

Room loadRoom(int roomIdx)
{
  Room r;

  char filename[256];
  snprintf(filename, sizeof filename, "res/rooms/%02d/mesh.3ds", roomIdx);

  if(!ifstream(filename).is_open())
  {
    snprintf(filename, sizeof filename, "res/rooms/ending/mesh.3ds");
    roomIdx = 0;
  }

  auto meshes = loadMesh(filename);

  for(auto& mesh : meshes)
  {
    auto name = mesh.name;

    if(startsWith(name, "f."))
    {
      auto const pos = toVector3f(mesh.vertices[mesh.faces[0].i1]);
      auto const formula = name.substr(2);
      r.things.push_back({ pos, formula });
      continue;
    }

    Convex brush;

    for(auto& face : mesh.faces)
    {
      auto A = toVector3f(mesh.vertices[face.i1]);
      auto B = toVector3f(mesh.vertices[face.i2]);
      auto C = toVector3f(mesh.vertices[face.i3]);

      auto const N = normalize(crossProduct(B - A, C - A));
      auto const D = dotProduct(N, A);
      brush.planes.push_back(Plane { N, D });
    }

    r.brushes.push_back(brush);
  }

  r.start = Vector3i(0, 0, 5);

  return r;
}

