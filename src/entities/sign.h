#pragma once

#include "base/util.h"
#include "base/scene.h"
#include "entity.h"
#include "models.h"

struct Sign : Entity
{
  Sign(int which_) : which(which_)
  {
    size = Size(2, 1, 2);
    solid = 0;
    collisionGroup = 0;
    collidesWith = 0;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size * 2;
    r.pos = pos - r.scale * 0.5;
    r.ratio = 0;
    r.action = which;

    view->sendActor(r);
  }

  int const which;
};

