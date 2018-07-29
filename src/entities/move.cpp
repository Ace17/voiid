#include "move.h"

void slideMove(IPhysicsProbe* physics, Body* body, Vector delta)
{
  for(int i = 0; i < 5; ++i)
  {
    auto tr = physics->moveBody(body, delta);

    if(tr.fraction == 1.0)
      break;

    // remove from 'delta' the fraction of the move that succeeded
    auto const actual = tr.fraction * delta;
    delta -= actual;

    // remove from 'delta' its component along the collision normal
    delta -= dotProduct(delta, tr.plane.N) * tr.plane.N;
  }
}

bool isOnGround(IPhysicsProbe* physics, Body* body)
{
  return physics->traceBox(body->getBox(), Down * 0.1, body).fraction < 1.0;
}

Vector vectorFromAngles(float alpha, float beta)
{
  auto const x = cos(alpha) * cos(beta);
  auto const y = sin(alpha) * cos(beta);
  auto const z = sin(beta);

  return Vector(x, y, z);
}

