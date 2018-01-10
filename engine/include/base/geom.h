/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Basic geometric types

#pragma once

#include <cmath>
#include <algorithm>
#include <cassert>

///////////////////////////////////////////////////////////////////////////////
// Dimension
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct GenericSize
{
  typedef GenericSize<T> MyType;

  GenericSize() : width(0), height(0)
  {
  }

  GenericSize(T w, T h) : width(w), height(h)
  {
  }

  template<typename F>
  friend MyType operator * (MyType const& a, F val)
  {
    return MyType(a.width * val, a.height * val);
  }

  template<typename F>
  friend MyType operator / (MyType const& a, F val)
  {
    return MyType(a.width / val, a.height / val);
  }

  bool operator == (GenericSize const& other) const
  {
    return width == other.width && height == other.height;
  }

  bool operator != (GenericSize const& other) const
  {
    return !(*this == other);
  }

  T width, height;
};

template<typename T>
struct GenericSize3
{
  typedef GenericSize3<T> MyType;

  GenericSize3() : cx(0), cy(0), cz(0)
  {
  }

  GenericSize3(T cx_, T cy_, T cz_) : cx(cx_), cy(cy_), cz(cz_)
  {
  }

  template<typename F>
  friend MyType operator * (MyType const& a, F val)
  {
    return MyType(a.cx * val, a.cy * val, a.cz * val);
  }

  T cx, cy, cz;
};

typedef GenericSize<int> Size2i;
typedef GenericSize<float> Size2f;
typedef GenericSize3<int> Size3i;
typedef GenericSize3<float> Size3f;

