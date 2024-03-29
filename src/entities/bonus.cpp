// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Bonus entity

#include <algorithm>
#include <cmath> // sin

#include "base/scene.h"
#include "base/util.h"
#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h"
#include "gameplay/player.h"
#include "gameplay/sounds.h"

struct Bonus : Entity
{
  Bonus(int modelAction_, int type_, String msg_)
  {
    modelAction = modelAction_;
    type = type_;
    msg = msg_;
    size = UnitSize;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor { pos, MDL_BONUS };
    r.scale = UnitSize;
    r.action = modelAction;
    view->sendActor(r);
  }

  virtual void tick() override
  {
    ++time;
  }

  void onCollide(Entity* other) override
  {
    if(dead)
      return;

    if(auto player = dynamic_cast<Player*>(other))
    {
      player->addUpgrade(type);
      game->playSound(SND_BONUS);
      game->textBox(msg);
      dead = true;
    }
  }

  int time = 0;
  int modelAction;
  int type;
  String msg;
};

std::unique_ptr<Entity> makeBonus(int action, int upgradeType, String msg)
{
  return std::make_unique<Bonus>(action, upgradeType, msg);
}

static auto const reg1 = registerEntity("upgrade_climb", [] (IEntityConfig*) { return makeBonus(4, UPGRADE_CLIMB, "jump while against wall"); });
static auto const reg2 = registerEntity("upgrade_shoot", [] (IEntityConfig*) { return makeBonus(3, UPGRADE_SHOOT, "press Z"); });
static auto const reg3 = registerEntity("upgrade_dash", [] (IEntityConfig*) { return makeBonus(5, UPGRADE_DASH, "press C while walking"); });
static auto const reg4 = registerEntity("upgrade_djump", [] (IEntityConfig*) { return makeBonus(6, UPGRADE_DJUMP, "jump while airborne"); });
static auto const reg5 = registerEntity("upgrade_ball", [] (IEntityConfig*) { return makeBonus(7, UPGRADE_BALL, "press down"); });
static auto const reg6 = registerEntity("upgrade_slide", [] (IEntityConfig*) { return makeBonus(8, UPGRADE_SLIDE, "go against wall while falling"); });
static auto const reg7 = registerEntity("bonus", [] (IEntityConfig*) { return makeBonus(0, 0, "life up"); });

