// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Basic geometric types: Vec2f, Vec2i, Vec3f, Vec3i

#pragma once

struct Vec2f
{
  float x = 0;
  float y = 0;

  Vec2f() = default;
  Vec2f(const Vec2f &) = default;
  Vec2f& operator = (const Vec2f &) = default;
  Vec2f(float x_, float y_) : x(x_)
    , y(y_)
  {
  }

  friend void operator += (Vec2f& a, Vec2f b) { a = a + b; }
  friend void operator -= (Vec2f& a, Vec2f b) { a = a - b; }
  friend void operator *= (Vec2f& a, float b) { a = a * b; }
  friend void operator /= (Vec2f& a, float b) { a = a / b; }

  friend Vec2f operator - (Vec2f v) { return Vec2f{ -v.x, -v.y }; }
  friend Vec2f operator + (Vec2f a, Vec2f b) { return Vec2f{ a.x + b.x, a.y + b.y }; }
  friend Vec2f operator - (Vec2f a, Vec2f b) { return Vec2f{ a.x - b.x, a.y - b.y }; }
  friend Vec2f operator * (Vec2f v, float f) { return Vec2f{ v.x* f, v.y* f }; }
  friend Vec2f operator * (float f, Vec2f v) { return v * f; }
  friend Vec2f operator / (Vec2f v, float f) { return Vec2f{ v.x / f, v.y / f }; }

  friend bool operator == (Vec2f a, Vec2f b) { return a.x == b.x && a.y == b.y; }
};

struct Vec2i
{
  int x = 0;
  int y = 0;

  Vec2i() = default;
  Vec2i(const Vec2i &) = default;
  Vec2i& operator = (const Vec2i &) = default;
  Vec2i(int x_, int y_) : x(x_)
    , y(y_)
  {
  }

  friend void operator += (Vec2i& a, Vec2i b) { a = a + b; }
  friend void operator -= (Vec2i& a, Vec2i b) { a = a - b; }
  friend void operator *= (Vec2i& a, int b) { a = a * b; }
  friend void operator /= (Vec2i& a, int b) { a = a / b; }

  friend Vec2i operator - (Vec2i v) { return Vec2i{ -v.x, -v.y }; }
  friend Vec2i operator + (Vec2i a, Vec2i b) { return Vec2i{ a.x + b.x, a.y + b.y }; }
  friend Vec2i operator - (Vec2i a, Vec2i b) { return Vec2i{ a.x - b.x, a.y - b.y }; }
  friend Vec2i operator * (Vec2i v, int f) { return Vec2i{ v.x* f, v.y* f }; }
  friend Vec2i operator * (int f, Vec2i v) { return v * f; }
  friend Vec2i operator / (Vec2i v, int f) { return Vec2i{ v.x / f, v.y / f }; }

  friend bool operator == (Vec2i a, Vec2i b) { return a.x == b.x && a.y == b.y; }
  friend bool operator != (Vec2i a, Vec2i b) { return !(a == b); }
};

struct Vec3f
{
  float x = 0;
  float y = 0;
  float z = 0;

  Vec3f() = default;
  Vec3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_)
  {
  }

  Vec3f operator += (Vec3f const& other)
  {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  Vec3f operator -= (Vec3f const& other)
  {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  template<typename F>
  friend Vec3f operator * (Vec3f const& a, F val)
  {
    return Vec3f(a.x * val, a.y * val, a.z * val);
  }

  template<typename F>
  friend Vec3f operator * (F val, Vec3f const& a)
  {
    return Vec3f(a.x * val, a.y * val, a.z * val);
  }

  friend Vec3f operator + (Vec3f const& a, Vec3f const& b)
  {
    Vec3f r = a;
    r += b;
    return r;
  }

  friend Vec3f operator - (Vec3f const& a, Vec3f const& b)
  {
    Vec3f r = a;
    r -= b;
    return r;
  }

  bool operator == (Vec3f const& other) const
  {
    return x == other.x && y == other.y && z == other.z;
  }
};

inline float dotProduct(Vec3f a, Vec3f b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

double magnitude(Vec3f v);

inline auto normalize(Vec3f v)
{
  return v * (1.0f / magnitude(v));
}

inline auto crossProduct(Vec3f a, Vec3f b)
{
  Vec3f r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - b.z * a.x;
  r.z = a.x * b.y - a.y * b.x;
  return r;
}

struct Vec4f
{
  float x, y, z, w;
};

static auto const PI = 3.14159265358979323846;

