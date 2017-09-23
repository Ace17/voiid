#include <map>
#include <stdlib.h>
#include "room.h"
#include "entity_factory.h"

static int roomA(Vector3i v);
static int roomB(Vector3i v);
static int fullRoom(Vector3i v);
static int emptyRoom(Vector3i v);
static int roomStairs(Vector3i v);

Room Graph_loadRoom(int /*roomIdx*/, IGame* game)
{
  Room r;

  r.name = "test room";

  auto const CX = 4;
  auto const CY = 4;
  auto const CZ = 2;

  // high-level map
  static const char minimap[CZ][CY][CX] =
  {
    {
      { 'A', 'S', 'B', 'A' },
      { 'A', '.', '.', 'A' },
      { 'A', '.', '.', 'A' },
      { 'A', 'B', 'A', 'A' },
    },
    {
      { ' ', ' ', 'A', 'A' },
      { ' ', ' ', 'A', 'A' },
      { 'A', 'A', 'A', 'A' },
      { 'A', 'A', 'A', 'A' },
    },
  };

  typedef int (* TileFunc)(Vector3i);
  map<char, TileFunc> funcs;
  funcs['A'] = &roomA;
  funcs['B'] = &roomB;
  funcs['.'] = &fullRoom;
  funcs[' '] = &emptyRoom;
  funcs['S'] = &roomStairs;

  auto const SIZE = 16;

  auto onCell =
    [&] (int x, int y, int z, int /*tile*/)
    {
      auto const cx = clamp(x / SIZE, 0, CX);
      auto const cy = clamp(y / SIZE, 0, CY);
      auto const cz = clamp(z / SIZE, 0, CZ);

      auto func = funcs.at(minimap[cz][cy][cx]);
      auto tile = func(Vector3i(x % SIZE, y % SIZE, z % SIZE));
      r.tiles.set(x, y, z, tile);
    };

  r.tiles.resize(Size3i(SIZE * CX, SIZE * CY, SIZE * CZ));
  r.tiles.scan(onCell);

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

static int roomA(Vector3i v)
{
  v.x = clamp(v.x, 0, 7);
  v.y = clamp(v.y, 0, 7);
  v.z = clamp(v.z, 0, 7);
  static int const data[8][8][8] =
  {
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 0, 0, 0, 0, 1, 1, },
      { 1, 1, 0, 0, 0, 0, 1, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 1, 1, 0, 0, 0, },
      { 0, 0, 0, 1, 1, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 1, 0, 0, 0, 0, 1, 1, },
      { 1, 1, 0, 0, 0, 0, 1, 1, },
    },
    {
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
    },
    {
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
    },
    {
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
    },
    {
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
    },
  };

  return data[v.z % 8][v.y % 8][v.x % 8];
}

static int roomB(Vector3i v)
{
  v.x = clamp(v.x, 0, 7);
  v.y = clamp(v.y, 0, 7);
  v.z = clamp(v.z, 0, 7);
  int data[8][8][8] =
  {
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 0, 0, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 0, 0, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 0, 0, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 0, 0, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
    },
    {
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
    },
    {
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
    },
  };

  return data[v.z % 8][v.y % 8][v.x % 8];
}

static int fullRoom(Vector3i v)
{
  if(v.x == 0 || v.y == 0 || v.z == 0)
    return 1;

  if(v.x == 7 || v.y == 7 || v.z == 7)
    return 1;

  return 0;
}

static int emptyRoom(Vector3i v)
{
  (void)v;
  return 0;
}

static int roomStairs(Vector3i v)
{
  const int data[8][8][8] =
  {
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 0, 0, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 1, 0, 0, 0, 0, },
      { 0, 0, 1, 1, 1, 0, 0, 0, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 0, 0, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 0, 0, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 0, 0, 0, 0, 0, 0, 0, 0, },
      { 0, 0, 0, 1, 0, 0, 0, 0, },
      { 1, 0, 0, 1, 0, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 0, 0, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 1, 1, 0, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 1, 0, 1, },
      { 1, 0, 0, 0, 1, 1, 0, 1, },
      { 1, 1, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 1, 0, 1, },
      { 1, 0, 0, 0, 0, 1, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 1, 0, 1, },
      { 1, 0, 0, 0, 0, 1, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 0, 0, 0, 0, 0, 0, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
    {
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 1, 1, },
      { 1, 0, 0, 0, 0, 0, 1, 1, },
      { 1, 1, 1, 1, 1, 1, 1, 1, },
    },
  };
  return data[v.z % 8][v.y % 8][v.x % 8];
}

