// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Physical world, as seen by entities

#pragma once

#include "body.h"
#include "trace.h"

struct IPhysicsProbe
{
  struct Trace : public ::Trace
  {
    Body* blocker;
  };
  virtual Trace moveBody(Body* body, Vector delta) = 0;
  virtual Trace traceBox(Box box, Vector delta, const Body* except) const = 0;
  virtual Body* getBodiesInBox(Box myRect, int collisionGroup, bool onlySolid = false, const Body* except = nullptr) const = 0;
};

