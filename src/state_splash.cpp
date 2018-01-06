/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// splash menu

#include "base/scene.h"

#include "toggle.h"
#include "models.h" // MDL_SPLASH
#include "sounds.h" // SND_PAUSE
#include "state_machine.h"

struct SplashState : Scene
{
  SplashState(View* view_, StateMachine* fsm_) : view(view_), fsm(fsm_)
  {
    ambientLight = 10;
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  void tick(Control const& c) override
  {
    auto const FADE_TIME = 2000;

    view->playMusic(1);

    if(!activated)
    {
      view->textBox("V O I I D");

      if(c.fire || c.jump || c.dash)
      {
        activated = true;
        delay = FADE_TIME;
        view->playSound(SND_PAUSE);
      }
    }

    if(activated)
    {
      ambientLight = delay / float(FADE_TIME);

      if(decrement(delay))
      {
        activated = false;
        fsm->next();
      }
    }
  }

  vector<Actor> getActors() const override
  {
    return vector<Actor>();
  }

private:
  View* const view;
  StateMachine* const fsm;
  bool activated = false;
  int delay = 0;
};

unique_ptr<Scene> createSplashState(StateMachine* fsm, View* view)
{
  return make_unique<SplashState>(view, fsm);
}
