/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// A collidable body inside the physics engine.
// Bodies are always axis-aligned boxes.

#pragma once

#include <functional>
#include "vec.h"
#include "trace.h"

using namespace std;

struct Body
{
  virtual ~Body()
  {
  }; // make type polymorphic

  bool solid = false;
  bool pusher = false; // push and crush?
  Vector pos;
  Size size = UnitSize;
  int collisionGroup = 1;
  int collidesWith = 0xFFFF;
  Body* ground = nullptr; // the body we rest on (if any)

  // only called if (this->collidesWith & other->collisionGroup)
  function<void(Body*)> onCollision = &nop;

  static void nop(Body*) {}

  Box getBox() const
  {
    Box r;
    r.pos = pos;
    r.size = size;
    return r;
  }
};

struct IPhysicsProbe
{
  struct Trace : public ::Trace
  {
    Body* blocker;
  };
  // called by entities
  virtual Trace moveBody(Body* body, Vector delta) = 0;
  virtual Trace traceBox(Box box, Vector delta, const Body* except) const = 0;
  virtual Body* getBodiesInBox(Box myRect, int collisionGroup, bool onlySolid = false, const Body* except = nullptr) const = 0;
};

struct IPhysics : IPhysicsProbe
{
  // called by game
  virtual void addBody(Body* body) = 0;
  virtual void removeBody(Body* body) = 0;
  virtual void checkForOverlaps() = 0;
  virtual void setEdifice(function<::Trace(Box, Vector)> isSolid) = 0;
};

