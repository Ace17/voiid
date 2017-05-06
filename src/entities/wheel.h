#pragma once

#include <algorithm>

#include "base/util.h"
#include "base/scene.h"
#include "collision_groups.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "toggle.h"
#include "entities/explosion.h"
#include "entities/move.h"

struct Wheel : Entity, Damageable
{
  Wheel()
  {
    dir = -1.0f;
    size = UnitSize * 1.5;
    collisionGroup = CG_WALLS;
    collidesWith = CG_SOLIDPLAYER;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_WHEEL);

    r.scale = UnitSize * 3;

    if(blinking)
      r.effect = Effect::Blinking;

    r.action = 0;
    r.ratio = (time % 200) / 200.0f;

    return r;
  }

  virtual void tick() override
  {
    ++time;

    vel.x = dir * 0.003;
    vel.y -= 0.00005; // gravity

    auto trace = slideMove(this, vel);

    if(!trace.tx || !trace.ty)
      dir = -dir;

    if(!trace.tz)
      vel.y = 0;

    decrement(blinking);
  }

  virtual void onCollide(Entity* other) override
  {
    if(auto damageable = dynamic_cast<Damageable*>(other))
      damageable->onDamage(5);
  }

  virtual void onDamage(int amount) override
  {
    blinking = 100;
    life -= amount;

    game->playSound(SND_DAMAGE);

    if(life < 0)
    {
      game->playSound(SND_EXPLODE);
      dead = true;

      auto explosion = makeExplosion();
      explosion->pos = getCenter();
      game->spawn(explosion.release());
    }
  }

  int life = 30;
  int time = 0;
  float dir;
};

