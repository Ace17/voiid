#pragma once

#include <vector>
#include <memory>
#include <string>

#include "base/span.h"
#include "base/mesh.h"

namespace tds
{
std::unique_ptr<Mesh> load(std::string filename);
std::unique_ptr<Mesh> load(Span<uint8_t const> buffer);
}

