#pragma once

#include "game.h"
#include <functional>

struct HandleWithDeleter : Handle
{
  HandleWithDeleter(function<void(void)> deleter_) : deleter(deleter_) {}

  ~HandleWithDeleter() { deleter(); }

  function<void(void)> deleter;
};

