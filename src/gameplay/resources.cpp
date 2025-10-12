// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "base/resource.h"
#include "base/span.h"
#include "models.h"
#include "sounds.h"

static const Resource resources[] =
{
  { ResourceType::Sound, SND_PAUSE, "res/sounds/pause.ogg" },
  { ResourceType::Sound, SND_FIRE, "res/sounds/fire.ogg" },
  { ResourceType::Sound, SND_JUMP, "res/sounds/jump.ogg" },
  { ResourceType::Sound, SND_LAND, "res/sounds/land.ogg" },
  { ResourceType::Sound, SND_SPARK, "res/sounds/electric.ogg" },
  { ResourceType::Sound, SND_SWITCH, "res/sounds/switch.ogg" },
  { ResourceType::Sound, SND_DOOR, "res/sounds/door.ogg" },
  { ResourceType::Sound, SND_HURT, "res/sounds/hurt.ogg" },
  { ResourceType::Sound, SND_DIE, "res/sounds/die.ogg" },
  { ResourceType::Sound, SND_BONUS, "res/sounds/bonus.ogg" },
  { ResourceType::Sound, SND_DAMAGE, "res/sounds/damage.ogg" },
  { ResourceType::Sound, SND_EXPLODE, "res/sounds/explode.ogg" },
  { ResourceType::Sound, SND_DISAPPEAR, "res/sounds/disappear.ogg" },
  { ResourceType::Sound, SND_TELEPORT, "res/sounds/teleport.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_1, "res/sounds/footstep_1.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_2, "res/sounds/footstep_2.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_3, "res/sounds/footstep_3.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_4, "res/sounds/footstep_4.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_5, "res/sounds/footstep_5.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_6, "res/sounds/footstep_6.ogg" },
  { ResourceType::Sound, SND_FOOTSTEP_7, "res/sounds/footstep_7.ogg" },
  { ResourceType::Sound, SND_FRAGMENT_BEEP, "res/sounds/fragment_beep.ogg" },

  { ResourceType::Model, MDL_SPLASH, "res/sprites/splash.render" },
  { ResourceType::Model, MDL_ENDING, "res/sprites/ending.render" },
  { ResourceType::Model, MDL_DOOR, "res/sprites/door.render" },
  { ResourceType::Model, MDL_RECT, "res/sprites/rect.render" },
  { ResourceType::Model, MDL_CUBE, "res/sprites/cube.render" },
  { ResourceType::Model, MDL_INVRECT, "res/sprites/invrect.render" },
  { ResourceType::Model, MDL_SWITCH, "res/sprites/switch.render" },
  { ResourceType::Model, MDL_TELEPORTER, "res/sprites/teleporter.render" },
  { ResourceType::Model, MDL_BONUS, "res/sprites/bonus.render" },
  { ResourceType::Model, MDL_FRAGMENT, "res/sprites/fragment.render" },
};

extern const Span<const Resource> AllResources = resources;

