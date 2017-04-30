#pragma once

#include "base/util.h"
#include "base/scene.h"
#include "game/entity.h"
#include "game/models.h"

struct Sign : Entity
{
  Sign(int which_) : which(which_)
  {
    size = Size2f(4, 2);
    solid = 0;
    collisionGroup = 0;
    collidesWith = 0;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_SIGN);
    r.scale = size * 2;
    r.pos = pos - Vector2f(r.scale.width / 2, r.scale.height / 2);
    r.ratio = 0;
    r.action = which;

    return r;
  }

  int const which;
};

