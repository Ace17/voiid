#include <stdlib.h>
#include "room.h"
#include "entity_factory.h"

Room Graph_loadRoom(int /*roomIdx*/, IGame* /*game*/)
{
  Room r;

  r.name = "test room";

  auto const W = 8;
  auto const H = 4;

  auto isBoundary =
    [&] (Vector3i v)
    {
      if(v.x == 0 || v.x == W)
        return 1;

      if(v.y == 0 || v.y == W)
        return 1;

      if(v.z == 0 || v.z == H)
        return 1;

      return 0;
    };

  auto getTile =
    [&] (Vector3i v)
    {
      if(v.x == 0 || v.y == 0 || v.z == 0)
        return 1;

      v.x--;
      v.y--;
      v.z--;

      if(!isBoundary(v))
        return 0;

      if(abs(v.x - W) <= 1)
        return 0;

      if(abs(v.y - W) <= 1)
        return 0;

      if(abs(v.z - H) <= 1)
        return 0;

      return 1;
    };

  auto onCell =
    [&] (int x, int y, int z, int /*tile*/)
    {
      r.tiles.set(x, y, z, getTile(Vector3i(x, y, z)));

      if(rand() % 123 == 0)
        r.tiles.set(x, y, z, 1);
    };

  r.tiles.resize(Size3i(64, 64, 8));
  r.tiles.scan(onCell);

  r.start = Vector3i(4, 4, 4);
  r.theme = 2;

  return r;
}

