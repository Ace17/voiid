#pragma once

#include <stdlib.h> // rand
#include <algorithm>

#include "base/util.h"
#include "base/scene.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "entities/explosion.h"

struct Hopper : Entity, Damageable
{
  Hopper()
  {
    dir = -1.0f;
    size = UnitSize * 0.5;
    collisionGroup = CG_WALLS;
    collidesWith = CG_SOLIDPLAYER;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_RECT);

    r.scale = size;

    if(blinking)
      r.effect = Effect::Blinking;

    r.action = 0;
    r.ratio = (time % 800) / 800.0f;

    return r;
  }

  virtual void tick() override
  {
    ++time;

    vel.y -= 0.00005; // gravity

    if(ground && time % 500 == 0 && rand() % 4 == 0)
    {
      vel.y = 0.013;
      ground = false;
    }

    if(ground)
      vel.x = 0;
    else
      vel.x = dir * 0.003;

    auto trace = slideMove(this, vel);

    if(!trace.tx || !trace.ty)
      dir = -dir;

    if(!trace.tz)
    {
      ground = true;
      vel.y = 0;
    }

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
  bool ground = false;
  float dir;
};

