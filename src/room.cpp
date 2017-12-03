#include "room.h"

Brush::TRACE Brush::trace(Vector3f A, Vector3f B, float radius) const
{
  Brush::TRACE trace;
  trace.fraction = 1;

  Plane clipPlane;

  float enterBrush = -1;
  float leaveBrush = 1;

  for(auto& plane : planes)
  {
    auto const epsilon = 1.0f / 128.0f;
    auto const distA = plane.dist(A) + radius;
    auto const distB = plane.dist(B) - radius;

    // trace is completely outside the brush
    if(distA > 0 && distB > 0)
      return trace;

    // this plane is not crossed
    if(distA <= 0 && distB <= 0)
      continue;

    // trace is entering the brush
    if(distA > 0 && distB <= 0)
    {
      float fraction = (distA - epsilon) / (distA - distB);
      fraction = clamp(fraction, 0.0f, 1.0f);

      if(fraction > enterBrush)
      {
        enterBrush = fraction;
        clipPlane = plane;
      }
    }

    // trace is leaving the brush
    if(distA <= 0 && distB > 0)
    {
      float fraction = (distA + epsilon) / (distA - distB);
      fraction = clamp(fraction, 0.0f, 1.0f);
      leaveBrush = max(leaveBrush, fraction);

      if(fraction < leaveBrush)
        leaveBrush = fraction;
    }
  }

  if(enterBrush > -1 && enterBrush < leaveBrush)
  {
    trace.fraction = enterBrush;
    trace.plane = clipPlane;
  }

  return trace;
}

