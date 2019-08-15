#pragma once

#include "base/scene.h"
#include "collision_groups.h"
#include "entity.h"
#include "models.h"

struct Conveyor : Entity
{
  Conveyor()
  {
    size = UnitSize;
    collisionGroup = CG_WALLS;
    collidesWith = CG_PLAYER;
    solid = 1;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.action = 2;
    r.scale = size;
    view->sendActor(r);
  }

  virtual void enter() override
  {
    Body::onCollision = [this] (Body* other) { touch(other); };
  }

  void touch(Body* other)
  {
    // avoid infinite recursion
    // (if the conveyor pushes the player towards the conveyor)
    if(noRecurse)
      return;

    noRecurse = true;
    physics->moveBody(other, Vector(-0.004, 0, 0));
    noRecurse = false;
  }

  bool noRecurse = false;
};

