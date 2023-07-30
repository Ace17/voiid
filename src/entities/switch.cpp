// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include <algorithm>
#include <cmath>

#include "base/scene.h"
#include "base/util.h"
#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h"
#include "gameplay/sounds.h"
#include "gameplay/toggle.h"
#include "gameplay/trigger.h"

#include "collision_groups.h"
#include "move.h"

struct Switch : Entity, Switchable
{
  Switch(int id_) : link(id_)
  {
    size = UnitSize * 0.75;
    solid = true;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_SWITCH);
    r.scale = size;

    if(blinking)
      r.effect = Effect::Blinking;

    r.action = state ? 1 : 0;
    view->sendActor(r);
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
    evt->link = link;
    game->postEvent(std::move(evt));
  }

  bool state = false;
  const int link;
};

unique_ptr<Entity> makeSwitch(int link)
{
  return make_unique<Switch>(link);
}

struct DetectorSwitch : Entity
{
  DetectorSwitch(int id_)
  {
    link = id_;
    size = UnitSize;
    solid = false;
    collisionGroup = 0; // dont' trigger other detectors
    collidesWith = CG_PLAYER | CG_SOLIDPLAYER;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);
    r.scale = size;
    view->sendActor(r);
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
        evt->link = link;
        game->postEvent(std::move(evt));

        touchDelay = 100;
      };
  }

  int link = 0;
  int touchDelay = 0;
};

static auto const reg1 = registerEntity("switch",
                                        [] (IEntityConfig* args) { auto arg = args->getInt("link"); return makeSwitch(arg); }
                                        );

