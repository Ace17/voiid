// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "base/resource.h"
#include "sounds.h"
#include "models.h"

static const Resource resources[] =
{
  { ResourceType::Sound, SND_PAUSE, "res/sounds/pause.ogg" },
  { ResourceType::Sound, SND_FIRE, "res/sounds/fire.ogg" },
  { ResourceType::Sound, SND_JUMP, "res/sounds/jump.ogg" },
  { ResourceType::Sound, SND_LAND, "res/sounds/land.ogg" },
  { ResourceType::Sound, SND_SWITCH, "res/sounds/switch.ogg" },
  { ResourceType::Sound, SND_DOOR, "res/sounds/door.ogg" },
  { ResourceType::Sound, SND_HURT, "res/sounds/hurt.ogg" },
  { ResourceType::Sound, SND_DIE, "res/sounds/die.ogg" },
  { ResourceType::Sound, SND_BONUS, "res/sounds/bonus.ogg" },
  { ResourceType::Sound, SND_DAMAGE, "res/sounds/damage.ogg" },
  { ResourceType::Sound, SND_EXPLODE, "res/sounds/explode.ogg" },
  { ResourceType::Sound, SND_DISAPPEAR, "res/sounds/disappear.ogg" },
  { ResourceType::Sound, SND_TELEPORT, "res/sounds/teleport.ogg" },

  { ResourceType::Model, MDL_DOOR, "res/sprites/door.json" },
  { ResourceType::Model, MDL_RECT, "res/sprites/rect.json" },
  { ResourceType::Model, MDL_AMULET, "res/sprites/amulet.json" },
  { ResourceType::Model, MDL_INVRECT, "res/sprites/invrect.json" },
  { ResourceType::Model, MDL_SWITCH, "res/sprites/switch.json" },
  { ResourceType::Model, MDL_WHEEL, "res/sprites/wheel.json" },
  { ResourceType::Model, MDL_LIFEBAR, "res/sprites/lifebar.json" },
  { ResourceType::Model, MDL_TELEPORTER, "res/sprites/teleporter.json" },
  { ResourceType::Model, MDL_BONUS, "res/sprites/bonus.json" },
  { ResourceType::Model, MDL_BULLET, "res/sprites/bullet.json" },
  { ResourceType::Model, MDL_EXPLOSION, "res/sprites/explosion.json" },
  { ResourceType::Model, MDL_SPIKES, "res/sprites/spikes.json" },
  { ResourceType::Model, MDL_SIGN, "res/sprites/sign.json" },
};

Span<const Resource> getResources()
{
  return resources;
}

