#pragma once
#include <vector>

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
};

