#pragma once

#include <math.h>
#include "entity.h"

struct Trace
{
  bool onGround;
};

inline
Trace slideMove(Entity* ent, Vector delta)
{
  Trace r;

  ent->physics->moveBody(ent, Vector(delta.x, 0, 0));
  ent->physics->moveBody(ent, Vector(0, delta.y, 0));
  r.onGround = !ent->physics->moveBody(ent, Vector(0, 0, delta.z));

  return r;
}

inline
Vector vectorFromAngles(float alpha, float beta)
{
  auto const x = cos(alpha) * cos(beta);
  auto const y = sin(alpha) * cos(beta);
  auto const z = sin(beta);

  return Vector(x, y, z);
}

