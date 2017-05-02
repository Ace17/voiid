#!/usr/bin/env rdmd

import std.conv;
import std.array;
import std.string;
import std.algorithm;
import std.stdio;

int main()
{
  Pos[] r;

  Segment sx;
  Segment sy;
  Segment sz;

  foreach(line; stdin.byLine)
  {
    if(startsWith(line, "#"))
      continue;

    auto fields = splitter(strip(line));
    Pos v;
    v.x = to!int(fields.front()); fields.popFront();
    v.y = to!int(fields.front()); fields.popFront();
    v.z = to!int(fields.front()); fields.popFront();

    sx.add(v.x);
    sy.add(v.y);
    sz.add(v.z);

    r ~= v;
  }

  const cx = roundUp(sx.right - sx.left, 8);
  const cy = roundUp(sy.right - sy.left, 8);
  const cz = roundUp(sz.right - sz.left, 8);

  foreach(ref v; r)
  {
    v.x -= sx.left;
    v.y -= sy.left;
    v.z -= sz.left;
  }

  bool[Pos] map;
  foreach(v; r)
    map[v] = true;

  writefln("int tab[%s][%s][%s] =", cz, cy, cx);
  writeln("{");
  for(int z=0;z < cz;++z)
  {
    writeln("  { ");
    for(int y=0;y < cy;++y)
    {
      write("    { ");
      for(int x=0;x < cx;++x)
      {
        if(Pos(x, y, z) in map)
          writef("%s, ", 1);
        else
          writef("%s, ", 0);
      }
      writeln("},");
    }
    writeln("  },");
  }
  writeln("};");


  return 0;
}

int roundUp(int val, int div)
{
  return ((val + div - 1) / div) * div;
}

struct Segment
{
  void add(int val)
  {
    left = min(left, val);
    right = max(right, val);
  }

  int left = int.max, right = int.min;
}

struct Pos
{
  int x, y, z;
}

