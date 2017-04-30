/**
 * @brief Game, as seen by the outside world
 * @author Sebastien Alaiwan
 */

/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once
#include <vector>

#include "util.h"
#include "geom.h"

using namespace std;

typedef int SOUND;
typedef int MODEL;

enum class Effect
{
  Normal,
  Blinking,
};

// a game object, as seen by the user-interface, i.e a displayable object.
struct Actor
{
  Actor(Vector3f pos_ = Vector3f(0, 0, 0), MODEL model_ = 0)
  {
    model = model_;
    pos = pos_;
  }

  Vector3f pos;
  Vector3f orientation = Vector3f(1, 0, 0);
  MODEL model = 0;
  int action = 0;
  float ratio = 0; // in [0 .. 1]
  Size3f scale = Size3f(1, 1, 1);
  Effect effect = Effect::Normal;
  bool focus = false; // is it the camera?
};

struct Control
{
  bool forward, backward;
  float look_horz = 0;
  float look_vert = 0;

  bool left, right;

  bool fire;
  bool dash;
  bool use;
  bool jump;

  bool restart; // restart level in case one gets stuck

  bool debug;
};

// outside world, seen by the game
struct View
{
  virtual void textBox(char const* msg) = 0;
};

// game, seen by the outside world

struct Scene
{
  virtual void tick(Control const& c) = 0;
  virtual int getMusic() const = 0;
  virtual vector<Actor> getActors() const = 0;
  virtual vector<SOUND> readSounds() = 0;

  float ambientLight = 0;
};

