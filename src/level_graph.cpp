// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Loader for rooms (levels)

#include "room.h"
#include "base/mesh.h"
#include <fstream>
#include <map>
#include <algorithm>

static Vector3f toVector3f(Mesh::Vertex v)
{
  return Vector3f(v.x, v.y, v.z);
}

static bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

static Vector3f computeNormal(Mesh& mesh, int i1, int i2, int i3)
{
  auto A = toVector3f(mesh.vertices[i1]);
  auto B = toVector3f(mesh.vertices[i2]);
  auto C = toVector3f(mesh.vertices[i3]);

  return normalize(crossProduct(B - A, C - A));
}

static bool operator != (Vector3f a, Vector3f b)
{
  return a.x != b.x || a.y != b.y || a.z != b.z;
}

static bool operator < (Vector3f a, Vector3f b)
{
  if(a.x != b.x)
    return a.x < b.x;

  if(a.y != b.y)
    return a.y < b.y;

  if(a.z != b.z)
    return a.z < b.z;

  return false;
}

void bevelSharpEdges(Mesh& mesh, Convex& brush)
{
  struct EdgeId
  {
    Vector3f v1, v2; // by convention: assert(v1 < v2)
    bool operator < (EdgeId other) const
    {
      if(v1 != other.v1)
        return v1 < other.v1;

      return v2 < other.v2;
    }

    static EdgeId mk(Vector3f a, Vector3f b)
    {
      return EdgeId { min(a, b), max(a, b) };
    }
  };

  struct EdgeInfo
  {
    std::vector<Vector3f> normals; // must have .size()==2
  };

  map<EdgeId, EdgeInfo> edges;

  for(auto f : mesh.faces)
  {
    auto const N = computeNormal(mesh, f.i1, f.i2, f.i3);

    auto A = toVector3f(mesh.vertices[f.i1]);
    auto B = toVector3f(mesh.vertices[f.i2]);
    auto C = toVector3f(mesh.vertices[f.i3]);
    edges[EdgeId::mk(A, B)].normals.push_back(N);
    edges[EdgeId::mk(B, C)].normals.push_back(N);
    edges[EdgeId::mk(C, A)].normals.push_back(N);
  }

  for(auto& e : edges)
  {
    auto& info = e.second;
    assert(info.normals.size() == 2);
    auto N1 = info.normals[0];
    auto N2 = info.normals[1];

    if(dotProduct(N1, N2) > 0)
      continue;

    auto N3 = normalize(N1 + N2);
    auto D = dotProduct(N3, e.first.v1);
    brush.planes.push_back(Plane { N3, D });
  }
}

Room loadRoom(int roomIdx)
{
  Room r;

  char filename[256];
  snprintf(filename, sizeof filename, "res/rooms/%02d/mesh.3ds", roomIdx);

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
      auto const N = computeNormal(mesh, face.i1, face.i2, face.i3);
      auto A = toVector3f(mesh.vertices[face.i1]);
      auto const D = dotProduct(N, A);
      brush.planes.push_back(Plane { N, D });
    }

    bevelSharpEdges(mesh, brush);

    r.brushes.push_back(brush);
  }

  r.start = Vector3i(0, 0, 5);

  return r;
}

