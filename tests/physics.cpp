#include "engine/tests/tests.h"
#include "src/body.h"
#include <cmath>
#include <memory>

///////////////////////////////////////////////////////////////////////////////

#define assertNearlyEquals(u, v) \
  assertNearlyEqualsFunc(u, v, __FILE__, __LINE__)

static inline
std::ostream & operator << (std::ostream& o, const Vector& v)
{
  o << "(";
  o << v.x;
  o << ", ";
  o << v.y;
  o << ")";
  return o;
}

void assertNearlyEqualsFunc(Vector expected, Vector actual, const char* file, int line)
{
  auto delta = expected - actual;

  if(fabs(delta.x) > 0.01 || fabs(delta.y) > 0.01)
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

unique_ptr<IPhysics> createPhysics();

static
bool isSolid(Box rect)
{
  return rect.y < 0 || rect.x < 0;
}

struct Fixture
{
  Fixture() : physics(createPhysics())
  {
    physics->setEdifice(&isSolid);
    physics->addBody(&mover);
  }

  unique_ptr<IPhysics> physics;
  Body mover;
};

unittest("Physics: simple move")
{
  Fixture fix;
  fix.mover.pos = Vector(10, 10, 10);

  auto allowed = fix.physics->moveBody(&fix.mover, Vector(10, 0, 0));
  assert(allowed);
  assertNearlyEquals(Vector(20, 10, 10), fix.mover.pos);
}

unittest("Physics: left move, blocked by vertical wall at x=0")
{
  Fixture fix;
  fix.mover.pos = Vector(10, 10, 0);

  auto allowed = fix.physics->moveBody(&fix.mover, Vector(-20, 0, 0));
  assert(!allowed);

  assertNearlyEquals(Vector(10, 10, 0), fix.mover.pos);
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
  assert(!allowed);

  assertNearlyEquals(Vector(100, 10, 0), fix.mover.pos);
}
