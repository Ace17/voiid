#pragma once

#include "base/mesh.h"
#include "game.h"

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
  Mesh world;
};

Room Graph_loadRoom(int levelIdx, IGame* game);

