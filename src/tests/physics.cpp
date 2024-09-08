#include "gameplay/body.h"
#include "gameplay/physics.h"
#include "tests.h"
#include <cmath>
#include <memory>

///////////////////////////////////////////////////////////////////////////////

#define assertNearlyEquals(u, v) \
        assertNearlyEqualsFunc(u, v, __FILE__, __LINE__)

static inline
std::ostream& operator << (std::ostream& o, const Vector& v)
{
  o << "(";
  o << v.x;
  o << ", ";
  o << v.y;
  o << ", ";
  o << v.z;
  o << ")";
  return o;
}

void assertNearlyEqualsFunc(Vector expected, Vector actual, const char* file, int line)
{
  auto delta = expected - actual;

  if(fabs(delta.x) > 0.1 || fabs(delta.y) > 0.1 || fabs(delta.z) > 0.1)
  {
    using namespace std;
    stringstream ss;
    ss << "Assertion failure" << endl;
    ss << file << "(" << line << ")" << endl;
    ss << "Expected '" << expected << "', got '" << actual << "'" << endl;
    throw logic_error(ss.str());
  }
}

///////////////////////////////////////////////////////////////////////////////

struct BlockerShape : Shape
{
  Trace raycast(Vec3f A, Vec3f B, Vec3f boxHalfSize) const override
  {
    Trace r {};
    r.fraction = 1.0;

    auto const delta = B - A;
    auto const x0 = boxHalfSize.x;
    auto const y0 = boxHalfSize.y;
    auto const targetX = B.x;
    auto const targetY = B.y;

    if(targetX < 0)
    {
      auto const fraction = fabs(A.x - x0) / fabs(delta.x);

      if(fraction < r.fraction)
      {
        r.fraction = fraction;
        r.plane.N = Vec3f(1, 0, 0);
      }
    }

    if(targetY < 0)
    {
      auto const fraction = fabs(A.y - y0) / fabs(delta.y);

      if(fraction < r.fraction)
      {
        r.fraction = fraction;
        r.plane.N = Vec3f(0, 1, 0);
      }
    }

    return r;
  }
};

const BlockerShape blockerShape;

struct Fixture
{
  Fixture() : physics(createPhysics())
  {
    physics->addBody(&mover);
    physics->addBody(&blocker);
    blocker.shape = &blockerShape;
    blocker.solid = 1;
  }

  Body blocker;
  std::unique_ptr<IPhysics> physics;
  Body mover;
};

unittest("Physics: simple move")
{
  Fixture fix;
  fix.mover.pos = Vector(10, 10, 10);

  auto allowed = fix.physics->moveBody(&fix.mover, Vector(10, 0, 0));
  assert(allowed.fraction == 1.0);
  assertNearlyEquals(Vector(20, 10, 10), fix.mover.pos);
}

unittest("Physics: left move, blocked by vertical wall at x=0")
{
  Fixture fix;
  fix.mover.pos = Vector(10, 10, 0);

  auto allowed = fix.physics->moveBody(&fix.mover, Vector(-20, 0, 0));
  assert(allowed.fraction < 1.0);

  assertNearlyEquals(Vector(0, 10, 0), fix.mover.pos);
}

unittest("Physics: left move, blocked by a bigger body")
{
  Fixture fix;
  fix.mover.pos = Vector(100, 10, 0);
  fix.mover.size = Size(1, 1, 1);

  Body blocker;
  blocker.pos = Vector(200, 5, 0);
  blocker.size = Size(10, 10, 10);
  blocker.solid = true;
  fix.physics->addBody(&blocker);

  auto allowed = fix.physics->moveBody(&fix.mover, Vector(100, 0, 0));
  assert(allowed.fraction < 1.0);

  assertNearlyEquals(Vector(199, 10, 0), fix.mover.pos);
}

unittest("Physics: a 1.9-unit wide solid can fit in a 2-unit wide gap")
{
  Fixture fix;

  Body leftBlocker;
  leftBlocker.pos = Vector(-22, 50, 0);
  leftBlocker.size = Size(22, 1, 1);
  leftBlocker.solid = true;
  fix.physics->addBody(&leftBlocker);

  Body rightBlocker;
  rightBlocker.pos = Vector(2, 50, 0);
  rightBlocker.size = Size(22, 1, 1);
  rightBlocker.solid = true;
  fix.physics->addBody(&rightBlocker);

  fix.mover.pos = Vector(0.01, 0, 0);
  fix.mover.size = Size(1.9, 1, 1);
  fix.physics->moveBody(&fix.mover, Vector(0, 100, 0));

  assertNearlyEquals(Vector(0.01, 100, 0), fix.mover.pos);
}

#include "entities/move.h"

unittest("Physics: slide up-left move, horizontally blocked by vertical wall at x=0")
{
  Fixture fix;
  fix.mover.pos = Vector(10, 10, 0);

  slideMove(fix.physics.get(), &fix.mover, Vector(-20, 20, 0));

  assertNearlyEquals(Vector(0, 30, 0), fix.mover.pos);
}

