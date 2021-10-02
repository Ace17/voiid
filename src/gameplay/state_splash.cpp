// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// splash menu

#include "base/scene.h"
#include "base/view.h"
#include <memory>

#include "models.h" // MDL_SPLASH
#include "sounds.h" // SND_PAUSE
#include "state_machine.h"
#include "toggle.h"

static const float LIGHT_ON = 4;
static const float LIGHT_OFF = 0;

template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

struct SplashState : Scene
{
  SplashState(View* view_) : view(view_)
  {
    view->setAmbientLight(LIGHT_ON);

    for(auto& value : randomSequence)
      value = (rand() % 2) / 2.0;
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  Scene* tick(Control c) override
  {
    time++;
    auto const FADE_TIME = 200;

    view->playMusic(1);

    if(!activated)
    {
      if(c.fire || c.jump || c.dash)
      {
        activated = true;
        delay = FADE_TIME;
        view->playSound(SND_SPARK);
      }
    }

    if(activated)
    {
      const float alpha = clamp(1.0 - delay / float(FADE_TIME), 0.0, 1.0);
      view->setAmbientLight(blend(LIGHT_ON, LIGHT_OFF, alpha));

      if(decrement(delay))
      {
        view->textBox("");
        activated = false;
        return createPlayingState(view);
      }
    }
    else
    {
      int idx = (time / 10) % (sizeof(randomSequence) / sizeof(*randomSequence));
      view->setAmbientLight(randomSequence[idx] * 2 + 0.1);
    }

    return this;
  }

  void draw() override
  {
    view->setCameraPos(Vector3f(0, 0, 0), Quaternion::fromEuler(0, 0, 0));
    Actor panel = Actor(Vector3f(2.5, 0, +0.15), MDL_SPLASH);
    view->sendActor(panel);
  }

private:
  View* const view;
  bool activated = false;
  int delay = 0;
  int time = 0;
  float randomSequence[128];
};

Scene* createSplashState(View* view)
{
  return new SplashState(view);
}

