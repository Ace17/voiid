#pragma once

#include "game.h"

struct Room
{
  Vector3i pos;
  Size3i size;
  int theme = 0;
  Matrix3<int> tiles;
  Vector3i start;
  std::string name;

  struct Thing
  {
    Vector pos;
    std::string name;
  };

  vector<Thing> things;
};

Room Graph_loadRoom(int levelIdx, IGame* game);

