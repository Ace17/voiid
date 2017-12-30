/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Loader for rooms (levels)

#include "room.h"
#include "entity_factory.h"
#include "base/mesh.h"
#include <stdlib.h>
#include <fstream>

static Vector3f toVector3f(Mesh::Vertex v)
{
  return Vector3f(v.x, v.y, v.z);
}

Room loadRoom(int roomIdx, IGame* game)
{
  Room r;

  char filename[256];
  snprintf(filename, sizeof filename, "res/rooms/%02d/mesh.3ds", roomIdx);

  if(!ifstream(filename).is_open())
  {
    snprintf(filename, sizeof filename, "res/rooms/ending/mesh.3ds");
    roomIdx = 0;
  }

  auto mesh = *loadMesh(filename);

  for(int objIdx = 0; objIdx < (int)mesh.objects.size(); ++objIdx)
  {
    auto name = mesh.objectNames[objIdx];

    if(name.substr(0, 2) == "f.")
    {
      auto ent = createEntity(name.substr(2));
      ent->pos = toVector3f(mesh.vertices[mesh.faces[mesh.objects[objIdx]].i1]);
      game->spawn(ent.release());
      continue;
    }

    const int beginFace = mesh.objects[objIdx];
    const int endFace = objIdx + 1 < (int)mesh.objects.size() ? mesh.objects[objIdx + 1] : (int)mesh.faces.size();

    Convex brush;

    for(int faceIdx = beginFace; faceIdx < endFace; ++faceIdx)
    {
      auto& face = mesh.faces[faceIdx];
      auto A = toVector3f(mesh.vertices[face.i1]);
      auto B = toVector3f(mesh.vertices[face.i2]);
      auto C = toVector3f(mesh.vertices[face.i3]);

      auto const N = normalize(crossProduct(B - A, C - A));
      auto const D = dotProduct(N, A);
      brush.planes.push_back(Plane { N, D });
    }

    r.brushes.push_back(brush);
  }

  r.start = Vector3i(0, 0, 5);

  return r;
}

