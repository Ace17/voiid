#include <stdlib.h>
#include "room.h"
#include "entity_factory.h"

Room Graph_loadRoom(int /*roomIdx*/, IGame* /*game*/)
{
  Room r;

  r.name = "test room";

  auto onCell =
    [&] (int x, int y, int z, int /*tile*/)
    {
      if(z <= 2 || z >= 5)
        r.tiles.set(x, y, z, 1);

      if(x == 0 || y == 0 || x == 63 || y == 63)
        r.tiles.set(x, y, z, 1);

      if(rand() % 13 == 0)
        r.tiles.set(x, y, z, 1);
    };

  r.tiles.resize(Size3i(64, 64, 8));
  r.tiles.scan(onCell);

  r.start = Vector3i(4, 4, 4);
  r.theme = 2;

  return r;
}

