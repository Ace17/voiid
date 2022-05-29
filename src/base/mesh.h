// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once
#include "matrix4.h"
#include "string.h"
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
    float tx, ty, tz;
    float bx, by, bz;
    float u, v;
  };

  struct Face
  {
    int i1, i2, i3;
  };

  struct Property
  {
    std::string name;
    std::string value;
  };

  std::string name;
  Matrix4f transform;
  std::vector<Property> properties;

  std::string material;
  std::vector<Vertex> vertices;
  std::vector<Face> faces;
};

struct Light
{
  float x, y, z;
  float r, g, b;
};

// contents of the .blend file
struct ImportedScene
{
  std::vector<Mesh> meshes;
  std::vector<Light> lights;
};

ImportedScene importMesh(String path);

