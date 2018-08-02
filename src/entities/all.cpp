// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
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

  r["upgrade_climb"] = [] (EntityArgs &) { return makeBonus(4, UPGRADE_CLIMB, "jump while against wall"); };
  r["upgrade_shoot"] = [] (EntityArgs &) { return makeBonus(3, UPGRADE_SHOOT, "press Z"); };
  r["upgrade_dash"] = [] (EntityArgs &) { return makeBonus(5, UPGRADE_DASH, "press C while walking"); };
  r["upgrade_djump"] = [] (EntityArgs &) { return makeBonus(6, UPGRADE_DJUMP, "jump while airborne"); };
  r["upgrade_ball"] = [] (EntityArgs &) { return makeBonus(7, UPGRADE_BALL, "press down"); };
  r["upgrade_slide"] = [] (EntityArgs &) { return makeBonus(8, UPGRADE_SLIDE, "go against wall while falling"); };
  r["bonus"] = [] (EntityArgs &) { return makeBonus(0, 0, "life up"); };
  r["spider"] = [] (EntityArgs &) { return make_unique<Spider>(); };
  r["spikes"] = [] (EntityArgs &) { return make_unique<Spikes>(); };
  r["fragile_door"] = [] (EntityArgs &) { return makeBreakableDoor(); };
  r["fragile_block"] = [] (EntityArgs &) { return make_unique<FragileBlock>(); };
  r["crumble_block"] = [] (EntityArgs &) { return make_unique<CrumbleBlock>(); };
  r["door"] = [] (EntityArgs& args) { auto arg = atoi(args[0].c_str()); return makeDoor(arg); };
  r["auto_door"] = [] (EntityArgs &) { return makeAutoDoor(); };
  r["switch"] = [] (EntityArgs& args) { auto arg = atoi(args[0].c_str()); return makeSwitch(arg); };
  r["moving_platform"] = [] (EntityArgs& args) { auto arg = atoi(args[0].c_str()); return make_unique<MovingPlatform>(arg); };
  r["finish"] = [] (EntityArgs &) { return make_unique<FinishLine>(); };
  r["conveyor"] = [] (EntityArgs &) { return make_unique<Conveyor>(); };
  r["sign"] = [] (EntityArgs& args) { auto arg = atoi(args[0].c_str()); return make_unique<Sign>(arg); };

  // aliases for legacy levels
  r["mp"] = r["moving_platform"];

  return r;
}

