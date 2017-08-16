#pragma once

#include "player.h"
#include "game.h" // Matrix
#include <memory>

std::unique_ptr<Player> makeEditor(Matrix& tiles);

