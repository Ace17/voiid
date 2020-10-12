#pragma once

#include "base/geom.h"

typedef GenericVector3<float> Vector;
typedef GenericSize3<float> Size;
typedef GenericBox3<float> Box;

static auto const UnitSize = Size(1, 1, 1);
static auto const Up = GenericVector3<float>(0, 0, 1);
static auto const Down = GenericVector3<float>(0, 0, -1);

