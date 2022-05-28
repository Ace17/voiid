#pragma once

#include "base/delegate.h"

#include <memory>
#include <unordered_map>

template<typename Key, typename T>
struct WeakCache
{
  std::shared_ptr<T> fetch(Key key)
  {
    std::shared_ptr<T> r;

    if(entries.find(key) == entries.end())
    {
      r = onCacheMiss(key);
      entries[key] = r;
    }

    return entries[key].lock();
  }

  Delegate<std::unique_ptr<T>(Key)> onCacheMiss;
  std::unordered_map<Key, std::weak_ptr<T>> entries;
};

