/*
 * Copyright (C) 2017 - Sebastien Alaiwan
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// A room (i.e a level)

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

