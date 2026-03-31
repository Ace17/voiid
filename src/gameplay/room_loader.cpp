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

Room loadRoom(String filename)
{
  Room r;

  r.startpos = Vec3f(0, 0, 2);

  auto importedMesh = importMesh(filename);

  for(auto& light : importedMesh.lights)
    r.lights.push_back({
      { light.x, light.y, light.z }, { light.r, light.g, light.b }
    });

  for(auto& mesh : importedMesh.meshes)
  {
    auto& name = mesh.name;

    if(startsWith(name, "nocollide."))
      continue;

    std::string typeName;

    std::map<std::string, std::string> config;

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
        r.startpos = Vec3f(pos.x, pos.y, pos.z);
      }
      else
      {
        r.things.push_back({ Vec3f(pos.x, pos.y, pos.z), typeName, config });
      }

      continue;
    }

    if(mesh.vertices.empty())
    {
      fprintf(stderr, "WARNING: mesh object '%s' has no vertices\n", name.c_str());
      continue;
    }

    for(auto& face : mesh.faces)
    {
      Room::Triangle t;
      t.p[0] = toVec3f(mesh.vertices[face.i1]);
      t.p[1] = toVec3f(mesh.vertices[face.i2]);
      t.p[2] = toVec3f(mesh.vertices[face.i3]);
      r.colliders.push_back(t);
    }
  }

  return r;
}

