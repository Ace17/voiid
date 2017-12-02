#include "room.h"
#include "entity_factory.h"
#include "base/mesh.h"
#include <stdlib.h>

Room Graph_loadRoom(int roomIdx, IGame* game)
{
  Room r;

  {
    char filename[256];
    snprintf(filename, sizeof filename, "res/rooms/room-%02d.3ds", roomIdx);
    r.world = *loadMesh(filename);
  }

  r.name = "test room";

  r.start = Vector3i(8 + 4, 4, 8 + 4);
  r.theme = 2;

  for(int k = 0; k < 4; ++k)
  {
    auto door = createEntity("door(0)");
    door->pos = Vector3f(15, 3 + k * 8, 1);
    door->pos += Vector3f(0, 0.5, 0.5);
    game->spawn(door.release());

    auto switch_ = createEntity("switch(0)");
    switch_->pos = Vector3f(4, 6 + k * 9, 2);
    game->spawn(switch_.release());
  }

  {
    auto switch_ = createEntity("upgrade_shoot");
    switch_->pos = Vector3f(10, 13, 2);
    game->spawn(switch_.release());
  }

  {
    auto switch_ = createEntity("upgrade_ball");
    switch_->pos = Vector3f(20, 13, 2);
    game->spawn(switch_.release());
  }

  return r;
}

