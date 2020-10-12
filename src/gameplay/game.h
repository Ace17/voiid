// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Game, as seen by entities

#pragma once

#include "base/geom.h"
#include "base/scene.h"
#include "base/view.h"
#include <functional>
#include <memory>

typedef Matrix3<int> Matrix;
typedef Vector3f Vector;

struct Entity;

struct Event
{
  // force a vtable so we can dynamic_cast events
  virtual ~Event() = default;

  template<typename T>
  const T* as() const
  {
    return dynamic_cast<const T*>(this);
  }
};

struct TouchLevelBoundary : Event
{
  TouchLevelBoundary(int targetLevel_, Vector transform_)
  {
    targetLevel = targetLevel_;
    transform = transform_;
  }

  int targetLevel;
  Vector transform {};
};

struct IEventSink
{
  virtual void notify(const Event* evt) = 0;
};

struct Handle
{
  virtual ~Handle() = default;
};

struct IGame
{
  virtual ~IGame() = default;

  // visual
  virtual void textBox(char const* msg) = 0;
  virtual void playSound(int id) = 0;

  // logic
  virtual void spawn(Entity* e) = 0;
  virtual void postEvent(unique_ptr<Event> event) = 0;
  virtual unique_ptr<Handle> subscribeForEvents(IEventSink*) = 0;
  virtual Vector getPlayerPosition() = 0;
  virtual void endLevel() {}
};

