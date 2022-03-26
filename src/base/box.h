// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <cassert>

#include "geom.h"

template<typename T>
struct GenericBox
{
  GenericBox() = default;
  GenericBox(T x, T y, T w, T h) : pos(x, y), size(w, h) {}

  GenericVector<T> pos;
  GenericSize<T> size;
};

template<typename T>
struct GenericBox3
{
  GenericBox3() = default;

  GenericBox3(T x, T y, T z, T cx, T cy, T cz) :
    pos(x, y, z),
    size(cx, cy, cz)
  {
  }

  GenericVector3<T> pos;
  GenericSize3<T> size;
};

using Rect2f = GenericBox<float>;
using Rect2i = GenericBox<int>;
using Rect3f = GenericBox3<float>;
using Rect3i = GenericBox3<int>;

template<typename T>
bool segmentsOverlap(T a_left, T a_right, T b_left, T b_right)
{
  static auto swap = [] (T& a, T& b)
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

template<typename T>
bool overlaps(GenericBox3<T> const& a, GenericBox3<T> const& b)
{
  assert(a.size.cx >= 0);
  assert(a.size.cy >= 0);
  assert(a.size.cz >= 0);
  assert(b.size.cx >= 0);
  assert(b.size.cy >= 0);
  assert(b.size.cz >= 0);

  if(!segmentsOverlap(a.pos.x, a.pos.x + a.size.cx, b.pos.x, b.pos.x + b.size.cx))
    return false;

  if(!segmentsOverlap(a.pos.y, a.pos.y + a.size.cy, b.pos.y, b.pos.y + b.size.cy))
    return false;

  if(!segmentsOverlap(a.pos.z, a.pos.z + a.size.cz, b.pos.z, b.pos.z + b.size.cz))
    return false;

  return true;
}

