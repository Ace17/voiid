// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Game, as seen by the outside world

#pragma once
#include <vector>

#include "util.h"
#include "geom.h"

using namespace std;

typedef int MODEL;

enum class Effect
{
  Normal,
  Blinking,
};

// a displayable object (= a game object, as seen by the user-interface)
struct Actor
{
  Actor(Vector3f pos_ = Vector3f(0, 0, 0), MODEL model_ = 0)
  {
    model = model_;
    pos = pos_;
  }

  Vector3f pos; // object position, in logical units
  Vector3f orientation = Vector3f(1, 0, 0);
  MODEL model = 0; // what sprite to display
  int action = 0; // what sprite action to use
  float ratio = 0; // in [0 .. 1]. 0 for action beginning, 1 for action end
  Size3f scale = Size3f(1, 1, 1); // sprite size
  Effect effect = Effect::Normal;
  bool focus = false; // is it the camera?
};

struct Control
{
  // player directions
  bool forward, backward;
  bool left, right;
  float look_horz = 0;
  float look_vert = 0;

  // player actions
  bool fire;
  bool jump;
  bool dash;
  bool use;

  bool restart; // kill the player (in case of getting stuck)

  bool debug; // toggle debug-mode
};

// game, seen by the outside world

struct Scene
{
  virtual ~Scene() = default;

  // advance the scene simulation to the next frame
  virtual void tick(Control const& c) = 0;

  // return a list of displayable objects for the current frame
  virtual vector<Actor> getActors() const = 0;

  float ambientLight = 0;
};

