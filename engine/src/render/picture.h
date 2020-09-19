#pragma once

#include <cstdint>
#include <vector>

#include "base/geom.h"

struct Picture
{
  Size2i dim;
  int stride;
  std::vector<uint8_t> pixels;
};

Picture addBorderToTiles(const Picture& src, int cols, int rows);
Picture loadPicture(const char* path);

