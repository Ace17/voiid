// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <math.h>
#include "body.h"
#include "physics_probe.h"

void slideMove(IPhysicsProbe* physics, Body* body, Vector delta);
bool isOnGround(IPhysicsProbe* physics, Body* body);
Vector vectorFromAngles(float alpha, float beta);

