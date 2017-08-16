/*
 * Copyright (C) 2017 - Sebastien Alaiwan
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <algorithm>

#include "base/scene.h"
#include "base/util.h"

#include "collision_groups.h"
#include "entities/player.h"
#include "entities/move.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"
#include "toggle.h"
#include "rockman.h"

auto const WALK_SPEED = 0.0075f;
auto const MAX_SPEED = 0.02f;

struct Editor : Player
{
  Editor()
  {
    size = Size(0.5, 0.5, 0.5);
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_ROCKMAN);
    r.scale = UnitSize * 0;
    r.focus = true;

    r.pos.z += 1.0;

    r.orientation.x = cos(lookAngleHorz) * cos(lookAngleVert);
    r.orientation.y = sin(lookAngleHorz) * cos(lookAngleVert);
    r.orientation.z = sin(lookAngleVert);

    return r;
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
    auto const left = vectorFromAngles(lookAngleHorz + PI / 2, lookAngleVert);

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

  static Vector vectorFromAngles(float alpha, float beta)
  {
    auto const x = cos(alpha) * cos(beta);
    auto const y = sin(alpha) * cos(beta);
    auto const z = sin(beta);

    return Vector(x, y, z);
  }

  virtual void tick() override
  {
    lookAngleVert = control.look_vert;
    lookAngleHorz = control.look_horz;

    computeVelocity(control);

    slideMove(this, vel);

    decrement(debounceFire);

    if(firebutton.toggle(control.fire) && tryActivate(debounceFire, 150))
    {
    }

    collisionGroup = CG_PLAYER;
  }

  int debounceFire = 0;
  float lookAngleHorz = 0;
  float lookAngleVert = 0;
  Toggle jumpbutton, firebutton, dashbutton;
  Control control {};

  int respawnDelay = 0;

  int upgrades = 0;
};

std::unique_ptr<Player> makeEditor()
{
  return make_unique<Editor>();
}

