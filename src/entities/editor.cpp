// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// "Editor-mode" player

#include <algorithm>

#include "base/scene.h"
#include "base/util.h"

#include "collision_groups.h"
#include "entities/move.h"
#include "entities/player.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "toggle.h"

auto const WALK_SPEED = 0.0075f;
auto const MAX_SPEED = 0.02f;

struct Editor : Player
{
  Editor(Matrix& tiles_) : tiles(tiles_)
  {
    size = Size(0.5, 0.5, 0.5);
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos - size * 0.5, MDL_INVRECT);
    r.scale = UnitSize * 0;
    r.focus = true;

    r.orientation = Quaternion::fromEuler(lookAngleHorz, lookAngleVert, 0);

    view->sendActor(r);
  }

  void think(Control const& c) override
  {
    control = c;
  }

  float health() override
  {
    return 1.0f;
  }

  virtual void addUpgrade(int /*upgrade*/) override
  {
  }

  void computeVelocity(Control c)
  {
    airMove(c);

    vel.x = clamp(vel.x, -MAX_SPEED, MAX_SPEED);
    vel.y = clamp(vel.y, -MAX_SPEED, MAX_SPEED);
    vel.z = clamp(vel.z, -MAX_SPEED, MAX_SPEED);
  }

  void airMove(Control c)
  {
    auto const forward = vectorFromAngles(lookAngleHorz, lookAngleVert);
    auto const left = vectorFromAngles(lookAngleHorz + PI / 2, 0);

    Vector wantedVel = Vector(0, 0, 0);

    {
      if(c.backward)
        wantedVel -= forward * WALK_SPEED;

      if(c.forward)
        wantedVel += forward * WALK_SPEED;

      if(c.left)
        wantedVel += left * WALK_SPEED;

      if(c.right)
        wantedVel -= left * WALK_SPEED;
    }

    vel.x = vel.x * 0.95 + wantedVel.x * 0.05;
    vel.y = vel.y * 0.95 + wantedVel.y * 0.05;
    vel.z = vel.z * 0.95 + wantedVel.z * 0.05;

    if(abs(vel.x) < 0.00001)
      vel.x = 0;

    if(abs(vel.y) < 0.00001)
      vel.y = 0;

    if(abs(vel.z) < 0.00001)
      vel.z = 0;
  }

  virtual void tick() override
  {
    lookAngleVert = control.look_vert;
    lookAngleHorz = control.look_horz;

    computeVelocity(control);

    slideMove(physics, this, vel);

    decrement(debounceFire);

    if(firebutton.toggle(control.jump) && tryActivate(debounceFire, 150))
    {
      auto const forward = vectorFromAngles(lookAngleHorz, lookAngleVert) * 2.0;

      auto const x = (int)(pos.x + forward.x);
      auto const y = (int)(pos.y + forward.y);
      auto const z = (int)(pos.z + forward.z);

      if(tiles.isInside(x, y, z))
      {
        auto t = tiles.get(x, y, z);
        tiles.set(x, y, z, t ? 0 : 1);
      }
    }

    collisionGroup = CG_PLAYER;
  }

  Vector vel;
  int debounceFire = 0;
  float lookAngleHorz = 0;
  float lookAngleVert = 0;
  Toggle firebutton;
  Control control {};

  Matrix& tiles;
};

std::unique_ptr<Player> makeEditor(Matrix& tiles)
{
  return make_unique<Editor>(tiles);
}

