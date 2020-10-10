// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Loader for the Blender exporter format

#include "base/mesh.h"
#include "base/span.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <string.h>
#include <vector>

#include "misc/decompress.h"
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

template<size_t N>
String S(const char (& s)[N])
{
  return { s, int(N - 1) };
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

bool accept(String& line, String word)
{
  if(!startsWith(line, word))
    return false;

  line += word.len;
  return true;
}

float parseFloat(String& stream)
{
  char* end = nullptr;
  const float value = strtof(stream.data, &end);

  if(end == stream.data)
    throw runtime_error("Invalid float in mesh file");

  const int len = int(end - stream.data);

  stream += len;
  return value;
}

void expect(String& stream, char c)
{
  if(stream[0] != c)
    throw runtime_error("Unexpected char");

  stream += 1;
}

Mesh parseOneMesh(String& stream, String name)
{
  Mesh mesh;

  mesh.name.assign(name.begin(), name.end());

  while(stream.len)
  {
    auto line = parseLine(stream);

    if(line.len == 0)
      break;

    if(accept(line, S("vertex: ")))
    {
      Mesh::Vertex vertex;

      vertex.x = parseFloat(line);
      expect(line, ' ');
      vertex.y = parseFloat(line);
      expect(line, ' ');
      vertex.z = parseFloat(line);
      expect(line, ' ');
      expect(line, '|');
      expect(line, ' ');
      vertex.nx = parseFloat(line);
      expect(line, ' ');
      vertex.ny = parseFloat(line);
      expect(line, ' ');
      vertex.nz = parseFloat(line);
      expect(line, ' ');
      expect(line, '|');
      expect(line, ' ');
      vertex.u = parseFloat(line);
      expect(line, ' ');
      vertex.v = parseFloat(line);

      mesh.vertices.push_back(vertex);
    }
    else if(accept(line, S("prop: ")))
    {
    }
    else if(accept(line, S("material: ")))
    {
      line += 1;
      line.len--;
      mesh.material.assign(line.begin(), line.end());
    }
    else if(accept(line, S("diffuse: ")))
    {
      line += 1;
      line.len--;
    }
    else
      throw runtime_error("Invalid line in mesh file: '" + string(line.begin(), line.end()) + "'");
  }

  for(int i = 0; i <= (int)mesh.vertices.size() - 3; i += 3)
    mesh.faces.push_back({ i, i + 1, i + 2 });

  return mesh;
}

Material parseOneMaterial(String& stream)
{
  Material r;

  while(stream.len)
  {
    auto line = parseLine(stream);

    if(line.len == 0)
      break;

    if(accept(line, S("diffuse: ")))
    {
      // skip quotes
      line += 1;
      line.len--;
      r.diffuse.assign(line.begin(), line.end());
    }
  }

  return r;
}
}

std::vector<Mesh> importMesh(char const* path)
{
  std::vector<Mesh> meshes;
  std::map<std::string, Material> materials;

  auto gzipData = File::read(path);

  auto s = gzipDecompress(Span<const uint8_t> { (uint8_t*)gzipData.data(), (int)gzipData.size() });

  auto stream = String { (const char*)s.data(), (int)s.size() };

  while(stream.len > 0)
  {
    auto line = parseLine(stream);

    if(accept(line, S("obj: ")))
    {
      // skip quotes
      line += 1;
      line.len--;
      meshes.push_back(parseOneMesh(stream, line));

      meshes.back().material = materials[meshes.back().material].diffuse;
    }
    else if(accept(line, S("material: ")))
    {
      // skip quotes
      line += 1;
      line.len--;
      std::string matName(line.begin(), line.end());
      materials[matName] = parseOneMaterial(stream);
    }
    else
    {
      fprintf(stderr, "skipping unrecognized item header: '%.*s'\n", line.len, line.data);

      while(stream.len)
      {
        auto line = parseLine(stream);

        if(line.len == 0)
          break;
      }
    }
  }

  return meshes;
}

