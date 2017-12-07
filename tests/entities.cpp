#include <algorithm>

#include "src/entities/bonus.h"
#include "src/entities/explosion.h"

#include "engine/tests/tests.h"

unittest("Entity: explosion")
{
  auto explosion = makeExplosion();

  assert(!explosion->dead);
  assertEquals(0, explosion->getActor().ratio);

  for(int i = 0; i < 10000; ++i)
    explosion->tick();

  assert(explosion->dead);
  assertEquals(100, int(explosion->getActor().ratio * 100));
}

#include "src/entities/player.h"

struct NullPlayer : Player
{
  virtual void think(Control const &)
  {
  }

  virtual float health()
  {
    return 0;
  }

  virtual void addUpgrade(int)
  {
  }

  virtual Actor getActor() const override
  {
    return Actor(pos, 0);
  }
};

struct NullGame : IGame
{
  virtual void playSound(SOUND)
  {
  }

  virtual void spawn(Entity*)
  {
  }

  virtual void postEvent(unique_ptr<Event> )
  {
  }

  virtual void subscribeForEvents(IEventSink*)
  {
  }

  virtual void unsubscribeForEvents(IEventSink*)
  {
  }

  virtual Vector3f getPlayerPosition()
  {
    return Vector3f(0, 0, 0);
  }

  virtual void textBox(char const*)
  {
  }
};

struct NullPhysicsProbe : IPhysicsProbe
{
  // called by entities
  bool moveBody(Body* body, Vector delta)
  {
    auto box = body->getBox();
    box.x += delta.x;
    box.y += delta.y;

    if(isSolid(body, box))
      return false;

    body->pos += delta;
    return true;
  }

  bool isSolid(const Body* /*body*/, Box box) const
  {
    return box.y < 0;
  }

  virtual Trace traceBox(Box box, Vector3f delta, const Body* except) const override
  {
    (void)box;
    (void)delta;
    (void)except;

    Trace r {};
    r.fraction = 1;
    return r;
  }

  Body* getBodiesInRect(Box, int, bool, const Body*) const
  {
    return nullptr;
  }
};

unittest("Entity: pickup bonus")
{
  struct MockPlayer : NullPlayer
  {
    virtual void addUpgrade(int upgrade)
    {
      upgrades |= upgrade;
    }

    int upgrades = 0;
  };

  NullGame game;
  NullPhysicsProbe physics;
  MockPlayer player;

  auto ent = makeBonus(0, 4, "cool");
  ent->game = &game;
  ent->physics = &physics;

  assert(!ent->dead);
  assertEquals(0, player.upgrades);

  ent->onCollide(&player);

  assert(ent->dead);
  assertEquals(4, player.upgrades);
}

bool nearlyEquals(float expected, float actual)
{
  return abs(expected - actual) < 0.01;
}

unittest("Entity: animate")
{
  auto ent = makeBonus(0, 4, "cool");

  float minVal = 10.0f;
  float maxVal = -10.0f;

  for(int i = 0; i < 1000; ++i)
  {
    auto actor = ent->getActor();
    minVal = min(minVal, actor.ratio);
    maxVal = max(maxVal, actor.ratio);
    ent->tick();
  }

  assert(nearlyEquals(0, minVal));
  assert(nearlyEquals(1, maxVal));
}

