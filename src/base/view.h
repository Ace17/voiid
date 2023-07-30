// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Outside world, as seen by the game

#pragma once

#include "geom.h"
#include "quaternion.h"
#include "resource.h"
#include "string.h"

enum class Effect
{
  Normal,
  Blinking,
};

// a displayable object (= a game object, as seen by the user-interface)
struct Actor
{
  Actor(Vec3f pos_ = Vec3f(0, 0, 0), int model_ = 0)
  {
    model = model_;
    pos = pos_;
  }

  Vec3f pos; // object position, in logical units
  Quaternion orientation = Quaternion::rotation(Vec3f(1, 0, 0), 0);
  int model = 0; // what sprite to display
  int action = 0; // what sprite action to use
  Vec3f scale = Vec3f(1, 1, 1); // sprite size
  Effect effect = Effect::Normal;
};

struct LightActor
{
  Vec3f pos; // light position, in logical units
  Vec3f color;
};

// This interface should act as a message sink.
// It should provide no way to query anything about the outside world.
struct View
{
  virtual ~View() = default;

  virtual void setTitle(String gameTitle) = 0;
  virtual void preload(Resource res) = 0;
  virtual void textBox(String msg) = 0;
  virtual void playMusic(int id) = 0;
  virtual void stopMusic() = 0;
  virtual void playSound(int id) = 0;
  virtual void setCameraPos(Vec3f pos, Quaternion orientation) = 0;
  virtual void setAmbientLight(float amount) = 0;

  // adds a displayable object to the current frame
  virtual void sendLight(LightActor const& actor) = 0;
  virtual void sendActor(Actor const& actor) = 0;
};

