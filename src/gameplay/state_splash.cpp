// Copyright (C) 2018 - Sebastien Alaiwan
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

static const float LIGHT_ON = 10;
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
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  Scene* tick(Control c) override
  {
    auto const FADE_TIME = 200;

    view->playMusic(1);

    view->textBox("V O I I D");

    if(!activated)
    {
      if(c.fire || c.jump || c.dash)
      {
        activated = true;
        delay = FADE_TIME;
        view->playSound(SND_PAUSE);
      }
    }

    if(activated)
    {
      const float alpha = 1.0 - delay / float(FADE_TIME);
      view->setAmbientLight(blend(LIGHT_ON, LIGHT_OFF, alpha));

      if(decrement(delay))
      {
        view->textBox("");
        activated = false;
        return createPlayingState(view);
      }
    }

    return this;
  }

  void draw() override
  {
  }

private:
  View* const view;
  bool activated = false;
  int delay = 0;
};

Scene* createSplashState(View* view)
{
  return new SplashState(view);
}

