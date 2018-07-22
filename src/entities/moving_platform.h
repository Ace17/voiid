#pragma once

#include "base/scene.h"
#include "collision_groups.h"
#include "entity.h"
#include "models.h"

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

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;

    return r;
  }

  void tick() override
  {
    auto delta = 0.003 * sin(ticks * 0.001);
    auto v = dir ? Vector(0, delta, 0) : Vector(0, 0, delta);
    physics->moveBody(this, v);
    ++ticks;
  }

  int ticks = 0;
  int dir = 0;
};

