#include <algorithm>

#include "entities/bonus.h"
#include "entities/explosion.h"

#include "tests.h"

unittest("Entity: explosion")
{
  auto explosion = makeExplosion();

  assert(!explosion->dead);

  for(int i = 0; i < 10000; ++i)
    explosion->tick();

  assert(explosion->dead);
}

#include "gameplay/player.h"

struct NullPlayer : Player
{
  virtual void think(Control const&) override
  {
  }

  virtual float health() override
  {
    return 0;
  }

  virtual void addUpgrade(int) override
  {
  }

  virtual void onDraw(View*) const override
  {
  }
};

struct NullGame : IGame
{
  virtual void playSound(int) {}
  virtual void spawn(Entity*) {}
  virtual void postEvent(std::unique_ptr<Event>) {}
  virtual std::unique_ptr<Handle> subscribeForEvents(IEventSink*) { return nullptr; }
  virtual void unsubscribeForEvents(IEventSink*) {}
  virtual void textBox(String) {}
};

struct NullPhysicsProbe : IPhysicsProbe
{
  // called by entities
  Trace moveBody(Body* body, Vector delta) override
  {
    Trace tr {};
    auto box = body->getBox();
    box.pos += delta;

    if(isSolid(body, box))
    {
      tr.fraction = 0;
      return tr;
    }

    body->pos += delta;
    tr.fraction = 1;
    return tr;
  }

  bool isSolid(const Body* /*body*/, Box box) const
  {
    return box.pos.y < 0;
  }

  virtual Trace traceBox(Box box, Vec3f delta, const Body* except) const override
  {
    (void)box;
    (void)delta;
    (void)except;

    Trace r {};
    r.fraction = 1;
    return r;
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

