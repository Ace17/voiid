// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Simplistic standalone JSON-parser

#pragma once

#include <map>
#include <string>
#include <vector>

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
  std::string stringValue;

  operator std::string() const
  {
    enforceType(Type::String);
    return stringValue;
  }

  ////////////////////////////////////////
  // type == Type::Object
  std::map<std::string, Value> members;

  Value const& operator [] (const char* name) const;

  bool has(const char* name) const
  {
    return members.find(name) != members.end();
  }

  ////////////////////////////////////////
  // type == Type::Array
  std::vector<Value> elements;

  Value const& operator [] (int i)
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
  int intPow10 = 0;

  operator int () const
  {
    enforceType(Type::Integer);
    return intValue;
  }

private:
  void enforceType(Type expected) const;
};

Value parse(const char* text, size_t len);
}

