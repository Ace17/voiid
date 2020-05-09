// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once
#include <string>
#include <vector>

struct Material
{
  std::string diffuse;
};

struct Mesh
{
  struct Vertex
  {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
  };

  struct Face
  {
    int i1, i2, i3;
  };

  std::string name;
  std::string material;
  std::vector<Vertex> vertices;
  std::vector<Face> faces;
};

std::vector<Mesh> importMesh(char const* path);

