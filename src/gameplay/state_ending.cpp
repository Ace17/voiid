// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// ending screen

#include "base/scene.h"
#include "base/view.h"
#include <memory>

#include "models.h" // MDL_ENDING
#include "state_machine.h"
#include "toggle.h"
#include "vec.h"

static const float LIGHT_ON = 1;

struct EndingState : Scene
{
  EndingState(View* view_) : view(view_)
  {
    view->setAmbientLight(LIGHT_ON);
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  Scene* tick(Control c) override
  {
    auto const FADE_TIME = 500;

    view->playMusic(1);

    if(!activated)
    {
      delay = FADE_TIME;

      if(c.fire || c.jump || c.dash || c.use)
      {
        activated = true;
      }
    }

    // view->setAmbientLight(delay / float(FADE_TIME) - 1.0);

    if(activated)
    {
      time++;

      if(decrement(delay))
      {
        activated = false;
        return createSplashState(view);
      }
    }

    return this;
  }

  void draw() override
  {
    double t = time * 0.01;
    view->setCameraPos(Vec3f(0, 0, 0), Quaternion::fromEuler(0, 0, 0));
    Actor panel = Actor(Vec3f(2.5, 0, +0.15), MDL_ENDING);
    panel.scale = panel.scale * (1.0 / (1.0 + t));
    panel.orientation = Quaternion::fromEuler(0, 0, t * 5.0);
    view->sendActor(panel);
  }

private:
  View* const view;
  bool activated = false;
  int delay = 0;
  int time = 0;
};

Scene* createEndingState(View* view)
{
  return new EndingState(view);
}

