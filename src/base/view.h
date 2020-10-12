// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Outside world, as seen by the game

#pragma once

#include "geom.h"
#include "resource.h"

enum class Effect
{
  Normal,
  Blinking,
};

// a displayable object (= a game object, as seen by the user-interface)
struct Actor
{
  Actor(Vector3f pos_ = Vector3f(0, 0, 0), int model_ = 0)
  {
    model = model_;
    pos = pos_;
  }

  Vector3f pos; // object position, in logical units
  Quaternion orientation = Quaternion::rotation(Vector3f(1, 0, 0), 0);
  int model = 0; // what sprite to display
  int action = 0; // what sprite action to use
  float ratio = 0; // in [0 .. 1]. 0 for action beginning, 1 for action end
  Size3f scale = Size3f(1, 1, 1); // sprite size
  Effect effect = Effect::Normal;
  bool focus = false; // is it the camera?
};

// This interface should act as a message sink.
// It should provide no way to query anything about the outside world.
struct View
{
  virtual ~View() = default;

  virtual void setTitle(char const* gameTitle) = 0;
  virtual void preload(Resource res) = 0;
  virtual void textBox(char const* msg) = 0;
  virtual void playMusic(int id) = 0;
  virtual void stopMusic() = 0;
  virtual void playSound(int id) = 0;
  virtual void setCameraPos(Vector3f pos, Quaternion orientation) = 0;
  virtual void setAmbientLight(float amount) = 0;

  // adds a displayable object to the current frame
  virtual void sendActor(Actor const& actor) = 0;
};

