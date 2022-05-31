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
  Trace raycast(Vector3f A, Vector3f B, Vector3f boxHalfSize) const override
  {
    Convex b;
    b.planes.resize(6);

    b.planes[0] = Plane { Vector3f(-1, 0, 0), 0 };
    b.planes[1] = Plane { Vector3f(+1, 0, 0), 1 };
    b.planes[2] = Plane { Vector3f(0, -1, 0), 0 };
    b.planes[3] = Plane { Vector3f(0, +1, 0), 1 };
    b.planes[4] = Plane { Vector3f(0, 0, -1), 0 };
    b.planes[5] = Plane { Vector3f(0, 0, +1), 1 };

    return b.trace(A, B, boxHalfSize);
  }
};

struct AffineTransformShape : Shape
{
  Vector3f pos;
  Vector3f size;

  const Shape* sub;

  Trace raycast(Vector3f A, Vector3f B, Vector3f boxHalfSize) const override
  {
    return sub->raycast(transform(A), transform(B), scale(boxHalfSize));
  }

  // (size.x;size.y;size.z) -> (1;1;1)
  Vector3f scale(Vector3f v) const
  {
    Vector3f r = v;
    r.x *= 1.0 / size.x;
    r.y *= 1.0 / size.y;
    r.z *= 1.0 / size.z;
    return r;
  }

  Vector3f transform(Vector3f v) const
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
    auto const halfSize = Vector3f(box.size.cx, box.size.cy, box.size.cz) * 0.5;
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

