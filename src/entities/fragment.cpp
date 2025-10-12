// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "base/scene.h"
#include "gameplay/entity.h"
#include "gameplay/entity_factory.h"
#include "gameplay/models.h"
#include "gameplay/player.h"
#include "gameplay/sounds.h"
#include <math.h> // sin

namespace
{
float pulse(float phase)
{
  if(phase < 0.5)
    return 1;
  else
    return 1 - (phase - 0.5) / 0.5;
}

struct Fragment : Entity
{
  Fragment()
  {
    size = Size(1, 1, 1) * 0.5;
    solid = false;
  }

  void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_FRAGMENT);
    r.scale = size;
    r.orientation = Quaternion::fromEuler(yaw, pitch, 0);
    view->sendActor(r);
    view->sendLight({ pos, Vec3f(0.1, 0.2, 0.77) * pulse(phase) * 0.003 });
  }

  void enter() override
  {
    Entity::enter();
    phase = (rand() % 1000) / 1000.0f;
  }

  void tick() override
  {
    yaw += 0.004;
    pitch += 0.006;
    phase += 0.005;

    while(phase >= 1.0)
    {
      phase -= 1.0;
      game->playSound(SND_FRAGMENT_BEEP, &pos);
    }
  }

  void onCollide(Entity* other) override
  {
    if(dead)
      return;

    if(dynamic_cast<Player*>(other))
    {
      game->playSound(SND_BONUS);
      game->textBox("Got fragment");
      dead = true;
    }
  }

  float phase = 0;
  float yaw = 0;
  float pitch = 0;
};
}

static auto const reg = registerEntity("fragment", [] (IEntityConfig*) -> std::unique_ptr<Entity> { return std::make_unique<Fragment>(); });

