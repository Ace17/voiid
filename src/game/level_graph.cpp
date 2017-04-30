#include "room.h"
#include "entity_factory.h"

Room Graph_loadRoom(int /*roomIdx*/, IGame* /*game*/)
{
  Room r;

  r.name = "test room";

  auto onCell =
    [&] (int x, int y, int z, int /*tile*/)
    {
      if(z <= 2)
        r.tiles.set(x, y, z, 1);
    };

  r.tiles.resize(Size3i(16, 16, 16));
  r.tiles.scan(onCell);

  r.start = Vector3i(4, 4, 4);

  return r;
}

