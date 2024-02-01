// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Loader for rooms (levels)

#include "base/mesh.h"
#include "base/span.h"
#include "room.h"
#include <algorithm>
#include <map>
#include <stdexcept>

static Vec3f toVec3f(Mesh::Vertex v)
{
  return Vec3f(v.x, v.y, v.z);
}

static bool startsWith(std::string s, std::string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

static Vec3f computeNormal(Mesh& mesh, int i1, int i2, int i3)
{
  auto A = toVec3f(mesh.vertices[i1]);
  auto B = toVec3f(mesh.vertices[i2]);
  auto C = toVec3f(mesh.vertices[i3]);

  return normalize(crossProduct(B - A, C - A));
}

static bool operator != (Vec3f a, Vec3f b)
{
  return a.x != b.x || a.y != b.y || a.z != b.z;
}

static bool operator < (Vec3f a, Vec3f b)
{
  if(a.x != b.x)
    return a.x < b.x;

  if(a.y != b.y)
    return a.y < b.y;

  if(a.z != b.z)
    return a.z < b.z;

  return false;
}

static
void bevelSharpEdges(Mesh& mesh, Convex& brush)
{
  struct EdgeId
  {
    Vec3f v1, v2; // by convention: assert(v1 < v2)
    bool operator < (EdgeId other) const
    {
      if(v1 != other.v1)
        return v1 < other.v1;

      return v2 < other.v2;
    }

    static EdgeId mk(Vec3f a, Vec3f b)
    {
      return EdgeId { std::min(a, b), std::max(a, b) };
    }
  };

  struct EdgeInfo
  {
    std::vector<Vec3f> normals; // must have .size()==2
  };

  std::map<EdgeId, EdgeInfo> edges;

  for(auto f : mesh.faces)
  {
    auto const N = computeNormal(mesh, f.i1, f.i2, f.i3);

    auto A = toVec3f(mesh.vertices[f.i1]);
    auto B = toVec3f(mesh.vertices[f.i2]);
    auto C = toVec3f(mesh.vertices[f.i3]);
    edges[EdgeId::mk(A, B)].normals.push_back(N);
    edges[EdgeId::mk(B, C)].normals.push_back(N);
    edges[EdgeId::mk(C, A)].normals.push_back(N);
  }

  for(auto& e : edges)
  {
    auto& info = e.second;

    if(info.normals.size() != 2)
    {
      fprintf(stderr, "BevelSharpEdges: issue with mesh '%s' : %d faces are incident to the same edge\n", mesh.name.c_str(), (int)info.normals.size());
      continue;
    }

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

static
std::vector<std::string> parseCall(std::string content)
{
  content += '\0';
  auto stream = content.c_str();

  auto head = [&] ()
    {
      return *stream;
    };

  auto accept = [&] (char what)
    {
      if(!*stream)
        return false;

      if(head() != what)
        return false;

      stream++;
      return true;
    };

  auto expect = [&] (char what)
    {
      if(!accept(what))
        throw std::runtime_error(std::string("Expected '") + what + "'");
    };

  auto parseString = [&] ()
    {
      std::string r;

      while(!accept('"'))
      {
        char c = head();
        accept(c);
        r += c;
      }

      return r;
    };

  auto parseIdentifier = [&] ()
    {
      std::string r;

      while(isalnum(head()) || head() == '_' || head() == '-')
      {
        char c = head();
        accept(c);
        r += c;
      }

      return r;
    };

  auto parseArgument = [&] ()
    {
      if(accept('"'))
        return parseString();
      else
        return parseIdentifier();
    };

  std::vector<std::string> r;
  r.push_back(parseIdentifier());

  if(accept('('))
  {
    bool first = true;

    while(!accept(')'))
    {
      if(!first)
        expect(',');

      r.push_back(parseArgument());
      first = false;
    }
  }

  return r;
}

static
std::map<std::string, std::string> parseFormula(std::string formula, std::string& name)
{
  std::map<std::string, std::string> r;

  auto words = parseCall(formula);
  name = words[0];
  words.erase(words.begin());

  int i = 0;

  for(auto& varValue : words)
    r[std::to_string(i++)] = varValue;

  return r;
}

Room loadRoom(String filename)
{
  Room r;

  r.startpos_x = 0;
  r.startpos_y = 0;
  r.startpos_z = 2;

  auto importedMesh = importMesh(filename);

  for(auto& light : importedMesh.lights)
    r.lights.push_back({
      { light.x, light.y, light.z }, { light.r, light.g, light.b }
    });

  for(auto& mesh : importedMesh.meshes)
  {
    auto& name = mesh.name;

    if(mesh.vertices.empty())
    {
      fprintf(stderr, "WARNING: object '%s' has no vertices\n", name.c_str());
      continue;
    }

    if(startsWith(name, "nocollide."))
      continue;

    std::string typeName;

    std::map<std::string, std::string> config;

    if(startsWith(name, "f."))
      config = parseFormula(name.substr(2), typeName);

    for(auto& prop : mesh.properties)
    {
      if(prop.name == "type")
        typeName = prop.value;
      else
        config[prop.name] = prop.value;
    }

    if(typeName.size())
    {
      Vec4f pos = { 0, 0, 0, 1 };
      pos = mesh.transform * pos;

      if(typeName == "start")
      {
        r.startpos_x = pos.x;
        r.startpos_y = pos.y;
        r.startpos_z = pos.z;
      }
      else
      {
        r.things.push_back({ Vec3f(pos.x, pos.y, pos.z), typeName, config });
      }

      continue;
    }

    Convex brush;

    for(auto& face : mesh.faces)
    {
      auto const N = computeNormal(mesh, face.i1, face.i2, face.i3);
      auto A = toVec3f(mesh.vertices[face.i1]);
      auto const D = dotProduct(N, A);
      brush.planes.push_back(Plane { N, D });
    }

    bevelSharpEdges(mesh, brush);

    r.colliders.push_back(brush);
  }

  return r;
}

