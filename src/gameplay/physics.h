// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Physical world, as seen by the game

#pragma once

#include "body.h"
#include "physics_probe.h"
#include <memory>

struct IPhysics : IPhysicsProbe
{
  virtual ~IPhysics() = default;

  // called by game
  virtual void checkForOverlaps() = 0;

  virtual void addBody(Body* body) = 0;
  virtual void removeBody(Body* body) = 0;
};

std::unique_ptr<IPhysics> createPhysics();

