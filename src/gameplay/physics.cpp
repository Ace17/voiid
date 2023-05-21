// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Minimalistic physics engine.
// Only handles collision detection and moving.
// Doesn't know about acceleration or velocity.

#include "base/util.h"
#include "body.h"
#include "convex.h"
#include "physics.h"
#include <memory>
#include <vector>

using namespace std;

struct BoxShape : Shape
{
  Trace raycast(Vec3f A, Vec3f B, Vec3f boxHalfSize) const override
  {
    Convex b;
    b.planes.resize(6);

    // cast a ray against the minkowski sum of both boxes
    const auto cx = boxHalfSize.x;
    const auto cy = boxHalfSize.y;
    const auto cz = boxHalfSize.z;

    b.planes[0] = Plane { Vec3f(-1, 0, 0), 0 + cx };
    b.planes[1] = Plane { Vec3f(+1, 0, 0), 1 + cx };
    b.planes[2] = Plane { Vec3f(0, -1, 0), 0 + cy };
    b.planes[3] = Plane { Vec3f(0, +1, 0), 1 + cy };
    b.planes[4] = Plane { Vec3f(0, 0, -1), 0 + cz };
    b.planes[5] = Plane { Vec3f(0, 0, +1), 1 + cz };

    return b.trace(A, B);
  }
};

struct AffineTransformShape : Shape
{
  Vec3f pos;
  Vec3f size;

  const Shape* sub;

  Trace raycast(Vec3f A, Vec3f B, Vec3f boxHalfSize) const override
  {
    return sub->raycast(transform(A), transform(B), scale(boxHalfSize));
  }

  // (size.x;size.y;size.z) -> (1;1;1)
  Vec3f scale(Vec3f v) const
  {
    Vec3f r = v;
    r.x *= 1.0 / size.x;
    r.y *= 1.0 / size.y;
    r.z *= 1.0 / size.z;
    return r;
  }

  Vec3f transform(Vec3f v) const
  {
    return scale(v - pos);
  }
};

const Shape* getShapeBox()
{
  static const BoxShape boxShape;
  return &boxShape;
};

struct Physics : IPhysics
{
  void addBody(Body* body) override
  {
    m_bodies.push_back(body);
  }

  void removeBody(Body* body) override
  {
    auto isItTheOne =
      [ = ] (Body* candidate) { return candidate == body; };
    unstableRemove(m_bodies, isItTheOne);
  }

  Trace moveBody(Body* body, Vector delta) override
  {
    auto box = body->getBox();

    auto const trace = traceBox(box, delta, body);

    delta = delta * trace.fraction;

    box.pos += delta;

    if(trace.blocker)
      collideBodies(*body, *trace.blocker);

    body->pos += delta;

    if(body->pusher)
    {
      for(auto otherBody : m_bodies)
      {
        // skip ourselves
        if(otherBody == body)
          continue;

        if(otherBody->pusher)
          continue;

        // move stacked bodies
        // push potential non-solid bodies
        if(otherBody->ground == body || overlaps(box, otherBody->getBox()))
          moveBody(otherBody, delta);
      }
    }

    // update ground
    if(!body->pusher)
    {
      auto const trace = traceBox(box, Down * 0.1, body);

      if(trace.fraction < 1.0)
        body->ground = trace.blocker;
    }

    return trace;
  }

  Trace traceBox(Box box, Vector delta, const Body* except) const override
  {
    auto const halfSize = Vec3f(box.size.x, box.size.y, box.size.z) * 0.5;
    auto const boxCenter = box.pos + halfSize;

    auto const A = boxCenter;
    auto const B = boxCenter + delta;

    Trace r {};
    r.fraction = 1.0;

    for(auto other : m_bodies)
    {
      if(other == except)
        continue;

      if(!other->solid)
        continue;

      AffineTransformShape afs;
      afs.pos = other->pos;
      afs.size = other->size;
      afs.sub = other->shape;

      auto tr = afs.raycast(A, B, halfSize);

      if(tr.fraction < r.fraction)
      {
        r.fraction = tr.fraction;
        r.plane = tr.plane;
        r.blocker = other;
      }
    }

    return r;
  }

  void checkForOverlaps() override
  {
    for(auto p : allPairs((int)m_bodies.size()))
    {
      auto& me = *m_bodies[p.first];
      auto& other = *m_bodies[p.second];

      if(overlaps(me.getBox(), other.getBox()))
        collideBodies(me, other);
    }
  }

  void collideBodies(Body& me, Body& other)
  {
    if(other.collidesWith & me.collisionGroup)
      other.onCollision(&me);

    if(me.collidesWith & other.collisionGroup)
      me.onCollision(&other);
  }

  Body* getBodiesInBox(Box box, int collisionGroup, const Body* except) const override
  {
    for(auto& body : m_bodies)
    {
      if(body == except)
        continue;

      if(!(body->collisionGroup & collisionGroup))
        continue;

      if(overlaps(body->getBox(), box))
        return body;
    }

    return nullptr;
  }

private:
  vector<Body*> m_bodies;
};

unique_ptr<IPhysics> createPhysics()
{
  return make_unique<Physics>();
}

