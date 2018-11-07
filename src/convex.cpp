// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Sphere/Convex continuous collision detection.

#include "base/util.h" // clamp
#include "convex.h"

// Sweeps a box from A to B.
// Returns the fraction of the move to the first intersection with the convex,
// or 1.0 if there are no intersections.
// Also returns the intersecting plane, if any.
Trace Convex::trace(Vector A, Vector B, Vector boxSize) const
{
  Trace trace;
  trace.fraction = 1;

  Plane clipPlane;

  float enterBrush = -1;
  float leaveBrush = 1;

  for(auto& plane : planes)
  {
    auto const radius = abs(boxSize.x * plane.N.x) + abs(boxSize.y * plane.N.y) + abs(boxSize.z * plane.N.z);
    auto const epsilon = 1.0f / 128.0f;
    auto const distA = plane.dist(A) - radius;
    auto const distB = plane.dist(B) - radius;

    // trace is completely outside the convex
    if(distA > 0 && distB > 0)
      return trace;

    // this plane is not crossed
    if(distA <= 0 && distB <= 0)
      continue;

    // trace is entering the convex
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

    // trace is leaving the convex
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

