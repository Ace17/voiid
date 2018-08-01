// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

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
#include "trigger.h"

struct Switch : Entity, Switchable
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
  }

  void onSwitch() override
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

