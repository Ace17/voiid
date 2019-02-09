// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Pluggable entity factory, registration side: game-specific part

#include "entity_factory.h"

#include "switch.h"
#include "spider.h"
#include "bonus.h"
#include "player.h"
#include "spikes.h"
#include "blocks.h"
#include "moving_platform.h"
#include "conveyor.h"
#include "sign.h"
#include "finish.h"

map<string, CreationFunc> getRegistry()
{
  map<string, CreationFunc> r;

  r["upgrade_climb"] = [] (EntityConfig &) { return makeBonus(4, UPGRADE_CLIMB, "jump while against wall"); };
  r["upgrade_shoot"] = [] (EntityConfig &) { return makeBonus(3, UPGRADE_SHOOT, "press Z"); };
  r["upgrade_dash"] = [] (EntityConfig &) { return makeBonus(5, UPGRADE_DASH, "press C while walking"); };
  r["upgrade_djump"] = [] (EntityConfig &) { return makeBonus(6, UPGRADE_DJUMP, "jump while airborne"); };
  r["upgrade_ball"] = [] (EntityConfig &) { return makeBonus(7, UPGRADE_BALL, "press down"); };
  r["upgrade_slide"] = [] (EntityConfig &) { return makeBonus(8, UPGRADE_SLIDE, "go against wall while falling"); };
  r["bonus"] = [] (EntityConfig &) { return makeBonus(0, 0, "life up"); };
  r["spider"] = [] (EntityConfig &) { return make_unique<Spider>(); };
  r["spikes"] = [] (EntityConfig &) { return make_unique<Spikes>(); };
  r["fragile_door"] = [] (EntityConfig &) { return makeBreakableDoor(); };
  r["fragile_block"] = [] (EntityConfig &) { return make_unique<FragileBlock>(); };
  r["crumble_block"] = [] (EntityConfig &) { return make_unique<CrumbleBlock>(); };
  r["door"] = [] (EntityConfig& args) { auto arg = atoi(args[0].c_str()); return makeDoor(arg); };
  r["auto_door"] = [] (EntityConfig &) { return makeAutoDoor(); };
  r["switch"] = [] (EntityConfig& args) { auto arg = atoi(args[0].c_str()); return makeSwitch(arg); };
  r["moving_platform"] = [] (EntityConfig& args) { auto arg = atoi(args[0].c_str()); return make_unique<MovingPlatform>(arg); };
  r["finish"] = [] (EntityConfig &) { return make_unique<FinishLine>(); };
  r["conveyor"] = [] (EntityConfig &) { return make_unique<Conveyor>(); };
  r["sign"] = [] (EntityConfig& args) { auto arg = atoi(args[0].c_str()); return make_unique<Sign>(arg); };

  // aliases for legacy levels
  r["mp"] = r["moving_platform"];

  return r;
}

