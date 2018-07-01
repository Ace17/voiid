// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once
#include <memory>
#include <vector>
#include <string>

struct Mesh
{
  struct Vertex
  {
    float x, y, z;
    float u, v;
  };

  struct Face
  {
    int i1, i2, i3;
  };

  std::vector<Vertex> vertices;
  std::vector<Face> faces;
  std::vector<int> objects; // index of each object beginning, in "faces"
  std::vector<std::string> objectNames;
};

Mesh loadMesh(char const* path);

