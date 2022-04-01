// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/sounds.h"
#include "gameplay/toggle.h" // decrement
#include "gameplay/trigger.h" // TriggerEvent

namespace
{
struct Lamp : Entity, IEventSink
{
  Lamp(int link_) : link(link_)
  {
    size = Size3f(0.1, 0.1, 0.1);
    solid = false;
  }

  void enter() override
  {
    subscription = game->subscribeForEvents(this);
  }

  void leave() override
  {
    subscription.reset();
  }

  virtual void tick() override { ++ticks; }

  virtual void onDraw(View* view) const override
  {
    if(enabled)
    {
      if(ticks < 5
         || (ticks > 40 && ticks < 50)
         || ticks > 80)
        view->sendLight({ pos, Vector3f(0.39, 0.65, 0.77) });
    }
  }

  virtual void notify(const Event* evt) override
  {
    if(enabled)
      return;

    if(auto trg = evt->as<TriggerEvent>())
    {
      if(trg->link != link)
        return;

      enabled = true;
      game->playSound(SND_SPARK);
      ticks = 0;
    }
  }

  int ticks = 0;
  bool enabled = false;
  const int link;
  unique_ptr<Handle> subscription;
};

unique_ptr<Entity> makeLight(int link)
{
  return make_unique<Lamp>(link);
}

static auto const reg1 = registerEntity("lamp", [] (IEntityConfig* args) { auto arg = args->getInt("0"); return makeLight(arg); });
static auto const reg2 = registerEntity("light", [] (IEntityConfig* args) { auto arg = args->getInt("0"); return makeLight(arg); });
}

