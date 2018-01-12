#pragma once

#include <vector>
#include <string>

#include "base/span.h"
#include "base/mesh.h"

namespace tds
{
Mesh load(std::string filename);
Mesh load(Span<uint8_t const> buffer);
}

