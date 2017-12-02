#pragma once

#include "base/mesh.h"
#include "game.h"

struct Plane
{
  Vector3f N;
  float D;

  float dist(Vector3f pos) const
  {
    return dotProduct(pos, N) - D;
  }
};

struct TRACE
{
  float fraction;
  Plane plane;
};

struct Brush
{
  vector<Plane> planes;
  TRACE trace(Vector3f A, Vector3f B, float radius) const;
};

struct Room
{
  Vector3i pos;
  Size3i size;
  int theme = 0;
  Vector3i start;
  std::string name;

  struct Thing
  {
    Vector pos;
    std::string name;
  };

  vector<Thing> things;
  vector<Brush> brushes;
};

Room Graph_loadRoom(int levelIdx, IGame* game);

