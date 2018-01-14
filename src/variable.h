#pragma once

#include "game.h"

struct HandleWithDeleter : Handle
{
  HandleWithDeleter(function<void(void)> deleter_) : deleter(deleter_) {}

  ~HandleWithDeleter() { deleter(); }

  function<void(void)> deleter;
};

