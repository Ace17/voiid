// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// 'end-of-level' touch-trigger entity

#include "base/scene.h"
#include "base/util.h"
#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h"
#include "gameplay/sounds.h"
#include "gameplay/toggle.h"

#include "collision_groups.h"

struct FinishLine : Entity
{
  FinishLine()
  {
    size = Size(2, 2, 2);
    solid = false;
    collisionGroup = 0; // dont' trigger other detectors
    collidesWith = CG_PLAYER | CG_SOLIDPLAYER;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_TELEPORTER);
    r.scale = size;
    r.effect = touchDelay > 0 ? Effect::Blinking : Effect::Normal;
    view->sendActor(r);

    if(touchDelay)
      view->sendLight({ pos + Vector3f(0.5, 0.5, 1), Vector3f(5, 5, 5) });
  }

  virtual void tick() override
  {
    if(decrement(touchDelay))
      game->endLevel();
  }

  virtual void onCollide(Entity*) override
  {
    if(touchDelay)
      return;

    game->playSound(SND_TELEPORT);
    touchDelay = 100;
  }

  int id = 0;
  int touchDelay = 0;
};

static auto const reg3_ = registerEntity("finish", [] (IEntityConfig*) -> unique_ptr<Entity> { return make_unique<FinishLine>(); });

