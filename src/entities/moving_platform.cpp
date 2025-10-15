// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// moving block that the player can walk on

#include "base/scene.h"
#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h"

#include "collision_groups.h"

#include <cmath>

struct MovingPlatform : Entity
{
  MovingPlatform(int dir_)
  {
    solid = true;
    pusher = true;
    size = Size(2, 2, 1);
    collisionGroup = CG_WALLS;
    ticks = 0;
    dir = dir_;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    view->sendActor(r);
  }

  void tick() override
  {
    auto delta = 0.03 * sin(ticks * 0.01);
    auto v = dir ? Vector(0, delta, 0) : Vector(0, 0, delta);
    physics->moveBody(this, v);
    ++ticks;
  }

  int ticks = 0;
  int dir = 0;
};

static auto const reg1_ = registerEntity("moving_platform", [] (IEntityConfig* args) -> std::unique_ptr<Entity> { auto arg = args->getInt("dir"); return std::make_unique<MovingPlatform>(arg); });

