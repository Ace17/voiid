#pragma once

#include "base/scene.h"
#include "base/util.h"
#include "collision_groups.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "toggle.h" // decrement

struct CrumbleBlock : Entity
{
  CrumbleBlock()
  {
    size = UnitSize;
    collisionGroup = CG_WALLS;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    r.ratio = 0;
    r.action = 3;

    if(!solid)
      r.scale = UnitSize * 0.01;

    view->sendActor(r);
  }

  virtual void enter() override
  {
    Body::onCollision = [this] (Body* other) { touch(other); };
  }

  void touch(Body* other)
  {
    if(other->pos.z > pos.z + size.cz)
    {
      disappearTimer = 1000;
      game->playSound(SND_DISAPPEAR);
    }
  }

  void tick() override
  {
    decrement(disappearTimer);

    if(disappearTimer > 0)
    {
      collidesWith = 0;

      if(disappearTimer < 900)
        solid = 0;
    }
    else if(!physics->getBodiesInBox(getBox(), CG_PLAYER, false, this))
    {
      collidesWith = CG_PLAYER;
      solid = 1;
    }
  }

  int disappearTimer = 0;
};

struct FragileBlock : Entity, Damageable
{
  FragileBlock()
  {
    size = UnitSize;
    reappear();
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    r.ratio = 0;
    r.action = solid ? 4 : 1;

    view->sendActor(r);
  }

  virtual void onDamage(int) override
  {
    disappear();
    game->playSound(SND_DAMAGE);
  }

  void tick() override
  {
    if(decrement(disappearTimer))
    {
      reappear();

      if(physics->getBodiesInBox(getBox(), CG_PLAYER, false, this))
        disappear();
    }
  }

  void reappear()
  {
    collisionGroup = CG_WALLS;
    solid = 1;
  }

  void disappear()
  {
    disappearTimer = 3000;
    collisionGroup = 0;
    solid = 0;
  }

  int disappearTimer = 0;
};

