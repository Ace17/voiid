// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "base/delegate.h"
#include "base/util.h"
#include "tests.h"
#include <cstring>
#include <vector>

template<>
struct ToStringImpl<std::pair<int, int>>
{
  static std::string call(const std::pair<int, int>& val)
  {
    return "(" + std::to_string(val.first) + ", " + std::to_string(val.second) + ")";
  }
};

template<>
struct ToStringImpl<std::vector<std::pair<int, int>>>
{
  static std::string call(const std::vector<std::pair<int, int>>& val)
  {
    std::string r;

    bool first = true;
    r += "[";

    for(auto& element : val)
    {
      if(!first)
        r += ", ";

      r += testValueToString(element);
      first = false;
    }

    r += "]";

    return r;
  }
};

unittest("Util: allPairs(1)")
{
  auto expected = std::vector<std::pair<int, int>>({});

  std::vector<std::pair<int, int>> result;

  for(auto p : allPairs(1))
    result.push_back(p);

  assertEquals(expected, result);
}

unittest("Util: allPairs simple")
{
  auto expected = std::vector<std::pair<int, int>>(
  {
    std::make_pair(0, 1),
    std::make_pair(0, 2),
    std::make_pair(1, 2),
  });

  std::vector<std::pair<int, int>> result;

  for(auto p : allPairs(3))
    result.push_back(p);

  assertEquals(expected, result);
}

unittest("Util: rasterScan simple")
{
  auto expected = std::vector<std::pair<int, int>>(
  {
    std::make_pair(0, 0),
    std::make_pair(1, 0),
    std::make_pair(0, 1),
    std::make_pair(1, 1),
    std::make_pair(0, 2),
    std::make_pair(1, 2),
  });

  std::vector<std::pair<int, int>> result;

  for(auto p : rasterScan(2, 3))
    result.push_back(p);

  assertEquals(expected, result);
}

unittest("Util: endsWith")
{
  assertTrue(endsWith("Hello World", "World"));
  assertTrue(!endsWith("Hello World", "You"));
}

unittest("Util: setExtension")
{
  assertEquals(std::string("test.obj"), setExtension("test.source", "obj"));
}

static bool operator == (const String& a, const String& b)
{
  if(a.len != b.len)
    return false;

  return memcmp(a.data, b.data, a.len) == 0;
}

template<>
struct ToStringImpl<String>
{
  static std::string call(const String& val)
  {
    return std::string(val.data, val.len);
  }
};

unittest("Util: dirName")
{
  assertEquals(std::string("Hello/World"), dirName("Hello/World/Goodbye.txt"));
}

unittest("Util: baseName")
{
  assertEquals(std::string("Goodbye.txt"), baseName("Hello/World/Goodbye.txt"));
}

unittest("Delegate: lambdas")
{
  Delegate<void(void)> callMe;

  bool wasCalled = false;
  callMe = [&] () { wasCalled = true; };
  callMe();
  assertTrue(wasCalled);
}

unittest("Delegate: return value")
{
  Delegate<int(int)> multiplyByThree;

  multiplyByThree = [] (int val) { return val * 3; };
  assertEquals(12, multiplyByThree(4));
}

