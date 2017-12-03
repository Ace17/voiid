/**
 * @brief Switch and door.
 * @author Sebastien Alaiwan
 */

/*
 * Copyright (C) 2017 - Sebastien Alaiwan
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <algorithm>
#include <cmath>

#include "base/util.h"
#include "base/scene.h"
#include "collision_groups.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "move.h"
#include "toggle.h"

struct TriggerEvent : Event
{
  int idx;
};

struct Switch : Entity
{
  Switch(int id_) : id(id_)
  {
    size = UnitSize * 0.75;
    solid = true;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_SWITCH);
    r.scale = size;

    if(blinking)
      r.effect = Effect::Blinking;

    r.action = state ? 1 : 0;
    return r;
  }

  virtual void tick() override
  {
    blinking = max(0, blinking - 1);
  }

  virtual void enter() override
  {
    Body::onCollision = [this] (Body*) { touch(); };
  }

  void touch()
  {
    if(blinking || state)
      return;

    blinking = 1200;
    state = !state;
    game->playSound(SND_SWITCH);

    auto evt = make_unique<TriggerEvent>();
    evt->idx = id;
    game->postEvent(move(evt));
  }

  bool state = false;
  const int id;
};

unique_ptr<Entity> makeSwitch(int id)
{
  return make_unique<Switch>(id);
}

struct DetectorSwitch : Entity
{
  DetectorSwitch(int id_)
  {
    id = id_;
    size = UnitSize;
    solid = false;
    collisionGroup = 0; // dont' trigger other detectors
    collidesWith = CG_PLAYER | CG_SOLIDPLAYER;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    return r;
  }

  virtual void tick() override
  {
    decrement(touchDelay);
  }

  virtual void enter() override
  {
    Body::onCollision =
      [ = ] (Body*)
      {
        if(touchDelay)
          return;

        game->playSound(SND_SWITCH);

        auto evt = make_unique<TriggerEvent>();
        evt->idx = id;
        game->postEvent(move(evt));

        touchDelay = 1000;
      };
  }

  int id = 0;
  int touchDelay = 0;
};

struct Door : Entity, IEventSink
{
  Door(int id_) : id(id_)
  {
    size = Size3f(0.5, 2, 2);
    solid = true;
  }

  void enter() override
  {
    game->subscribeForEvents(this);
  }

  void leave() override
  {
    game->unsubscribeForEvents(this);
  }

  virtual void tick() override
  {
    decrement(openingDelay);

    if(openingDelay > 0)
    {
      auto sign = state ? 1 : -1;
      slideMove(this, Vector3f(0, 0, 0.002) * sign);
    }
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_DOOR);
    r.action = 1;
    r.ratio = 0;
    r.scale = size;
    return r;
  }

  virtual void notify(const Event* evt) override
  {
    if(auto trg = evt->as<TriggerEvent>())
    {
      if(trg->idx != id)
        return;

      game->playSound(SND_DOOR);
      state = !state;

      if(state)
        openingDelay = 1000;
      else
        solid = true;
    }
  }

  bool state = false;
  int openingDelay = 0;
  const int id;
};

unique_ptr<Entity> makeDoor(int id)
{
  return make_unique<Door>(id);
}

///////////////////////////////////////////////////////////////////////////////

#include "entities/explosion.h"

struct BreakableDoor : Entity, Damageable
{
  BreakableDoor()
  {
    size = UnitSize;
    solid = true;
    collisionGroup = CG_WALLS;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_DOOR);
    r.scale = size;

    if(blinking)
      r.effect = Effect::Blinking;

    return r;
  }

  virtual void tick() override
  {
    decrement(blinking);
  }

  virtual void onDamage(int amount) override
  {
    blinking = 200;
    life -= amount;

    if(life < 0)
    {
      game->playSound(SND_EXPLODE);
      dead = true;

      auto explosion = makeExplosion();
      explosion->pos = getCenter();
      game->spawn(explosion.release());
    }
    else
      game->playSound(SND_DAMAGE);
  }

  int life = 130;
};

unique_ptr<Entity> makeBreakableDoor()
{
  return make_unique<BreakableDoor>();
}

