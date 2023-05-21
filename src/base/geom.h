// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Basic geometric types

#pragma once

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

using Size2f = GenericSize<float>;
using Size2i = GenericSize<int>;
using Size3f = GenericSize3<float>;
using Size3i = GenericSize3<int>;

///////////////////////////////////////////////////////////////////////////////
// Vector
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct GenericVector
{
  typedef GenericVector<T> MyType;

  GenericVector() : x(0), y(0) {}
  GenericVector(T x_, T y_) : x(x_), y(y_) {}

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
double magnitude(GenericVector3<T> v);

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

using Vec2f = GenericVector<float>;
using Vec2i = GenericVector<int>;
using Vec3f = GenericVector3<float>;
using Vec3i = GenericVector3<int>;

struct Vec4f
{
  float x, y, z, w;
};

static auto const PI = 3.14159265358979323846;

