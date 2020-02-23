// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Loader for the Blender exporter format

#include "base/mesh.h"
#include "base/span.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <string.h>
#include <vector>

#include "misc/file.h"

using namespace std;

namespace
{
using String = Span<const char>;

String substr(String s, int n)
{
  s.len = min(n, s.len);
  return s;
}

String stringOf(const char* s)
{
  return { s, (int)strlen(s) };
}

bool startsWith(String s, String prefix)
{
  auto head = substr(s, prefix.len);

  if(head.len != prefix.len)
    return false;

  return memcmp(head.data, prefix.data, prefix.len) == 0;
}

String parseLine(String& stream)
{
  String r = stream;

  while(stream[0] != 0 && stream[0] != '\n')
    stream += 1;

  r.len = stream.data - r.data;

  // skip EOL
  stream += 1;

  return r;
}

Mesh parseOneMesh(String& stream)
{
  Mesh mesh;

  {
    auto line = parseLine(stream);

    if(!startsWith(line, stringOf("obj: ")))
      throw runtime_error("invalid mesh header: '" + string(line.begin(), line.end()) + "'");

    line += 6;
    line.len--;
    mesh.name.assign(line.begin(), line.end());
  }

  while(stream.len)
  {
    auto line = parseLine(stream);

    if(line.len == 0)
      break;

    Mesh::Vertex vertex;
    int count = sscanf(line.data,
                       "%f %f %f - %f %f %f - %f %f",
                       &vertex.x, &vertex.y, &vertex.z,
                       &vertex.nx, &vertex.ny, &vertex.nz,
                       &vertex.u, &vertex.v);

    if(count == 8)
    {
      mesh.vertices.push_back(vertex);
    }
    else if(startsWith(line, stringOf("prop: ")))
    {
    }
    else if(startsWith(line, stringOf("material: ")))
    {
      line += 11;
      line.len--;
      mesh.material.assign(line.begin(), line.end());
    }
    else if(startsWith(line, stringOf("diffuse: ")))
    {
      line += 11;
      line.len--;
    }
    else
      throw runtime_error("Invalid line in mesh file: '" + string(line.begin(), line.end()) + "'");
  }

  for(int i = 0; i <= (int)mesh.vertices.size() - 3; i += 3)
    mesh.faces.push_back({ i, i + 1, i + 2 });

  return mesh;
}
}

std::vector<Mesh> loadMesh(char const* path)
{
  std::vector<Mesh> meshes;

  auto s = read(path);

  auto stream = String { s.data(), (int)s.size() };

  while(stream.len > 0)
    meshes.push_back(parseOneMesh(stream));

  return meshes;
}

