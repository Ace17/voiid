// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// A collidable body inside the physics engine.
// Bodies are always axis-aligned boxes.

#pragma once

#include <functional>
#include "vec.h"
#include "trace.h"

using namespace std;

struct Body
{
  // make type polymorphic
  virtual ~Body() = default;

  bool solid = false;
  bool pusher = false; // push and crush?
  Vector pos;

  // shape used for collision detection
  Size size = UnitSize;

  // collision masks
  int collisionGroup = 1;
  int collidesWith = 0xFFFF;

  // the body we rest on (if any)
  Body* ground = nullptr;

  // only called if (this->collidesWith & other->collisionGroup)
  function<void(Body*)> onCollision = [] (Body*) {};

  Box getBox() const
  {
    Box r;
    r.pos = pos;
    r.size = size;
    return r;
  }
};

