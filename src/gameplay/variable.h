#pragma once

#include "base/delegate.h"
#include "game.h"

struct HandleWithDeleter : Handle
{
  HandleWithDeleter(Delegate<void(void)>&& deleter_) : deleter(std::move(deleter_)) {}

  ~HandleWithDeleter() { deleter(); }

  Delegate<void(void)> deleter;
};

