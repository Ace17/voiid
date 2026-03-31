// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Sphere/Convex continuous collision detection.

#include "base/util.h" // clamp
#include "convex.h"
#include <cfloat>

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
    auto const radius = std::abs(boxSize.x * plane.N.x) + std::abs(boxSize.y * plane.N.y) + std::abs(boxSize.z * plane.N.z);
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
      fraction = ::clamp(fraction, 0.0f, 1.0f);

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
      fraction = ::clamp(fraction, 0.0f, 1.0f);
      leaveBrush = std::max(leaveBrush, fraction);

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

Trace raycastBoxVsTriangle(Vec3f A, Vec3f B, Vec3f boxHalfSize, const Triangle& t)
{
  Vec3f axes[16];
  int axeCount = 0;

  assert(t.normal == t.normal);

  axes[axeCount++] = t.normal;
  axes[axeCount++] = { 1, 0, 0 };
  axes[axeCount++] = { 0, 1, 0 };
  axes[axeCount++] = { 0, 0, 1 };

  auto addAxis = [&](Vec3f a, Vec3f b)
    {
      Vec3f x = crossProduct(a, b);

      if(dotProduct(x, x) > 0.0001)
        axes[axeCount++] = normalize(x);
    };

  for(auto& edge : t.edgeDirs)
    addAxis({ 1, 0, 0 }, edge);

  for(auto& edge : t.edgeDirs)
    addAxis({ 0, 1, 0 }, edge);

  for(auto& edge : t.edgeDirs)
    addAxis({ 0, 0, 1 }, edge);

  Trace r;
  r.fraction = 0;

  float leaveFraction = 1;

  Vec3f delta = B - A;

  for(auto axis : Span<Vec3f>(axes).sub(axeCount))
  {
    assert(axis == axis);

    // make the move always increase the position along the axis
    if(dotProduct(axis, delta) < 0)
      axis = axis * -1;

    // compute projections on the axis
    const float startPos = dotProduct(A, axis);
    const float targetPos = dotProduct(B, axis);
    const float epsilon = 0;// 1.0f / 128.0f;

    assert(startPos <= targetPos || std::abs(startPos - targetPos) < 0.0001);

    float projectedObstacleMin = +FLT_MAX;
    float projectedObstacleMax = -FLT_MAX;
    float boxProjection = std::abs(axis.x) * boxHalfSize.x + std::abs(axis.y) * boxHalfSize.y + std::abs(axis.z) * boxHalfSize.z;

    for(auto p : t.vertices)
    {
      float minP = dotProduct(p, axis) - boxProjection;
      float maxP = dotProduct(p, axis) + boxProjection;

      projectedObstacleMin = std::min(projectedObstacleMin, minP);
      projectedObstacleMax = std::max(projectedObstacleMax, maxP);
    }

    if(targetPos < projectedObstacleMin)
    {
      r.fraction = 1;
      return r; // all the axis-projected move is before the obstacle
    }

    if(startPos >= projectedObstacleMax)
    {
      r.fraction = 1;
      return r; // all the axis-projected move is after the obstacle
    }

    if(startPos > projectedObstacleMin && startPos < projectedObstacleMax)
      if(targetPos > projectedObstacleMax)
      {
        r.fraction = 1;
        return r; // all the axis-projected move is after the obstacle
      }

    if(std::abs(startPos - targetPos) > 0.00001)
    {
      float f = (projectedObstacleMin - startPos - epsilon) / (targetPos - startPos);

      if(f > r.fraction)
      {
        r.fraction = f;
        r.plane.N = dotProduct(axis, delta) < 0 ? axis : axis * -1;
        r.plane.D = projectedObstacleMin + boxProjection;
      }

      float fMax = (projectedObstacleMax + epsilon - startPos) / (targetPos - startPos);

      if(startPos <= projectedObstacleMax && targetPos > projectedObstacleMax)
      {
        // trace is leaving the shape on this axis
        if(fMax < leaveFraction)
          leaveFraction = fMax;
      }
    }
  }

  // we're leaving before we're entering, which means the trajectory can be
  // separated from the obstacle, by a plane parallel to the trajectory
  if(leaveFraction < r.fraction)
  {
    r.fraction = 1;
    return r;
  }

  return r;
}

