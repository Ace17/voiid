#pragma once

#include <math.h>
#include "body.h"

struct Trace2
{
  bool onGround;
};

inline
Trace2 slideMove(IPhysicsProbe* physics, Body* body, Vector delta)
{
  Trace2 r;

  physics->moveBody(body, Vector(delta.x, 0, 0));
  physics->moveBody(body, Vector(0, delta.y, 0));
  r.onGround = !physics->moveBody(body, Vector(0, 0, delta.z));

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

