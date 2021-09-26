// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include <algorithm>

#include "base/scene.h"
#include "base/util.h"
#include "gameplay/entity.h"
#include "gameplay/models.h"
#include "gameplay/player.h"
#include "gameplay/sounds.h"
#include "gameplay/toggle.h"

#include "collision_groups.h"
#include "hero.h"
#include "move.h"

auto const GRAVITY = 0.005;
auto const JUMP_SPEED = 0.15;
auto const WALK_SPEED = 0.075f;
auto const MAX_HORZ_SPEED = 0.2f;
auto const MAX_FALL_SPEED = 0.2f;
auto const HURT_DELAY = 50;
auto const STAIR_CLIMB = 0.5;

static auto const NORMAL_SIZE = Size(0.7, 0.7, 1.5);

struct Hero : Player, Damageable
{
  Hero()
  {
    size = NORMAL_SIZE;
  }

  void enter() override
  {
    respawnPoint = pos;
  }

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_INVRECT);
    r.scale = size;
    r.orientation = Quaternion::fromEuler(lookAngleHorz, -lookAngleVert, 0);

    if(0) // hide debug box
      view->sendActor(r);

    auto eyesPos = r.pos + Vector3f(
      r.scale.cx * 0.5,
      r.scale.cy * 0.5,
      r.scale.cz * 0.9);

    view->setCameraPos(eyesPos, r.orientation);
  }

  void think(Control const& c) override
  {
    control = c;
  }

  float health() override
  {
    return ::clamp(life / 31.0f, 0.0f, 1.0f);
  }

  virtual void addUpgrade(int upgrade) override
  {
    upgrades |= upgrade;
    blinking = 2000;
    life = 31;
  }

  void computeVelocity(Control c)
  {
    airMove(c);

    vel.z -= GRAVITY;

    if(jumpbutton.toggle(c.jump) && ground)
    {
      game->playSound(SND_JUMP);
      vel.z = JUMP_SPEED;
    }

    // stop jump if the player release the button early
    if(vel.z > 0 && !c.jump)
      vel.z = 0;

    vel.x = ::clamp(vel.x, -MAX_HORZ_SPEED, MAX_HORZ_SPEED);
    vel.y = ::clamp(vel.y, -MAX_HORZ_SPEED, MAX_HORZ_SPEED);
    vel.z = max(vel.z, -MAX_FALL_SPEED);
  }

  void airMove(Control c)
  {
    auto const forward = vectorFromAngles(lookAngleHorz, 0);
    auto const left = vectorFromAngles(lookAngleHorz + PI / 2, 0);

    Vector wantedVel = Vector(0, 0, 0);

    if(c.backward)
      wantedVel -= forward * WALK_SPEED;

    if(c.forward)
      wantedVel += forward * WALK_SPEED;

    if(c.left)
      wantedVel += left * WALK_SPEED;

    if(c.right)
      wantedVel -= left * WALK_SPEED;

    vel.x = vel.x * 0.90 + wantedVel.x * 0.10;
    vel.y = vel.y * 0.90 + wantedVel.y * 0.10;

    if(abs(vel.x) < 0.0001)
      vel.x = 0;

    if(abs(vel.y) < 0.0001)
      vel.y = 0;

    if(abs(vel.z) < 0.0001)
      vel.z = 0;
  }

  virtual void tick() override
  {
    decrement(blinking);
    decrement(hurtDelay);

    if(hurtDelay || life <= 0)
      control = Control {};

    lookAngleVert -= control.look_vert * 1.0;
    lookAngleHorz -= control.look_horz * 1.0;

    lookAngleHorz = fmod(lookAngleHorz, PI * 2.0);
    lookAngleVert = ::clamp<float>(lookAngleVert, -PI * 0.4, PI * 0.4);

    if(decrement(respawnDelay))
    {
      pos = respawnPoint;
      life = 31;
      blinking = 200;
    }

    computeVelocity(control);

    physics->moveBody(this, Vector3f(0, 0, STAIR_CLIMB));
    slideMove(physics, this, vel);
    physics->moveBody(this, Vector3f(0, 0, -STAIR_CLIMB));

    auto const onGround = isOnGround(physics, this);

    if(!onGround)
    {
      ground = false;
    }
    else
    {
      if(vel.z < 0 && !ground)
      {
        if(tryActivate(debounceLanding, 15))
          game->playSound(SND_LAND);

        ground = true;
        vel.z = 0;
      }
    }

    decrement(debounceLanding);
    decrement(debounceUse);

    if(control.use && debounceUse == 0)
    {
      debounceUse = 200;

      // look in front of us for a body to switch,
      // and switch it.
      auto const forward = vectorFromAngles(lookAngleHorz, 0);
      Box box = getBox();
      auto body = physics->traceBox(box, forward, this).blocker;

      if(auto switchable = dynamic_cast<Switchable*>(body))
        switchable->onSwitch();
    }

    if(control.restart)
      onDamage(10000);

    collisionGroup = CG_PLAYER;

    if(!blinking)
      collisionGroup |= CG_SOLIDPLAYER;

    if(respawnDelay == 0)
      if(pos.z < -15.0)
        die();
  }

  virtual void onDamage(int amount) override
  {
    if(life <= 0)
      return;

    if(!blinking)
    {
      life -= amount;

      if(life < 0)
      {
        die();
        return;
      }

      hurtDelay = HURT_DELAY;
      blinking = 200;
      game->playSound(SND_HURT);
    }
  }

  void die()
  {
    game->playSound(SND_DIE);
    respawnDelay = 100;
    game->textBox("game over");
  }

  int debounceLanding = 0;
  int debounceUse = 0;
  float lookAngleHorz = 0;
  float lookAngleVert = 0;
  bool ground = false;
  Toggle jumpbutton, firebutton;
  int hurtDelay = 0;
  int life = 31;
  Control control {};

  int respawnDelay = 0;
  Vector respawnPoint;
  Vector vel;

  int upgrades = 0;
};

std::unique_ptr<Player> makeHero()
{
  return make_unique<Hero>();
}

