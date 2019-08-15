#pragma once

#include "base/util.h"
#include "base/scene.h"
#include "entity.h"
#include "models.h"

struct Spikes : Entity
{
  Spikes()
  {
    size = Size(1, 1, 0.95);
    solid = 1;
    collisionGroup = CG_WALLS;
    collidesWith = CG_SOLIDPLAYER;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    r.ratio = 0;
    view->sendActor(r);
  }

  void onCollide(Entity* other) override
  {
    if(auto damageable = dynamic_cast<Damageable*>(other))
      damageable->onDamage(1000);
  }
};

