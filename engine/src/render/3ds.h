// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>

#include "base/mesh.h"
#include "base/span.h"

namespace tds
{
std::vector<Mesh> load(std::string filename);
std::vector<Mesh> load(Span<uint8_t const> buffer);
}

