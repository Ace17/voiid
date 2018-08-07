// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "entity.h"
#include "models.h" // MDL_DOOR
#include "sounds.h" // SND_DOOR
#include "toggle.h" // decrement
#include "trigger.h" // TriggerEvent
#include "collision_groups.h" // CG_WALLS

struct Door : Entity, IEventSink
{
  Door(int id_) : id(id_)
  {
    size = Size3f(0.5, 2, 2);
    solid = true;
  }

  void enter() override
  {
    subscription = game->subscribeForEvents(this);
  }

  void leave() override
  {
    subscription.reset();
  }

  virtual void tick() override
  {
    decrement(openingDelay);

    if(openingDelay > 0)
    {
      auto sign = state ? 1 : -1;
      physics->moveBody(this, Up * 0.003 * sign);
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
  unique_ptr<Handle> subscription;
};

unique_ptr<Entity> makeDoor(int id)
{
  return make_unique<Door>(id);
}

///////////////////////////////////////////////////////////////////////////////

struct AutoDoor : Entity, Switchable
{
  AutoDoor()
  {
    size = Size3f(0.5, 2, 2);
    solid = true;
  }

  virtual void enter()
  {
    basePos = pos;
  }

  virtual void tick() override
  {
    switch(state)
    {
    case State::Closed:
      break;
    case State::Opening:
      {
        if(pos.z - basePos.z < 2.3)
          physics->moveBody(this, Up * 0.004);
        else
          state = State::Open;

        break;
      }
    case State::Open:
      {
        if(decrement(timer))
        {
          game->playSound(SND_DOOR);
          state = State::Closing;
        }

        break;
      }
    case State::Closing:
      {
        if(pos.z - basePos.z > 0.001)
          physics->moveBody(this, Down * 0.004);
        else
          state = State::Closed;
      }
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

  virtual void onSwitch() override
  {
    if(state != State::Closed)
      return;

    game->playSound(SND_DOOR);
    state = State::Opening;
    timer = 1500;
  }

  enum class State
  {
    Closed,
    Opening,
    Open,
    Closing,
  };

  State state = State::Closed;
  Vector basePos;
  int timer = 0;
};

unique_ptr<Entity> makeAutoDoor()
{
  return make_unique<AutoDoor>();
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

