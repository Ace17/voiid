#pragma once

#include <math.h>
#include "body.h"

inline
void slideMove(IPhysicsProbe* physics, Body* body, Vector delta)
{
  for(int i = 0; i < 5; ++i)
  {
    auto tr = physics->moveBody(body, delta);

    if(tr.fraction == 1.0)
      break;

    delta -= dotProduct(delta, tr.plane.N) * (tr.fraction - 1) * tr.plane.N;
    delta += tr.plane.N * 0.011;
  }
}

inline
bool isOnGround(IPhysicsProbe* physics, Body* body)
{
  return physics->traceBox(body->getBox(), Down * 0.1, body).fraction < 1.0;
}

inline
Vector vectorFromAngles(float alpha, float beta)
{
  auto const x = cos(alpha) * cos(beta);
  auto const y = sin(alpha) * cos(beta);
  auto const z = sin(beta);

  return Vector(x, y, z);
}

