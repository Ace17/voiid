#include "room.h"
#include "entity_factory.h"
#include "base/mesh.h"
#include <stdlib.h>
#include <fstream>

static Vector3f toVector3f(Mesh::Vertex v)
{
  return Vector3f(v.x, v.y, v.z);
}

Room Graph_loadRoom(int roomIdx, IGame* game)
{
  Room r;

  r.name = "test room";

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

    Brush brush;

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
  r.theme = roomIdx;

  return r;
}

