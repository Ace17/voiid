#pragma once

#include <vector>
#include <memory>
#include <string>

#include "base/span.h"

namespace tds
{
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

std::unique_ptr<Mesh> load(std::string filename);
std::unique_ptr<Mesh> load(Span<uint8_t const> buffer);
}