///////////////////////////////////////////////////////////////////////////////
// Vector
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct GenericVector
{
  typedef GenericVector<T> MyType;

  GenericVector() : x(0), y(0)
  {
  }

  GenericVector(T x_, T y_) : x(x_), y(y_)
  {
  }

  MyType operator += (MyType const& other)
  {
    x += other.x;
    y += other.y;
    return *this;
  }

  MyType operator -= (MyType const& other)
  {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  template<typename F>
  friend MyType operator * (MyType const& a, F val)
  {
    return MyType(a.x * val, a.y * val);
  }

  template<typename F>
  friend MyType operator / (MyType const& a, F val)
  {
    return MyType(a.x / val, a.y / val);
  }

  template<typename F>
  friend MyType operator * (F val, MyType const& a)
  {
    return MyType(a.x * val, a.y * val);
  }

  friend MyType operator + (MyType const& a, MyType const& b)
  {
    MyType r = a;
    r += b;
    return r;
  }

  friend MyType operator - (MyType const& a, MyType const& b)
  {
    MyType r = a;
    r -= b;
    return r;
  }

  T x, y;
};

template<typename T>
struct GenericVector3
{
  typedef GenericVector3<T> MyType;

  GenericVector3() : x(0), y(0), z(0)
  {
  }

  GenericVector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_)
  {
  }

  GenericVector3(GenericSize3<T> size) : x(size.cx), y(size.cy), z(size.cz)
  {
  }

  MyType operator += (MyType const& other)
  {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  MyType operator -= (MyType const& other)
  {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  template<typename F>
  friend MyType operator * (MyType const& a, F val)
  {
    return MyType(a.x * val, a.y * val, a.z * val);
  }

  template<typename F>
  friend MyType operator * (F val, MyType const& a)
  {
    return MyType(a.x * val, a.y * val, a.z * val);
  }

  friend MyType operator + (MyType const& a, MyType const& b)
  {
    MyType r = a;
    r += b;
    return r;
  }

  friend MyType operator - (MyType const& a, MyType const& b)
  {
    MyType r = a;
    r -= b;
    return r;
  }

  T x, y, z;
};

template<typename T>
inline T dotProduct(GenericVector3<T> a, GenericVector3<T> b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<typename T>
inline auto magnitude(GenericVector3<T> v)
{
  return sqrt(dotProduct(v, v));
}

template<typename T>
inline auto normalize(GenericVector3<T> v)
{
  return v * (1.0f / magnitude(v));
}

template<typename T>
inline auto crossProduct(GenericVector3<T> a, GenericVector3<T> b)
{
  GenericVector3<T> r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - b.z * a.x;
  r.z = a.x * b.y - a.y * b.x;
  return r;
}

typedef GenericVector<int> Vector2i;
typedef GenericVector<float> Vector2f;

typedef GenericVector3<int> Vector3i;
typedef GenericVector3<float> Vector3f;

///////////////////////////////////////////////////////////////////////////////
// Box
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct GenericBox : GenericVector<T>, GenericSize<T>
{
  GenericBox() = default;

  GenericBox(T x, T y, T w, T h) :
    GenericVector<T>(x, y),
    GenericSize<T>(w, h)
  {
  }
};

template<typename T>
struct GenericBox3 : GenericVector3<T>, GenericSize3<T>
{
  GenericBox3()
  {
  }

  GenericBox3(T x, T y, T z, T cx, T cy, T cz) :
    GenericVector3<T>(x, y, z),
    GenericSize3<T>(cx, cy, cz)
  {
  }
};

typedef GenericBox<int> Rect2i;
typedef GenericBox<float> Rect2f;

typedef GenericBox3<float> Rect3f;

template<typename T>
bool segmentsOverlap(T a_left, T a_right, T b_left, T b_right)
{
  if(a_left > b_left)
  {
    std::swap(a_left, b_left);
    std::swap(a_right, b_right);
  }

  return b_left >= a_left && b_left < a_right;
}

template<typename T>
bool overlaps(GenericBox3<T> const& a, GenericBox3<T> const& b)
{
  assert(a.cx >= 0);
  assert(a.cy >= 0);
  assert(a.cz >= 0);
  assert(b.cx >= 0);
  assert(b.cy >= 0);
  assert(b.cz >= 0);

  if(!segmentsOverlap(a.x, a.x + a.cx, b.x, b.x + b.cx))
    return false;

  if(!segmentsOverlap(a.y, a.y + a.cy, b.y, b.y + b.cy))
    return false;

  if(!segmentsOverlap(a.z, a.z + a.cz, b.z, b.z + b.cz))
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Matrix
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct Matrix2
{
  Matrix2() = default;

  Matrix2(Matrix2 const &) = delete;
  void operator = (Matrix2 const &) = delete;

  Matrix2(Matrix2&& other)
  {
    data = other.data;
    size = other.size;
    other.data = nullptr;
  }

  void operator = (Matrix2&& other)
  {
    delete[] data;
    data = other.data;
    size = other.size;
    other.data = nullptr;
  }

  Matrix2(Size2i size_) : size(size_)
  {
    resize(size_);
  }

  ~Matrix2()
  {
    delete[] data;
  }

  void resize(Size2i size_)
  {
    delete[] data;

    size = size_;
    data = new T[size.width * size.height];

    for(int i = 0; i < size.width * size.height; ++i)
      data[i] = T();
  }

  Size2i size = Size2i(0, 0);

  T& get(int x, int y)
  {
    assert(isInside(x, y));
    return data[raster(x, y)];
  }

  const T& get(int x, int y) const
  {
    assert(isInside(x, y));
    return data[raster(x, y)];
  }

  void set(int x, int y, T const& val)
  {
    assert(isInside(x, y));
    get(x, y) = val;
  }

  template<typename Lambda>
  void scan(Lambda f)
  {
    for(int y = 0; y < size.height; y++)
      for(int x = 0; x < size.width; x++)
        f(x, y, get(x, y));
  }

  template<typename Lambda>
  void scan(Lambda f) const
  {
    for(int y = 0; y < size.height; y++)
      for(int x = 0; x < size.width; x++)
        f(x, y, get(x, y));
  }

  bool isInside(int x, int y) const
  {
    if(x < 0 || y < 0)
      return false;

    if(x >= size.width || y >= size.height)
      return false;

    return true;
  }

private:
  T* data = nullptr;

  int raster(int x, int y) const
  {
    return y * size.width + x;
  }
};

template<typename T>
struct Matrix3
{
  Matrix3() = default;

  Matrix3(Size3i size_) : size(size_)
  {
    resize(size_);
  }

  ~Matrix3()
  {
    delete[] data;
  }

  Matrix3(Matrix3&& other)
  {
    data = other.data;
    size = other.size;
    other.data = nullptr;
  }

  void operator = (Matrix3&& other)
  {
    delete[] data;
    data = other.data;
    size = other.size;
    other.data = nullptr;
  }

  Matrix3(Matrix3 const& other)
  {
    *this = other;
  }

  void operator = (Matrix3 const& other)
  {
    resize(other.size);

    for(int i = 0; i < size.cx * size.cy * size.cz; ++i)
      data[i] = other.data[i];
  }

  void resize(Size3i size_)
  {
    delete[] data;

    size = size_;
    data = new T[size.cx * size.cy * size.cz];

    for(int i = 0; i < size.cx * size.cy * size.cz; ++i)
      data[i] = T();
  }

  Size3i size;

  T& get(int x, int y, int z)
  {
    assert(isInside(x, y, z));
    return data[raster(x, y, z)];
  }

  const T& get(int x, int y, int z) const
  {
    assert(isInside(x, y, z));
    return data[raster(x, y, z)];
  }

  void set(int x, int y, int z, T const& val)
  {
    assert(isInside(x, y, z));
    get(x, y, z) = val;
  }

  template<typename Lambda>
  void scan(Lambda f)
  {
    for(int z = 0; z < size.cz; z++)
      for(int y = 0; y < size.cy; y++)
        for(int x = 0; x < size.cx; x++)
          f(x, y, z, get(x, y, z));
  }

  template<typename Lambda>
  void scan(Lambda f) const
  {
    for(int z = 0; z < size.cz; z++)
      for(int y = 0; y < size.cy; y++)
        for(int x = 0; x < size.cx; x++)
          f(x, y, z, get(x, y, z));
  }

  bool isInside(int x, int y, int z) const
  {
    if(x < 0 || y < 0 || z < 0)
      return false;

    if(x >= size.cx || y >= size.cy || z >= size.cz)
      return false;

    return true;
  }

private:
  T* data = nullptr;

  int raster(int x, int y, int z) const
  {
    return (z * size.cy + y) * size.cx + x;
  }
};

///////////////////////////////////////////////////////////////////////////////
// Trigo
///////////////////////////////////////////////////////////////////////////////
static auto const PI = 3.14159265358979323846;

