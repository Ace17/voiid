/*
 * Copyright (C) 2017 - Sebastien Alaiwan
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include "base/util.h"
#include "base/scene.h"
#include "game/entity.h"
#include "game/models.h"
#include "game/sounds.h"
#include "game/toggle.h"
#include "game/collision_groups.h"

struct RoomBoundaryDetector : Entity
{
  RoomBoundaryDetector()
  {
    size = UnitSize;
    solid = false;
    collisionGroup = 0;
    collidesWith = CG_PLAYER | CG_SOLIDPLAYER;
  }

  virtual void enter() override
  {
    Body::onCollision =
      [ = ] (Body*)
      {
        game->postEvent(make_unique<TouchLevelBoundary>(targetLevel, transform));
        Body::onCollision = &nop;
      };
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    return r;
  }

  int targetLevel = 0;
  Vector transform;
};

struct RoomBoundaryBlocker : Entity
{
  RoomBoundaryBlocker(int groupsToBlock)
  {
    size = UnitSize;
    solid = true;
    collisionGroup = CG_WALLS;
    collidesWith = groupsToBlock;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    r.effect = Effect::Blinking;
    return r;
  }
};

