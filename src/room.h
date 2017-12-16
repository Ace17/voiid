#pragma once

#include "base/mesh.h"
#include "game.h"
#include "convex.h"

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
  vector<Convex> brushes;
};

Room Graph_loadRoom(int levelIdx, IGame* game);

