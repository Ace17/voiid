#pragma once

#include <cstdint>
#include <vector>

#include "base/geom.h"
#include "base/string.h"

struct PictureView
{
  Vec2i dim;
  int stride;
  uint8_t* pixels;
};

struct Picture
{
  Vec2i dim;
  int stride;
  std::vector<uint8_t> pixels;

  operator PictureView () { return { dim, stride, pixels.data() }; }
};

Picture addBorderToTiles(PictureView src, int cols, int rows);
Picture loadPicture(String path);

