#pragma once

#include "entity.h"
#include <memory>

// e.g:
// createEntity("spider");
// createEntity("door(4)");
std::unique_ptr<Entity> createEntity(string name);

