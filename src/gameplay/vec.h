// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Basic geometric types for 3D
#pragma once

#include "base/box.h"
#include "base/geom.h"

typedef GenericVector3<float> Vector;
typedef GenericSize3<float> Size;
typedef GenericBox3<float> Box;

static auto const UnitSize = Size(1, 1, 1);
static auto const Up = GenericVector3<float>(0, 0, 1);
static auto const Down = GenericVector3<float>(0, 0, -1);

