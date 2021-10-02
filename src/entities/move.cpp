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
  const auto box = body->getBox();

  auto trace = physics->traceBox(box, Down * 0.1, body);

  if(trace.fraction == 1.0)
    return false; // nothing was hit

  // we hit something, and it's reasonably horizontal
  if(trace.fraction < 1.0 && trace.plane.N.z > 0.71)
    return true;

  // make a second attempt to find horizontal ground: search down with a thinner box
  Box smallerBox;
  smallerBox.pos.x = box.pos.x + box.size.cx * 0.25;
  smallerBox.pos.y = box.pos.y + box.size.cy * 0.25;
  smallerBox.pos.z = box.pos.z;
  smallerBox.size.cx = box.size.cx * 0.5;
  smallerBox.size.cy = box.size.cy * 0.5;
  smallerBox.size.cz = box.size.cz;

  trace = physics->traceBox(smallerBox, Down * 0.1, body);

  // we hit something, and it's reasonably horizontal
  if(trace.fraction < 1.0 && trace.plane.N.z > 0.71)
    return true;

  return false;
}

Vector vectorFromAngles(float alpha, float beta)
{
  auto const x = cos(alpha) * cos(beta);
  auto const y = sin(alpha) * cos(beta);
  auto const z = sin(beta);

  return Vector(x, y, z);
}

