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

#include "game/collision_groups.h"
#include "game/entities/player.h"
#include "game/entities/move.h"
#include "game/entity.h"
#include "game/models.h"
#include "game/sounds.h"
#include "game/toggle.h"
#include "rockman.h"

auto const GRAVITY = 0.00015;
auto const WALK_SPEED = 0.0075f;
auto const MAX_HORZ_SPEED = 0.02f;
auto const MAX_FALL_SPEED = 100.0f;
auto const HURT_DELAY = 500;

enum ORIENTATION
{
  LEFT,
  RIGHT,
};

struct Bullet : Entity
{
  Bullet()
  {
    size = UnitSize * 0.4;
    collisionGroup = 0;
    collidesWith = CG_WALLS;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_BULLET);
    r.scale = size;
    r.action = 0;
    r.ratio = 0;

    return r;
  }

  void tick() override
  {
    pos += vel;
    decrement(life);

    if(life == 0)
      dead = true;
  }

  void onCollide(Entity* other) override
  {
    if(auto damageable = dynamic_cast<Damageable*>(other))
      damageable->onDamage(10);

    dead = true;
  }

  int life = 1000;
};

static auto const NORMAL_SIZE = Size(0.7, 0.7, 1.5);

struct Rockman : Player, Damageable
{
  Rockman()
  {
    size = NORMAL_SIZE;
  }

  void enter() override
  {
    respawnPoint = pos;
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_ROCKMAN);
    r.scale = UnitSize * 0;
    r.focus = true;

    r.pos.z += 1.5;

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
    return clamp(life / 31.0f, 0.0f, 1.0f);
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

    if(ground)
      doubleJumped = false;

    vel.z -= GRAVITY;

    sliding = false;

    if(jumpbutton.toggle(c.jump))
    {
      if(ground)
      {
        game->playSound(SND_JUMP);
        vel.z = 0.1;
        doubleJumped = false;
      }
      else if((upgrades & UPGRADE_DJUMP) && !doubleJumped)
      {
        game->playSound(SND_JUMP);
        vel.z = 0.1;
        doubleJumped = true;
      }
    }

    // stop jump if the player release the button early
    if(vel.z > 0 && !c.jump)
      vel.z = 0;

    vel.x = clamp(vel.x, -MAX_HORZ_SPEED, MAX_HORZ_SPEED);
    vel.y = clamp(vel.y, -MAX_HORZ_SPEED, MAX_HORZ_SPEED);
    vel.z = max(vel.z, -MAX_FALL_SPEED);
  }

  void airMove(Control c)
  {
    auto const forward = Vector(cos(lookAngleHorz), sin(lookAngleHorz), 0);
    auto const left = Vector(cos(lookAngleHorz + M_PI / 2), sin(lookAngleHorz + M_PI / 2), 0);

    Vector wantedVel = Vector(0, 0, 0);

    if(!climbDelay)
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

    vel = vel * 0.95 + wantedVel * 0.05;

    if(abs(vel.x) < 0.00001)
      vel.x = 0;

    if(abs(vel.y) < 0.00001)
      vel.y = 0;
  }

  virtual void tick() override
  {
    decrement(blinking);
    decrement(hurtDelay);

    if(ground)
      decrement(dashDelay);

    if(hurtDelay || life <= 0)
      control = Control {};

    lookAngleVert = control.look_vert;
    lookAngleHorz = control.look_horz;

    if(decrement(respawnDelay))
    {
      pos = respawnPoint;
      life = 31;
      blinking = 2000;
    }

    time++;
    computeVelocity(control);

    auto trace = slideMove(this, vel);

    if(trace.tz)
    {
      ground = false;
    }
    else
    {
      if(vel.z < 0 && !ground)
      {
        if(tryActivate(debounceLanding, 150))
          game->playSound(SND_LAND);

        ground = true;
        dashDelay = 0;
      }

      vel.z = 0;
    }

    decrement(debounceFire);
    decrement(debounceLanding);
    decrement(climbDelay);
    decrement(shootDelay);

    if(upgrades & UPGRADE_SHOOT && !ball)
    {
      if(firebutton.toggle(control.fire) && tryActivate(debounceFire, 150))
      {
        auto b = make_unique<Bullet>();

        b->pos = pos;
        b->vel = Vector(0.025, 0, 0);
        game->spawn(b.release());
        game->playSound(SND_FIRE);
        shootDelay = 300;
      }
    }

    if(control.restart)
      onDamage(10000);

    collisionGroup = CG_PLAYER;

    if(!blinking)
      collisionGroup |= CG_SOLIDPLAYER;
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
      blinking = 2000;
      game->playSound(SND_HURT);
    }
  }

  void die()
  {
    game->playSound(SND_DIE);
    ball = false;
    respawnDelay = 1000;
  }

  int debounceFire = 0;
  int debounceLanding = 0;
  float lookAngleHorz = 0;
  float lookAngleVert = 0;
  bool ground = false;
  Toggle jumpbutton, firebutton, dashbutton;
  int time = 0;
  int climbDelay = 0;
  int hurtDelay = 0;
  int dashDelay = 0;
  int shootDelay = 0;
  int life = 31;
  bool doubleJumped = false;
  bool ball = false;
  bool sliding = false;
  Control control {};

  int respawnDelay = 0;
  Vector respawnPoint;

  int upgrades = 0;
};

std::unique_ptr<Player> makeRockman()
{
  return make_unique<Rockman>();
}

