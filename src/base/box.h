// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Basic geometric types: Rect2f, Rect2i, Rect3f
#pragma once

#include <cassert>

#include "geom.h"

struct Rect2f
{
  Rect2f() = default;
  Rect2f(float x, float y, float cx, float cy) : pos(x, y), size(cx, cy) {}
  Rect2f(Vec2f pos_, Vec2f size_) : pos(pos_), size(size_) {}
  Vec2f pos;
  Vec2f size;
};

struct Rect2i
{
  Vec2i pos;
  Vec2i size;
};

struct Rect3f
{
  Rect3f() = default;
  Rect3f(float x, float y, float z, float cx, float cy, float cz) : pos(x, y, z), size(cx, cy, cz) {}
  Rect3f(Vec3f pos_, Vec3f size_) : pos(pos_), size(size_) {}
  Vec3f pos;
  Vec3f size;
};

inline bool segmentsOverlap(float a_left, float a_right, float b_left, float b_right)
{
  static auto swap = [] (float& a, float& b)
    {
      auto t = a;
      a = b;
      b = t;
    };

  if(a_left > b_left)
  {
    swap(a_left, b_left);
    swap(a_right, b_right);
  }

  return b_left >= a_left && b_left < a_right;
}

inline bool overlaps(const Rect3f& a, const Rect3f& b)
{
  assert(a.size.x >= 0);
  assert(a.size.y >= 0);
  assert(a.size.z >= 0);
  assert(b.size.x >= 0);
  assert(b.size.y >= 0);
  assert(b.size.z >= 0);

  if(!segmentsOverlap(a.pos.x, a.pos.x + a.size.x, b.pos.x, b.pos.x + b.size.x))
    return false;

  if(!segmentsOverlap(a.pos.y, a.pos.y + a.size.y, b.pos.y, b.pos.y + b.size.y))
    return false;

  if(!segmentsOverlap(a.pos.z, a.pos.z + a.size.z, b.pos.z, b.pos.z + b.size.z))
    return false;

  return true;
}

