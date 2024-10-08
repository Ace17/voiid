// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>

struct Entity;

struct IEntityConfig
{
  virtual std::string getString(const char* varName, std::string defaultValue = "") = 0;
  virtual int getInt(const char* varName, int defaultValue = 0) = 0;
};

// e.g:
// createEntity("spider");
// createEntity("door(4)");
std::unique_ptr<Entity> createEntity(std::string name, IEntityConfig* config);

using CreationFunc = std::unique_ptr<Entity>(*)(IEntityConfig* args);
int registerEntity(std::string type, CreationFunc func);

