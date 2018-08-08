// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Entity: base game object

#pragma once

#include "base/scene.h"
#include "base/geom.h"
#include "game.h"
#include "body.h"
#include "physics_probe.h"

struct Damageable
{
  virtual void onDamage(int amount) = 0;
};

// implemented by doors, switches
struct Switchable
{
  // when the player presses the 'use' button
  // and we're in range
  virtual void onSwitch() = 0;
};

struct Entity : Body
{
  virtual ~Entity() = default;

  virtual void enter()
  {
    onCollision =
      [ = ] (Body* otherBody)
      {
        auto other = dynamic_cast<Entity*>(otherBody);
        assert(other);
        onCollide(other);
      };
  }

  virtual void leave() {}

  virtual Actor getActor() const = 0;
  virtual void tick() {}

  virtual void onCollide(Entity* /*other*/) {}

  bool dead = false;
  int blinking = 0;
  IGame* game = nullptr;
  IPhysicsProbe* physics = nullptr;

  Vector getCenter() const
  {
    return pos + size * 0.5;
  }
};

