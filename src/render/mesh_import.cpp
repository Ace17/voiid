// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Loader for the Blender exporter format

#include "base/mesh.h"
#include "base/span.h"

#include <stdint.h>
#include <string>

#include "misc/file.h"

ImportedScene parseFbx(Span<const uint8_t> data);

void applyTransforms(ImportedScene& scene)
{
  for(auto& model : scene.meshes)
  {
    const auto normalTransform = transpose(invertStandardMatrix(model.transform));

    for(auto& v : model.vertices)
    {
      {
        Vector4f vert = { v.x, v.y, v.z, 1 };
        vert = model.transform * vert;

        v.x = vert.x;
        v.y = vert.y;
        v.z = vert.z;
      }

      {
        Vector4f normal = { v.nx, v.ny, v.nz, 0 };
        normal = normalTransform * normal;

        v.nx = normal.x;
        v.ny = normal.y;
        v.nz = normal.z;
      }
    }
  }
}

ImportedScene importMesh(String path)
{
  auto fbxData = File::read(path);
  auto scene = parseFbx({ (uint8_t*)fbxData.c_str(), (int)fbxData.size() });
  applyTransforms(scene);
  return scene;
}

