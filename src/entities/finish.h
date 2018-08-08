// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Entity for 'end-of-level'

#pragma once

#include "base/util.h"
#include "base/scene.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "toggle.h"
#include "collision_groups.h"

struct TouchFinishLineEvent : Event
{
};

struct FinishLine : Entity
{
  FinishLine()
  {
    size = Size(2, 2, 2);
    solid = false;
    collisionGroup = 0; // dont' trigger other detectors
    collidesWith = CG_PLAYER | CG_SOLIDPLAYER;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_TELEPORTER);
    r.scale = size;
    r.effect = touchDelay > 0 ? Effect::Blinking : Effect::Normal;
    return r;
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
    touchDelay = 1000;
  }

  int id = 0;
  int touchDelay = 0;
};

