// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// pluggable factory for entities, instanciation side.

#pragma once

#include "entity.h"
#include <memory>
#include <map>

// e.g:
// createEntity("spider");
// createEntity("door(4)");
std::unique_ptr<Entity> createEntity(string name);

typedef vector<string> const EntityConfig;
typedef function<unique_ptr<Entity>(EntityConfig & args)> CreationFunc;

// user-provided
extern map<string, CreationFunc> getRegistry();

