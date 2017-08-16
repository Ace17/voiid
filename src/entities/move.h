#pragma once

#include "entity.h"

struct Trace
{
  bool tx;
  bool ty;
  bool tz;
};

inline
Trace slideMove(Entity* ent, Vector vel)
{
  Trace r;

  r.tx = ent->physics->moveBody(ent, Vector(vel.x, 0, 0));
  r.ty = ent->physics->moveBody(ent, Vector(0, vel.y, 0));
  r.tz = ent->physics->moveBody(ent, Vector(0, 0, vel.z));

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

