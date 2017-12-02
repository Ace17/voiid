#include "entity_factory.h"
#include <map>
#include <functional>

using namespace std;

typedef vector<string> const EntityArgs;

typedef function<unique_ptr<Entity>(EntityArgs & args)> CreationFunc;
static map<string, CreationFunc> getRegistry();
static const map<string, CreationFunc> registry = getRegistry();

unique_ptr<Entity> createEntity(string formula)
{
  EntityArgs args;
  auto name = formula;

  auto i_func = registry.find(name);

  if(i_func == registry.end())
    throw runtime_error("unknown entity type: '" + name + "'");

  return (*i_func).second(args);
}

#include "entities/switch.h"
#include "entities/wheel.h"
#include "entities/spider.h"
#include "entities/hopper.h"
#include "entities/sweeper.h"
#include "entities/detector.h"
#include "entities/bonus.h"
#include "entities/player.h"
#include "entities/spikes.h"
#include "entities/blocks.h"
#include "entities/moving_platform.h"
#include "entities/conveyor.h"
#include "entities/sign.h"

static map<string, CreationFunc> getRegistry()
{
  map<string, CreationFunc> r;

  r["upgrade_climb"] =
    [] (EntityArgs &)
    {
      return makeBonus(4, UPGRADE_CLIMB, "jump while against wall");
    };

  r["upgrade_shoot"] =
    [] (EntityArgs &)
    {
      return makeBonus(3, UPGRADE_SHOOT, "press Z");
    };

  r["upgrade_dash"] =
    [] (EntityArgs &)
    {
      return makeBonus(5, UPGRADE_DASH, "press C while walking");
    };

  r["upgrade_djump"] =
    [] (EntityArgs &)
    {
      return makeBonus(6, UPGRADE_DJUMP, "jump while airborne");
    };

  r["upgrade_ball"] =
    [] (EntityArgs &)
    {
      return makeBonus(7, UPGRADE_BALL, "press down");
    };

  r["upgrade_slide"] =
    [] (EntityArgs &)
    {
      return makeBonus(8, UPGRADE_SLIDE, "go against wall while falling");
    };

  r["bonus"] =
    [] (EntityArgs &)
    {
      return makeBonus(0, 0, "life up");
    };

  r["wheel"] =
    [] (EntityArgs &)
    {
      return make_unique<Wheel>();
    };

  r["hopper"] =
    [] (EntityArgs &)
    {
      return make_unique<Hopper>();
    };

  r["sweeper"] =
    [] (EntityArgs &)
    {
      return make_unique<Sweeper>();
    };

  r["spider"] =
    [] (EntityArgs &)
    {
      return make_unique<Spider>();
    };

  r["spikes"] =
    [] (EntityArgs &)
    {
      return make_unique<Spikes>();
    };

  r["fragile_door"] =
    [] (EntityArgs &)
    {
      return makeBreakableDoor();
    };

  r["fragile_block"] =
    [] (EntityArgs &)
    {
      return make_unique<FragileBlock>();
    };

  r["crumble_block"] =
    [] (EntityArgs &)
    {
      return make_unique<CrumbleBlock>();
    };

  r["door(0)"] =
    [] (EntityArgs &)
    {
      return makeDoor(0);
    };

  r["switch(0)"] =
    [] (EntityArgs &)
    {
      return makeSwitch(0);
    };

  r["moving_platform(0)"] =
    [] (EntityArgs &)
    {
      return make_unique<MovingPlatform>(0);
    };

  r["moving_platform(1)"] =
    [] (EntityArgs &)
    {
      return make_unique<MovingPlatform>(1);
    };

  r["conveyor(0)"] =
    [] (EntityArgs &)
    {
      return make_unique<Conveyor>();
    };

  r["sign(0)"] =
    [] (EntityArgs &)
    {
      return make_unique<Sign>(0);
    };

  r["sign(1)"] =
    [] (EntityArgs &)
    {
      return make_unique<Sign>(1);
    };

  return r;
}

