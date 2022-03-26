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

ImportedScene importMesh(String path)
{
  auto fbxData = File::read(path);
  return parseFbx({ (uint8_t*)fbxData.c_str(), (int)fbxData.size() });
}

