#include "engine/tests/tests.h"
#include "src/convex.h"

static
void assertNearlyEqualsFunc(float expected, float actual, const char* file, int line)
{
  auto delta = expected - actual;

  if(fabs(delta) > 0.01)
  {
    using namespace std;
    stringstream ss;
    ss << "Assertion failure" << endl;
    ss << file << "(" << line << ")" << endl;
    ss << "Expected '" << expected << "', got '" << actual << "'" << endl;
    throw logic_error(ss.str());
  }
}

#define assertNearlyEquals(u, v) \
  assertNearlyEqualsFunc(u, v, __FILE__, __LINE__)

auto const ZeroSize = Vector3f(0, 0, 0);
auto const HalfSize = Vector3f(0.5, 0.5, 0.5);

unittest("Convex: trace down through the floor in one big step")
{
  Convex floor;
  floor.planes.push_back(Plane { Vector3f(0, 0, 1), 0 });

  auto trace = floor.trace(Vector3f(0, 0, 10), Vector3f(0, 0, -10), ZeroSize);

  assertNearlyEquals(0.5f, trace.fraction);
}

unittest("Convex: trace down through the floor using small steps")
{
  Convex floor;
  auto Up = Vector3f(0, 0, 1);
  floor.planes.push_back(Plane { Up, 0 });

  float z = 1.0;
  auto dz = -0.1;

  while(fabs(dz) > 0.0001 && z > 0.0f)
  {
    auto pos = Vector3f(0, 0, z);
    auto delta = Vector3f(0, 0, dz);
    auto trace = floor.trace(pos, delta, ZeroSize);
    z += trace.fraction * dz;
    dz *= 0.999;
  }

  assertEquals(true, z > 0.0f);
}

unittest("Convex: trace AABB down through the floor using small steps")
{
  Convex floor;
  auto Up = Vector3f(0, 0, 1);
  floor.planes.push_back(Plane { Up, 0 });

  float z = 1.0;
  auto dz = -0.1;

  while(fabs(dz) > 0.0001)
  {
    auto pos = Vector3f(0, 0, z);
    auto delta = Vector3f(0, 0, dz);
    auto trace = floor.trace(pos, delta, HalfSize);
    z += trace.fraction * dz;
    dz *= 0.9999;
  }

  assertEquals(true, z > 0.4999f);
}

unittest("Convex: trace point down through sloped floor using small steps")
{
  Convex floor;
  auto Up = normalize(Vector3f(0, 0.1, 0.9));
  floor.planes.push_back(Plane { Up, 0 });

  auto delta = Up * -1.0;
  auto pos = Vector3f(0, 0, 1);

  while(fabs(delta.z) > 0.0001 && pos.z > 0.0f)
  {
    auto trace = floor.trace(pos, delta, ZeroSize);
    pos = pos + trace.fraction * delta;
    delta = delta * 0.999;
  }

  assertEquals(true, pos.z > 0.0f);
}

