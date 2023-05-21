// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// A room (i.e a level)

#pragma once

#include "base/geom.h"
#include <map>
#include <string>
#include <vector>

using namespace std;

#include "base/mesh.h"
#include "convex.h"

struct Room
{
  int startpos_x;
  int startpos_y;
  int startpos_z;

  struct Thing
  {
    Vector pos;
    string name;
    map<string, string> config;
  };

  struct Light
  {
    Vector pos;
    Vector color;
  };

  vector<Thing> things;
  vector<Convex> colliders;
  vector<Light> lights;
};

Room loadRoom(String filename);

