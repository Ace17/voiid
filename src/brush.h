#pragma once

struct Plane
{
  Vector N;
  float D;

  float dist(Vector pos) const
  {
    return dotProduct(pos, N) - D;
  }
};

struct Brush
{
  struct TRACE
  {
    float fraction;
    Plane plane;
  };

  vector<Plane> planes;
  TRACE trace(Vector A, Vector B, float radius) const;
};

