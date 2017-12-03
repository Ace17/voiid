#pragma once

struct Plane
{
  Vector3f N;
  float D;

  float dist(Vector3f pos) const
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
  TRACE trace(Vector3f A, Vector3f B, float radius) const;
};

