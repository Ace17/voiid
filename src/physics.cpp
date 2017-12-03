#include "body.h"
#include "base/util.h"
#include "brush.h"
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

  bool moveBody(Body* body, Vector delta)
  {
    auto rect = body->getRect();

    auto const trace = traceBox(rect, delta, body);
    auto const blocked = trace.fraction < 1.0f;

    rect.x += delta.x * trace.fraction;
    rect.y += delta.y * trace.fraction;
    rect.z += delta.z * trace.fraction;

    if(blocked)
    {
      if(trace.blocker)
        collideBodies(*body, *trace.blocker);
    }
    else
    {
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
          if(other != body && overlaps(rect, other->getRect()))
            moveBody(other, delta);
      }
    }

    // update ground
    if(!body->pusher)
    {
      auto const trace = traceBox(rect, Vector3f(0, 0, -0.01), body);

      if(trace.fraction < 1.0)
        body->ground = trace.blocker;
    }

    return !blocked;
  }

  TRACE traceBox(Box rect, Vector3f delta, const Body* except) const
  {
    auto traceBodies = traceBoxThroughBodies(rect, delta, except);
    auto traceEdifice = m_traceEdifice(rect, delta);

    if(traceBodies.fraction < traceEdifice.fraction)
      return traceBodies;
    else
      return traceEdifice;
  }

  TRACE traceBoxThroughBodies(Box box, Vector3f delta, const Body* except) const
  {
    auto const halfSize = Vector3f(box.cx, box.cy, box.cz) * 0.5;

    auto const A = Vector3f(box.x, box.y, box.z) + halfSize;
    auto const B = A + delta;

    TRACE r {};
    r.fraction = 1.0;

    Brush b;
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
        r.N = tr.plane.N;
      }
    }

    return r;
  }

  bool isSolid(const Body* except, Box rect) const
  {
    if(getSolidBodyInRect(rect, except))
      return true;

    if(m_traceEdifice(rect, Vector3f(0, 0, 0)).fraction < 1.0)
      return true;

    return false;
  }

  void checkForOverlaps()
  {
    for(auto p : allPairs((int)m_bodies.size()))
    {
      auto& me = *m_bodies[p.first];
      auto& other = *m_bodies[p.second];

      auto rect = me.getRect();
      auto otherRect = other.getRect();

      if(overlaps(rect, otherRect))
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

  void setEdifice(function<TRACE(Box, Vector3f)> trace)
  {
    m_traceEdifice = trace;
  }

  Body* getBodiesInRect(Box myRect, int collisionGroup, bool onlySolid, const Body* except) const
  {
    for(auto& body : m_bodies)
    {
      if(onlySolid && !body->solid)
        continue;

      if(body == except)
        continue;

      if(!(body->collisionGroup & collisionGroup))
        continue;

      auto rect = body->getRect();

      if(overlaps(rect, myRect))
        return body;
    }

    return nullptr;
  }

private:
  Body* getSolidBodyInRect(Box myRect, const Body* except) const
  {
    return getBodiesInRect(myRect, -1, true, except);
  }

  vector<Body*> m_bodies;
  function<TRACE(Box, Vector3f)> m_traceEdifice;
};

unique_ptr<IPhysics> createPhysics()
{
  return make_unique<Physics>();
}

