/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Minimalistic physics engine.
// Only handles collision detection and moving.
// Doesn't know about acceleration or velocity.

#include "body.h"
#include "base/util.h"
#include "convex.h"
#include <vector>
#include <memory>

using namespace std;

template<typename T, typename Lambda>
void unstableRemove(vector<T>& container, Lambda predicate)
{
  for(int i = 0; i < (int)container.size(); ++i)
  {
    if(predicate(container[i]))
    {
      auto const j = (int)container.size() - 1;

      swap(container[i], container[j]);

      if(i != j)
        --i;

      container.pop_back();
    }
  }
}

struct Physics : IPhysics
{
  void addBody(Body* body)
  {
    m_bodies.push_back(body);
  }

  void removeBody(Body* body)
  {
    auto isItTheOne =
      [ = ] (Body* candidate) { return candidate == body; };
    unstableRemove(m_bodies, isItTheOne);
  }

  void clearBodies()
  {
    m_bodies.clear();
  }

  Trace moveBody(Body* body, Vector delta) override
  {
    auto rect = body->getBox();

    auto const trace = traceBox(rect, delta, body);

    delta = delta * trace.fraction;

    rect.x += delta.x;
    rect.y += delta.y;
    rect.z += delta.z;

    if(trace.blocker)
      collideBodies(*body, *trace.blocker);

    body->pos += delta;

    if(body->pusher)
    {
      // move stacked bodies
      for(auto otherBody : m_bodies)
      {
        if(otherBody->ground == body)
          moveBody(otherBody, delta);
      }

      // push potential non-solid bodies
      for(auto other : m_bodies)
        if(other != body && overlaps(rect, other->getBox()))
          moveBody(other, delta);
    }

    // update ground
    if(!body->pusher)
    {
      auto const trace = traceBox(rect, Down * 0.1, body);

      if(trace.fraction < 1.0)
        body->ground = trace.blocker;
    }

    return trace;
  }

  Trace traceBox(Box rect, Vector delta, const Body* except) const override
  {
    auto traceBodies = traceBoxThroughBodies(rect, delta, except);
    auto traceEdifice = traceBoxThroughEdifice(rect, delta);

    if(traceBodies.fraction < traceEdifice.fraction)
      return traceBodies;
    else
      return traceEdifice;
  }

  Trace traceBoxThroughEdifice(Box box, Vector delta) const
  {
    auto t = m_traceEdifice(box, delta);
    Trace r {};
    r.fraction = t.fraction;
    r.plane = t.plane;
    return r;
  }

  Trace traceBoxThroughBodies(Box box, Vector delta, const Body* except) const
  {
    auto const halfSize = Vector3f(box.cx, box.cy, box.cz) * 0.5;

    auto const A = Vector3f(box.x, box.y, box.z) + halfSize;
    auto const B = A + delta;

    Trace r {};
    r.fraction = 1.0;

    Convex b;
    b.planes.resize(6);

    for(auto other : m_bodies)
    {
      if(other == except)
        continue;

      if(!other->solid)
        continue;

      b.planes[0] = Plane { Vector3f(-1, 0, 0), -other->pos.x };
      b.planes[1] = Plane { Vector3f(+1, 0, 0), other->pos.x + other->size.cx };
      b.planes[2] = Plane { Vector3f(0, -1, 0), -other->pos.y };
      b.planes[3] = Plane { Vector3f(0, +1, 0), other->pos.y + other->size.cy };
      b.planes[4] = Plane { Vector3f(0, 0, -1), -other->pos.z };
      b.planes[5] = Plane { Vector3f(0, 0, +1), other->pos.z + other->size.cz };

      auto tr = b.trace(A, B, halfSize.x);

      if(tr.fraction < r.fraction)
      {
        r.fraction = tr.fraction;
        r.plane = tr.plane;
        r.blocker = other;
      }
    }

    return r;
  }

  void checkForOverlaps()
  {
    for(auto p : allPairs((int)m_bodies.size()))
    {
      auto& me = *m_bodies[p.first];
      auto& other = *m_bodies[p.second];

      auto rect = me.getBox();
      auto otherBox = other.getBox();

      if(overlaps(rect, otherBox))
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

  void setEdifice(function<::Trace(Box, Vector)> trace) override
  {
    m_traceEdifice = trace;
  }

  Body* getBodiesInBox(Box myBox, int collisionGroup, bool onlySolid, const Body* except) const
  {
    for(auto& body : m_bodies)
    {
      if(onlySolid && !body->solid)
        continue;

      if(body == except)
        continue;

      if(!(body->collisionGroup & collisionGroup))
        continue;

      auto rect = body->getBox();

      if(overlaps(rect, myBox))
        return body;
    }

    return nullptr;
  }

private:
  vector<Body*> m_bodies;
  function<::Trace(Box, Vector)> m_traceEdifice;
};

unique_ptr<IPhysics> createPhysics()
{
  return make_unique<Physics>();
}

