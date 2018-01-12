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

