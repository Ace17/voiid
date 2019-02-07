// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Simplistic standalone JSON-parser

#pragma once

#include <map>
#include <string>
#include <vector>
#include <stdexcept>
using namespace std;

namespace json
{
struct Value
{
  enum class Type
  {
    String,
    Object,
    Array,
    Integer,
    Boolean,
  };

  Type type;

  ////////////////////////////////////////
  // type == Type::String
  string stringValue;

  operator string () const
  {
    enforceType(Type::String);
    return stringValue;
  };

  ////////////////////////////////////////
  // type == Type::Object
  map<string, Value> members;

  Value const & operator [] (const char* name) const
  {
    enforceType(Type::Object);
    auto it = members.find(name);

    if(it == members.end())
      throw runtime_error("Member '" + string(name) + "' was not found");

    return it->second;
  }

  ////////////////////////////////////////
  // type == Type::Array
  vector<Value> elements;

  Value const & operator [] (int i)
  {
    enforceType(Type::Array);
    return elements[i];
  }

  ////////////////////////////////////////
  // type == Type::Boolean
  bool boolValue {};

  ////////////////////////////////////////
  // type == Type::Integer
  int intValue {};

  operator int () const
  {
    enforceType(Type::Integer);
    return intValue;
  };

private:
  void enforceType(Type expected) const
  {
    if(type != expected)
      throw runtime_error("Type error");
  }
};

Value parse(const char* text, size_t len);
}

