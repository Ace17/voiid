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

#include "base/mesh.h"
#include "convex.h"

struct Room
{
  Vec3f startpos;

  struct Thing
  {
    Vector pos;
    std::string name;
    std::map<std::string, std::string> config;
  };

  struct Light
  {
    Vector pos;
    Vector color;
  };

  std::vector<Thing> things;
  std::vector<Convex> colliders;
  std::vector<Light> lights;
};

Room loadRoom(String filename);

