#pragma once

#include "base/scene.h"
#include <vector>
#include <memory>

struct StateMachine : Scene
{
  void next()
  {
    currIdx = (currIdx + 1) % states.size();
    tick(Control {});
  }

  void tick(Control const& c) override
  {
    auto current = states[currIdx].get();
    current->tick(c);
  }

  void draw() override
  {
    auto current = states[currIdx].get();
    current->draw();
  }

  vector<unique_ptr<Scene>> states;
  int currIdx = 0;
};

