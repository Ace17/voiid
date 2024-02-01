#pragma once

#include "game.h"
#include <functional>

struct HandleWithDeleter : Handle
{
  HandleWithDeleter(std::function<void(void)> deleter_) : deleter(deleter_) {}

  ~HandleWithDeleter() { deleter(); }

  std::function<void(void)> deleter;
};

