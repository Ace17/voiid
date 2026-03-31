#include "gameplay/convex.h"
#include "tests.h"

unittest("Convex: raycastBoxVsTriangle, facing")
{
  Triangle t{};

  t.vertices[0] = { -100, -100, 0 };
  t.vertices[1] = { +100, -100, 0 };
  t.vertices[2] = { 0, +100, 0 };
  t.normal = { 0, 0, 1 };
  t.edgeDirs[0] = normalize(t.vertices[1] - t.vertices[0]);
  t.edgeDirs[1] = normalize(t.vertices[2] - t.vertices[1]);
  t.edgeDirs[2] = normalize(t.vertices[0] - t.vertices[2]);

  Trace r = raycastBoxVsTriangle({ 0, 0, 10 }, { 0, 0, -10 }, {}, t);
  assertEquals(0.5f, r.fraction);

  r = raycastBoxVsTriangle({ 0, 0, 10 }, { 0, 0, 0 }, {}, t);
  assertEquals(1.0f, r.fraction);

  r = raycastBoxVsTriangle({ 0, 0, 10 }, { 0, 0, 0 }, { 2, 2, 2 }, t);
  assertEquals(0.8f, r.fraction);
}

unittest("Convex: raycastBoxVsTriangle, from side")
{
  Triangle t{};

  t.vertices[0] = { -100, 0, 0 };
  t.vertices[1] = { +100, 0, 0 };
  t.vertices[2] = { 0, +100, 0 };
  t.normal = { 0, 0, 1 };
  t.edgeDirs[0] = normalize(t.vertices[1] - t.vertices[0]);
  t.edgeDirs[1] = normalize(t.vertices[2] - t.vertices[1]);
  t.edgeDirs[2] = normalize(t.vertices[0] - t.vertices[2]);

  Trace r = raycastBoxVsTriangle({ 0, -10, 0 }, { 0, +10, 0 }, { 1, 1, 1 }, t);
  assertEquals(0.45f, r.fraction);
}

unittest("Convex: raycastBoxVsTriangle, from inside to outside")
{
  Triangle t{};

  t.vertices[0] = { -100, 0, 0 };
  t.vertices[1] = { +100, 0, 0 };
  t.vertices[2] = { 0, +100, 0 };
  t.normal = { 0, 0, 1 };

  Trace r = raycastBoxVsTriangle({ 0, 0, 0 }, { 0, 0, 10 }, { 1, 1, 1 }, t);
  assertEquals(1, r.fraction);
}

