// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h" // MDL_DOOR
#include "gameplay/sounds.h" // SND_DOOR
#include "gameplay/toggle.h" // decrement
#include "gameplay/trigger.h" // TriggerEvent

#include "collision_groups.h" // CG_WALLS

struct Door : Entity, IEventSink
{
  Door(int link_) : link(link_)
  {
    size = Vec3f(0.5, 2, 2);
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
      physics->moveBody(this, Up * 0.03 * sign);
    }
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_DOOR);
    r.action = 1;
    r.scale = size;
    view->sendActor(r);
  }

  virtual void notify(const Event* evt) override
  {
    if(auto trg = evt->as<TriggerEvent>())
    {
      if(trg->link != link)
        return;

      game->playSound(SND_DOOR);
      state = !state;

      if(state)
        openingDelay = 100;
      else
        solid = true;
    }
  }

  bool state = false;
  int openingDelay = 0;
  const int link;
  unique_ptr<Handle> subscription;
};

unique_ptr<Entity> makeDoor(int link)
{
  return make_unique<Door>(link);
}

///////////////////////////////////////////////////////////////////////////////

struct AutoDoor : Entity, Switchable
{
  AutoDoor()
  {
    size = Vec3f(0.5, 2, 2);
    solid = true;
  }

  virtual void enter() override
  {
    pos -= size * 0.5;
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
          physics->moveBody(this, Up * 0.04);
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
          physics->moveBody(this, Down * 0.04);
        else
          state = State::Closed;
      }
    }
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_DOOR);
    r.action = 1;
    r.scale = size;
    view->sendActor(r);
  }

  virtual void onSwitch() override
  {
    if(state != State::Closed)
      return;

    game->playSound(SND_DOOR);
    state = State::Opening;
    timer = 150;
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

#include "explosion.h"

struct BreakableDoor : Entity, Damageable
{
  BreakableDoor()
  {
    size = UnitSize;
    solid = true;
    collisionGroup = CG_WALLS;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_DOOR);
    r.scale = size;

    if(blinking)
      r.effect = Effect::Blinking;

    view->sendActor(r);
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

static auto const reg1 = registerEntity("auto_door", [] (IEntityConfig*) { return makeAutoDoor(); });
static auto const reg2 = registerEntity("door", [] (IEntityConfig* args) { auto arg = args->getInt("link"); return makeDoor(arg); });

